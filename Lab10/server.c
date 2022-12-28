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
    sockfd = socket(AF_INET, SOCK_RAW, 6969);
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(atoi(argv[3]));
		
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
        sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
		
	socklen_t len = sizeof(cliaddr); //len is value/result
    int n;

    // receive chunklist
    struct chlist_chk* hahaptr;
    bzero(recvbuf, sizeof(recvbuf));
    while (1) {
        recvfrom(sockfd, recvbuf, sizeof(struct chlist_chk), MSG_DONTWAIT, ( struct sockaddr *) &cliaddr, &len); // make sure client know server know which file is gonna be send
        hahaptr = (struct chlist_chk*) recvbuf;
        if (hahaptr->chlist == 'y') {
            printf("copying...\n");
            for (int i = 0; i < 1000; i++) {
                chunk_list[i] = hahaptr->list[i];
            }
            break;
        }
    }

    chk** data = calloc(1000, sizeof(chk*));
    for (int i = 0; i < 1000; i++) {
        data[i] = calloc(chunk_list[i], sizeof(chk));
        for (int j = 0; j < chunk_list[i]; j++) {
            bzero(data[i][j].data, sizeof(data[i][j].data));
            data[i][j].file = i;
            data[i][j].seq = j;
            data[i][j].len = 0;
            data[i][j].legal = 'l';
        }
    }

    int* sent[1000];
    for (int i = 0; i < 1000; i++) 
        sent[i] = calloc(chunk_list[i], sizeof(int));

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < chunk_list[i]; j++) {
            sent[i][j] = 0;
        }
    }

    struct timeval time = {0, 10};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&time,sizeof(time));

    gmc* gmc_ptr = calloc(1, sizeof(gmc));
    gmc_ptr->legal = 'l';
    int done = 0;

    while (!done) {
        // send "give me chunk"
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < chunk_list[i]; j++) {
                if (!sent[i][j]) {
                    gmc_ptr->file = i;
                    gmc_ptr->seq = j;
                    sendto(sockfd, gmc_ptr, sizeof(gmc), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
                }
            }
        }


        // receive chunk
        int n;
        bzero(recvbuf, sizeof(recvbuf));
        while (n = recvfrom(sockfd, recvbuf, sizeof(chk), 0, ( struct sockaddr *) &cliaddr, &len)) { // make sure client know server know which file is gonna be send
            if (n < 0) {
                printf("break\n");
                break;
            }

            ch_ptr = (chk*) recvbuf;
            if (ch_ptr->legal == 'l') {
                memcpy(&data[ch_ptr->file][ch_ptr->seq], ch_ptr, sizeof(chk));
                sent[ch_ptr->file][ch_ptr->seq] = 1;
            }
            bzero(recvbuf, sizeof(recvbuf));
        }

        done = 1;
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < chunk_list[i]; j++) {
                if (!sent[i][j]) {
                    done = 0;
                    break;
                }
            }
            if (!done) {
                break;
            }
        }
    }
    printf("\n\n\nserver done\n\n\n\n");

    printf("write to file...\n");
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < chunk_list[i]; j++) {
            ntofn(filename, i);
            bzero(path, sizeof(path));
            strcpy(path, argv[1]);
            strcat(path, "/");
            strcat(path, filename);

            fd = open(path, O_RDWR | O_CREAT, 0777);
            lseek(fd, j * 1000, SEEK_SET);
            write(fd, data[i][j].data, data[i][j].len);
            close(fd);
        }
    }

    bzero(sendbuf, sizeof(sendbuf));
    strcpy(sendbuf, "hehe");

    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received

	return 0;
}

