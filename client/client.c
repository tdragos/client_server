#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define ADDR "127.0.0.1"
#define PORT 4443
#define BUFSIZE 1000

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage is ./client [FILENAME]");
        exit(1);
    }
    
    int sockfd, status, domain;
    struct timeval tm;
    struct sockaddr_storage sa;
    char check_protocol[16];
    
    //check if we have an ipv4 or an ipv6 address
    if (inet_pton(AF_INET, ADDR, check_protocol)) {
        domain = AF_INET;
        ((struct sockaddr_in *)&sa)->sin_port = htons(PORT);
        ((struct sockaddr_in *)&sa)->sin_family = AF_INET;
        inet_pton(AF_INET, ADDR, &((struct sockaddr_in *)&sa)->sin_addr);
        
    }
    else if (inet_pton(AF_INET6, ADDR, check_protocol)) {
        domain = AF_INET6;
        ((struct sockaddr_in6 *)&sa)->sin6_port = htons(PORT);
        ((struct sockaddr_in6 *)&sa)->sin6_family = AF_INET6;
        inet_pton(AF_INET6, ADDR, &((struct sockaddr_in6 *)&sa)->sin6_addr);
    }
    else {
        perror("Invalid Addres");
        exit(1);
    }
    
    //create the socket
    sockfd = socket(domain, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("create socket");
        exit(1);
    }
    
    //connect to the server
    status = connect(sockfd, (const struct sockaddr *)&sa, sizeof(sa));
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
    //we set a timeout of 1000 ns for recv
    int openfd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    
    tm.tv_sec = 0;
    tm.tv_usec = 1000;
    
    //add the timeout for recv for our socket
    status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tm, sizeof tm);
    if (status < 0) {
        perror ("setsock");
        exit(1);
    }
    
    //get the file
    while ((chars_read = recv(sockfd, buf, BUFSIZE, 0)) > 0) {
        chars_written = write(openfd, buf, chars_read);
        if (chars_written < 0) {
            perror ("write failed, please get the file again");
            exit(1);
        }
    }
    
    printf("Received the file\n");
    
    //close the file
    close(openfd);
    close(sockfd);
    
    return 0;
}
