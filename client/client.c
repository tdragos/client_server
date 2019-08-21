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
#define PORT 4445
#define BUFSIZE 1000

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage is ./client [FILENAME]");
        exit(1);
    }
    
    int sockfd, status;
    struct sockaddr_in sa;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("create socket");
        exit(1);
    }
    
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    inet_aton(ADDR, &sa.sin_addr);
    
    //connect to the server
    status = connect(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in));
    if (status < 0) {
        perror("connect");
        exit(1);
    }
    
    //send the filename to the server
    status = write(sockfd, argv[1], strlen(argv[1]));
    if (status < 0) {
        perror("write");
        exit(1);
    }
    
    //check if the server has the file
    int chars_read, chars_written;
    char buf[BUFSIZE];
    chars_read = recv(sockfd, buf, 14, 0);
    if (!strncmp(buf, "File not found", 14)) {
        printf("File not found");
        return 0;
    }
    
    //receive the file
    //we use select to see when we get data from the server
    //we set a timeout of 1 second
    int openfd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    
    while (1) {
        status = select(sockfd + 1, &fds, NULL, NULL, &tm);
        if (status == -1) {
            perror("select");
            close(openfd);
            return 0;
        }
        else if (status) {
            chars_read = recv(sockfd, buf, BUFSIZE, 0);
            if (chars_read > 0)
                write(openfd, buf, chars_read);
        }
        else if (status == 0)
            //timeout => file was received so we exit the loop
            break;
    }
    
    //close the file
    close(openfd);
    
    printf("Received the file\n");
    
    return 0;
}
