#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define ADDR "127.0.0.1"
#define MAX_CLIENTS 5
#define BASE 10
#define BUFSIZE 1000

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage is ./server PORT\n");
        exit(1);
    }
    
    int sockfd, status, connection_socket;
    char *ptr;
    struct sockaddr_in sa;

    //create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("create socket");
        exit(1);
    }

    sa.sin_family = AF_INET;
    inet_aton(ADDR, &sa.sin_addr);
    //get the port from argv[1] with strtol (atoi is deprecated)
    sa.sin_port = htons((unsigned short)strtol(argv[1], &ptr, BASE));

    status = bind(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
    if (status < 0) {
        perror("bind");
        exit(1);
    }

    status = listen(sockfd, MAX_CLIENTS);
    if (status < 0) {
        perror("listen");
        exit(1);
    }
    

    //server waits for clients forever
    while (1) {
        //accept a connection
        connection_socket = accept(sockfd, NULL, NULL);
        if (connection_socket < 0) {
            perror("accept");
            exit(1);
        }

        //read the filename. We assume it's length is max 20
        int chars_read, chars_written;
        char filename[20];
        
        chars_read = read(connection_socket, filename, 20);
        if (chars_read <= 0) {
            perror("error receiving filename, we tell the client we don't have the file");
            write(connection_socket, "File not found", 14);
            continue;
        }
        filename[chars_read] = '\0';
        
        //tell the client we have the file
        chars_written = send(connection_socket, "File was found", 14, 0);
        
        //open the file
        int open_fd = open(filename, O_RDONLY);
        if (open_fd < 0) {
            //if open fails it means we don't have the file
            perror("we don't have the file");
            write (connection_socket, "File not found", 14);
            continue;
        }
        
        //send the file
        char buf[BUFSIZE];
        while ((chars_read = read(open_fd, buf, BUFSIZE)) > 0) {
            chars_written = send(connection_socket, buf, chars_read, 0);
        }

        //close the file
        close(open_fd);
        
        printf("Send was successful\n");
    }
    
    return 0;
}
