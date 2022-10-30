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

static struct timeval _t0;
static unsigned long long bytesent = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {
	struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n\n",
		_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0));
	exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, n, status;
    struct sockaddr_in servaddr;

    // socket()
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("socket error\n");

    bzero(&servaddr, sizeof(servaddr));
    inet_pton(AF_INET, "140.113.213.213", &(servaddr.sin_addr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(10003); 

    // connect
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
        printf("connect error\n");

    // handle signal to count byterate
    signal(SIGINT,  handler);
	signal(SIGTERM, handler);

	gettimeofday(&_t0, NULL);

    // send data to dummy server
    char st[] = "hahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahahaha";
    struct timespec t = { 0, 1 };
    double bitrate = strtod(argv[1], NULL);
    t.tv_nsec = 1;

	while (1) {
        write(sockfd, st, strlen(st));
        // +66 for bytes sent besides the data (tcp header, ip header, ...)
        bytesent += strlen(st) + 66;
		nanosleep(&t, NULL);
	}

    return 0;
}