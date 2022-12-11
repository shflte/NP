// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXLINE 1000

struct chlist_chk {
    short list[500];
    short which;
};

typedef struct chunk {
    char legal; // == 'l' means legal. To avoid other case
    short file;
    short seq;
    char data[MAXLINE];
    short len;
} chk;

typedef struct acknowledge {
    short file;
    short seq;
    char legal; // == 'l' means legal. To avoid other case
} ack;

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

void filenum(char* buffer, int number) {
    bzero(buffer, MAXLINE);
    sprintf(buffer, "file %d", number);
}

int num_of_filenum(char* str) { // parse the number from the filenum string
    char tmp[20];
    strcpy(tmp, str);
    char* tok = strtok(tmp, " ");
    tok = strtok(NULL, " ");
    return atoi(tok);
}

int main(int argc, char **argv) {
	int sockfd;
	char sendbuf[MAXLINE];
	char recvbuf[MAXLINE];
	struct sockaddr_in	 servaddr;
    int nof = atoi(argv[2]); // num of file
    FILE *fptr;
    char filename[10], path[50];
    int filelen;

	
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[3]));
	servaddr.sin_addr.s_addr = INADDR_ANY;

    inet_pton(AF_INET, argv[4], &servaddr.sin_addr);

	int n;
    socklen_t len;
	len = sizeof(servaddr); //len is value/result


    // calculate chunk list 
    short chunk_list[1000];
    short filelen_list[1000];
    for (int i = 0; i < 1000; i++) {
        // get fullpath
        ntofn(filename, i);
        bzero(path, sizeof(path));
        strcpy(path, argv[1]);
        strcat(path, "/");
        strcat(path, filename);
        fptr = fopen(path, "r");
        if (fptr == NULL) {
            printf("%s not exist?\n", path);
        }
        fseek(fptr, 0, SEEK_END);
        filelen = ftell(fptr);
        filelen_list[i] = filelen;
        chunk_list[i] = (filelen % MAXLINE == 0) ? (filelen / MAXLINE) : (filelen / MAXLINE + 1);
        fclose(fptr);
    }

    struct chlist_chk first, second;
    first.which = 1;
    second.which = 2;
    for (int i = 0; i < 500; i++) {
        first.list[i] = chunk_list[i];
        second.list[i] = chunk_list[500 + i];
    }
    struct chlist_chk* first_ptr = &first;
    struct chlist_chk* second_ptr = &second;

    // send chunklist to server
    // while (1) {
        // sendto(sockfd, (const char *)first_ptr, sizeof(struct chlist_chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        // sendto(sockfd, (const char *)second_ptr, sizeof(struct chlist_chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        // bzero(recvbuf, sizeof(recvbuf));
        // recvfrom(sockfd, (char *)recvbuf, MAXLINE, MSG_DONTWAIT, ( struct sockaddr *) &servaddr, &len);
        // if (strncmp(recvbuf, "both", 4) == 0) {
        //     break;
        // }
    // }

    // initialize empty checklist
    int* sent[1000];
    for (int i = 0; i < 1000; i++) {
        sent[i] = calloc(chunk_list[i], sizeof(int));
        for (int j = 0; j < chunk_list[i]; j++) {
            sent[i][j] = 0;
        }
    }


    // send file!
    chk dummy;
    dummy.legal = 'l';
    chk* ch_ptr = &dummy;
    ack* ack_ptr;

    int done = 0;
    while (1) {
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < chunk_list[i]; j++) {
                if (!sent[i][j]) { // this chunk is not sent
                    // get fullpath
                    ntofn(filename, i);
                    bzero(path, sizeof(path));
                    strcpy(path, argv[1]);
                    strcat(path, "/");
                    strcat(path, filename);

                    dummy.file = i;
                    dummy.seq = j;
                    if (filelen_list[i] % 1000 != 0) {
                        if (j == (filelen_list[i] / 1000)) { // last chunk
                            dummy.len = filelen_list[i] - 1000 * j;
                        }
                        else {
                            dummy.len = 1000;
                        }
                    }
                    else {
                        dummy.len = 1000;
                    }

                    fptr = fopen(path, "r");
                    fseek(fptr, j * 1000, SEEK_SET);
                    bzero(dummy.data, sizeof(dummy.data));
                    fread(dummy.data, sizeof(char), dummy.len, fptr);
                    fclose(fptr);
                    // the more packets, the more chance data is sent successfully... right?
                    sendto(sockfd, (const char *)ch_ptr, sizeof(chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                    sendto(sockfd, (const char *)ch_ptr, sizeof(chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                    sendto(sockfd, (const char *)ch_ptr, sizeof(chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                }
            }
        }

        // reason to call that many time "recvfrom()" is there could be many ack packets in the buffer
        for (int i = 0; i < 1000; i++) {
            for (int j = 0; j < chunk_list[i]; j++) {
                recvfrom(sockfd, recvbuf, MAXLINE, MSG_DONTWAIT, ( struct sockaddr *) &servaddr, &len);
                ack_ptr = (ack*) recvbuf;
                if (ack_ptr->legal == 'l') {
                    sent[ack_ptr->file][ack_ptr->seq] = 1;
                }
            }
        }

        // check if all files are sent
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

        if (!done) {
            continue;
        }

        break;
    }

    bzero(sendbuf, sizeof(sendbuf));
    strcpy(sendbuf, "done");
    // probability that all of 5 packets failed is 0.01024
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("client done\n");

	close(sockfd);
	return 0;
}
