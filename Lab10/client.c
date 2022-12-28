// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

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
	int sockfd;
	char sendbuf[MAXLINE];
	char recvbuf[MAXLINE];
	struct sockaddr_in	servaddr;
    int nof = atoi(argv[2]); // num of file
    FILE *fptr;
    char filename[10], path[50];
    int filelen;
	
	// Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_RAW, 6969);
	
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
    uint32_t saddr, daddr;
    uint32_t* saddr_ptr = &saddr;
    uint32_t* daddr_ptr = &daddr;
    inet_pton(AF_INET, "127.0.0.1", saddr_ptr);
    inet_pton(AF_INET, argv[3], daddr_ptr);

	struct iphdr header;
    header.ihl = 5;
    header.version = 4; 
    header.tos = 0;
    header.tot_len = sizeof(struct iphdr) + sizeof(chk);
    header.id = htons(0);
    header.frag_off = 0;
    header.ttl = 64;
    header.protocol = 6969;
    header.saddr = saddr;
    header.daddr = daddr;
    header.check = 0;

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

    chk dummy;
    dummy.legal = 'l';
    chk* ch_ptr = &dummy;

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < chunk_list[i]; j++) {
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

            fptr = fopen(path, "rb");
            fseek(fptr, j * 1000, SEEK_SET);
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
