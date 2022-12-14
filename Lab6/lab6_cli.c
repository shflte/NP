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
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>

#define SA struct sockaddr
#define n 10

int sockfd, sinkfd[n];
char buf[500];
struct sockaddr_in servaddr;
struct sockaddr_in servaddr2;

void handle() {
    char buf[500];
    bzero(buf, sizeof(buf));
    sprintf(buf, "/report\n");
    write(sockfd, buf, strlen(buf));

    bzero(buf, sizeof(buf));
    read(sockfd, buf, 100);
    printf("%s", buf);  

    for (int i = 0; i < n; i++) {
        close(sinkfd[i]);
    }

    exit(0);
}

int main(int argc, char *argv[]) {
    // socket()
    int optval = 1;
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("socket error\n");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
    for (int i = 0; i < n; i++) {
        if ( (sinkfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            printf("socket2 error\n");
        if (setsockopt(sinkfd[i], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
            perror("setsockopt");
            exit(-1);
        }
        if (setsockopt(sinkfd[i], SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
            perror("setsockopt");
            exit(-1);
        }
    }

    bzero(&servaddr, sizeof(servaddr));
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(atoi(argv[1])); 

    bzero(&servaddr2, sizeof(servaddr2));
    inet_pton(AF_INET, "127.0.0.1", &(servaddr2.sin_addr));
    servaddr2.sin_family = AF_INET;
    servaddr2.sin_port   = htons(atoi(argv[1]) + 1); 

    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
        printf("connect command server error\n");

    for (int i = 0; i < n; i++) {
        if (connect(sinkfd[i], (SA *)&servaddr2, sizeof(servaddr2)) < 0)
            printf("connect sink error\n");
    }

    signal(SIGINT, handle);
    signal(SIGTERM, handle);

    bzero(buf, sizeof(buf));
    sprintf(buf, "/reset\n");
    write(sockfd, buf, strlen(buf));
    bzero(buf, sizeof(buf));
    read(sockfd, buf, 100);
    printf("%s", buf);

    char g[1000000];
    memset(g, 'a', sizeof(g));

    while (1) {
        for(int i = 0; i < n; i++) {
            write(sinkfd[i], g, sizeof(g));
        }
        // sleep(0.05);
    }

    return 0;
}