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
#define NOF 1 // number of phase

typedef struct give_me_chunk {
    char legal;
    short file;
    short seq;
} gmc;

struct chlist_chk {
    char chlist;
    short list[1000];
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
    gmc* gmc_ptr;
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
	len = sizeof(servaddr); // len is value/result

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

    // send chunklist to server
    struct chlist_chk* hahaptr = calloc(1, sizeof(struct chlist_chk));
    for (int i = 0; i < 1000; i++) {
        hahaptr->list[i] = chunk_list[i];
    }
    hahaptr->chlist = 'y';

    while (1) {
        sendto(sockfd, (const char *)hahaptr, sizeof(struct chlist_chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        bzero(recvbuf, sizeof(recvbuf));
        recvfrom(sockfd, recvbuf, sizeof(gmc), MSG_DONTWAIT, ( struct sockaddr *) &servaddr, &len);
        gmc_ptr = (gmc*) recvbuf;
        if (gmc_ptr->legal == 'l') {
            break;
        }
    }
    printf("chunklist sent!\n");
    
    int* sent[1000];
    for (int i = 0; i < 1000; i++) 
        sent[i] = calloc(chunk_list[i], sizeof(int));

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < chunk_list[i]; j++) {
            sent[i][j] = 0;
        }
    }

    chk dummy;
    dummy.legal = 'l';
    chk* ch_ptr = &dummy;
    ack* ack_ptr;

    int done = 0;
    int phase = 0;
    int haha = 0;
    int phaselen = 1000 / NOF;

    while (1) {
        bzero(recvbuf, sizeof(recvbuf));
        recvfrom(sockfd, recvbuf, sizeof(recvbuf), MSG_DONTWAIT, ( struct sockaddr *) &servaddr, &len);
        if (strncmp(recvbuf, "hehe", 4) == 0) {
            break;
        }
        gmc_ptr = (gmc*) recvbuf;
        if (gmc_ptr->legal == 'l') {
            ntofn(filename, gmc_ptr->file);
            bzero(path, sizeof(path));
            strcpy(path, argv[1]);
            strcat(path, "/");
            strcat(path, filename);

            dummy.file = gmc_ptr->file;
            dummy.seq = gmc_ptr->seq;
            if (filelen_list[gmc_ptr->file] % 1000 != 0) {
                if (gmc_ptr->seq == (filelen_list[gmc_ptr->file] / 1000)) { // last chunk
                    dummy.len = filelen_list[gmc_ptr->file] - 1000 * gmc_ptr->seq;
                }
                else {
                    dummy.len = 1000;
                }
            }
            else {
                dummy.len = 1000;
            }

            fptr = fopen(path, "rb");
            fseek(fptr, gmc_ptr->seq * 1000, SEEK_SET);
            bzero(dummy.data, sizeof(dummy.data));
            fread(dummy.data, sizeof(char), dummy.len, fptr);
            fclose(fptr);
            // the more packets, the more chance data is sent successfully... right?
            sendto(sockfd, (const char *)ch_ptr, sizeof(chk), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        }
    }

    printf("client done\n");

	close(sockfd);
	return 0;
}
