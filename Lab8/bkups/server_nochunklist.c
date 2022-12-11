// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
	
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

int num_of_filenum(char* str) { // parse the number from the filenum string
    char tmp[20];
    strcpy(tmp, str);
    char* tok = strtok(tmp, " ");
    tok = strtok(NULL, " ");
    return atoi(tok);
}

int main(int argc, char **argv) {
	int sockfd, fd;
	char sendbuf[MAXLINE];
	char recvbuf[MAXLINE];
	struct sockaddr_in servaddr, cliaddr;
    int nof = atoi(argv[2]); // num of file
    FILE *fptr;
    char filename[10], path[50];
    int end;
    chk* ch_ptr;
    int chunk_list[1000];
    

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
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

    // receive chunk from client
    ack dummy;
    dummy.legal = 'l';
    ack* ack_ptr = &dummy;
    while (1) {
        bzero(recvbuf, sizeof(recvbuf));
        recvfrom(sockfd, recvbuf, sizeof(chk), MSG_DONTWAIT, ( struct sockaddr *) &cliaddr, &len); // make sure client know server know which file is gonna be send
        
        // exit if done
        if (strncmp(recvbuf, "done", 4) == 0) {
            printf("server got done\n");

            printf("exit\n");
            exit(0);
        }

        ch_ptr = (chk*) recvbuf;
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

            // ack the client (no matter if server has received this chunk)
            dummy.file = ch_ptr->file;
            dummy.seq = ch_ptr->seq;
            sendto(sockfd, (const char *)ack_ptr, sizeof(ack), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)); // ack client that the message is received
        }
    }

	return 0;
}
