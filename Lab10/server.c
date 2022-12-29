// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/stat.h>
#include <fcntl.h>
	
#define MAXLINE 1000

typedef struct chunk {
    char legal; // == 'l' means legal. To avoid other case
    short file;
    short seq;
    char data[MAXLINE];
    short len;
} chk;

void ntofn (char* filename, int num) {
    bzero(filename, 10);
    if (num >= 1000) {
        sprintf(filename, "00%d", num);
    }
    else if (num >= 100) {
        sprintf(filename, "000%d", num);
    }
    else if (num >= 10) {
        sprintf(filename, "0000%d", num);
    }
    else {
        sprintf(filename, "00000%d", num);
    }
}

int main(int argc, char **argv) {
	int sockfd, fd;
	char sendbuf[5000];
	char recvbuf[5000];
	struct sockaddr_in servaddr, cliaddr;
    int nof = atoi(argv[2]); // num of file
    FILE *fptr;
    char filename[10], path[50];
    int end;
    chk* ch_ptr;
    int chunk_list[1000];

	// Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_RAW, 69);
    int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	socklen_t len = sizeof(cliaddr); //len is value/result

    int n;
    bzero(recvbuf, sizeof(recvbuf));
    while (n = recvfrom(sockfd, recvbuf, sizeof(chk) + 20, 0, ( struct sockaddr *) &cliaddr, &len)) { // make sure client know server know which file is gonna be send
        if (n > 0) {
            printf("recv sth.\n");
        }
        ch_ptr = (chk*) (recvbuf + 20);
        if (ch_ptr->legal == 'l') {
            // printf("file: %d; seq: %d\n", ch_ptr->file, ch_ptr->seq);
            // write to file 
            ntofn(filename, ch_ptr->file);
            bzero(path, sizeof(path));
            strcpy(path, argv[1]);
            strcat(path, "/");
            strcat(path, filename);
            fd = open(path, O_RDWR | O_CREAT, 0777);
            lseek(fd, ch_ptr->seq * 1000, SEEK_SET);
            write(fd, ch_ptr->data, ch_ptr->len);
            close(fd);
        }
        bzero(recvbuf, sizeof(recvbuf));
    }

	return 0;
}

