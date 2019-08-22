#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>

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
    int client_sockets[MAX_CLIENTS], current_client = 0, max_fd;
    char *ptr;
    struct sockaddr_in sa;
    fd_set fds;

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
    
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_sockets[i] = 0;
    
    //server waits for clients forever
    while (1) {
        
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        max_fd = sockfd;

        //verify our valid clients and get the max fd
        for (int i = 0; i < MAX_CLIENTS; i++) {
            current_client = client_sockets[i];
            
            if (current_client > 0)
                FD_SET(current_client, &fds);
            
            if (current_client > max_fd)
                max_fd = current_client;
        }
                
        status = select(max_fd + 1, &fds, NULL, NULL, NULL);
        if (status < 0) {
            perror("select");
        }
                
        //check if we get a new connection
        if (FD_ISSET(sockfd, &fds)) {
            
            //accept the connection
            connection_socket = accept(sockfd, NULL, NULL);
            if (connection_socket < 0) {
                perror("accept");
                exit(1);
            }
            printf("I accept new connection, fd: %d\n", connection_socket);
            
            //add the new client in our array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = connection_socket;
                    break;
                }
            }
            continue;
        }

        //see what client we have to send the file to
        for (int i = 0; i < MAX_CLIENTS; i++) {
            current_client = client_sockets[i];
            
            if (FD_ISSET(current_client, &fds)) {
                
                printf("CURRENT CLIENT IS: %d\n", current_client);
                //read the filename. We assume it's length is max 20
                int chars_read, chars_written;
                char filename[20];
                
                chars_read = read(current_client, filename, 20);
                if (chars_read <= 0) {
                    perror("error receiving filename, we tell the client we don't have the file");
                    write(current_client, "File not found", 14);
                    continue;
                }
                filename[chars_read] = '\0';
                
                //open the file
                int open_fd = open(filename, O_RDONLY);
                if (open_fd < 0) {
                    //if open fails it means we don't have the file
                    perror("we don't have the file");
                    write (current_client, "File not found", 14);
                    continue;
                }
                
                //tell the client we have the file
                chars_written = send(current_client, "File was found", 14, 0);
                if (chars_written < 0) {
                    perror("send file was found");
                }
                
                //send the file
                char buf[BUFSIZE];
                while ((chars_read = read(open_fd, buf, BUFSIZE)) > 0) {
                    chars_written = send(current_client, buf, chars_read, 0);
                    if (chars_written < 0) {
                        perror("send failed, go to next client");
                        continue;
                    }
                }
                
                printf("Send was successful\n");

                //close the file
                close(open_fd);
                
                //not a client any more
                client_sockets[i] = 0;
                close(current_client);
            }
        }
    }
    
    return 0;
}
