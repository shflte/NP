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



int main() {
    struct timeval tv;
	gettimeofday(&tv, NULL);
	double time = (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;

    printf("time: %f\n", time);
    printf("sec: %ld; usec: %d\n", tv.tv_sec, tv.tv_usec);
    printf("left: %f\n", (double)tv.tv_sec);
    printf("multed: %f\n", (double)tv.tv_usec * 0.000001);

    return 0;
}