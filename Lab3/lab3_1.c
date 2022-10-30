#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define SA struct sockaddr

int main(int argc, char *argv[]) {
    int sockfd, n, status;
    struct sockaddr_in servaddr;
    struct addrinfo hints, *res;

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("socket error\n");

    // resolve domain name
    memset(&hints, 0, sizeof hints); 
    status = getaddrinfo("inp111.zoolab.org", "10002", &hints, &res);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(10002); 

    if (connect(sockfd, (SA *)res->ai_addr, sizeof(servaddr)) < 0)
        printf("connect error\n");

    char buf[200];
    read(sockfd, buf, 200);
    printf("%s\n", buf);

    char go[] = "GO\n";
    write(sockfd, go, 3);

    int count = 0;
    char c[1];
    while (1) {
        read(sockfd, c, 1);
        count += 1;
        if (c[0] == '?') {
            break;
        }
    }
    read(sockfd, buf, 200);

    char num[20];
    bzero(num, 20);
    sprintf(num, "%d\n", count - 85);
    printf("num: %s\n", num);

    write(sockfd, num, 20);
    
    bzero(buf, 200);
    read(sockfd, buf, 200);
    printf("%s\n", buf);

    return 0;
}