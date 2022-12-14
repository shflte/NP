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

struct client{
	int			fd;
	char 		name[50];
	char		ip_port[30];
};

void initial_client(struct client *client) {
	client->fd = -1;
	bzero(client->name, sizeof(client->name));
	strcpy(client->name, "");
	bzero(client->ip_port, sizeof(client->ip_port));
	strcpy(client->ip_port, "");
}

void handle(int signal) {

}

double gettime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double time = tv.tv_sec + tv.tv_usec * 0.000001;

	return time;
}

// 1: command channel
// 2: sink server

int main(int argc, char **argv) {
	int					listenfd, listenfd2, newfd, newfd2, sockfd;
	int 				nready;
	int 				numcli;
	int					yes = 0;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, cliaddr2, servaddr, servaddr2;
	struct client		clilist[15];
	int 				counter;
	char				buf[500];
	double 				time, last_reset;
	char*				str_read;
	char* 				del = " ";
	char* 				parsed;
	int					byte_read;
	fd_set 				rset, allset;
	struct timeval 		waittime = {0, 0}; // 0.1s?
	int	 				optval;

	listenfd  = socket(AF_INET, SOCK_STREAM, 0);
	listenfd2 = socket(AF_INET, SOCK_STREAM, 0);

	optval = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
	if (setsockopt(listenfd2, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
	if (setsockopt(listenfd2, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	bzero(&servaddr2, sizeof(servaddr2));
	servaddr2.sin_family      = AF_INET;
	servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr2.sin_port        = htons(atoi(argv[1]) + 1);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	bind(listenfd2, (struct sockaddr *) &servaddr2, sizeof(servaddr2));
	listen(listenfd, 50);
	listen(listenfd2, 50);

	// handle broken pipe
	signal(SIGPIPE, handle);

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	FD_SET(listenfd2, &allset);

	// initial client list
	for (int i = 0; i < 15; i++) {
		initial_client(&clilist[i]);
	}
	numcli = 0;

	while (1) {
		rset = allset;
		nready = select(1000, &rset, NULL, NULL, &waittime);

		if (nready < 0) {
			exit(1);
		}
		else if (nready == 0) { // no fd is ready
			continue;
		}
		else {
			if (FD_ISSET(listenfd, &rset)) { // command server
				// accept new connection
				clilen = sizeof(cliaddr);
				newfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

				FD_SET(newfd, &allset);
			}
			if (FD_ISSET(listenfd2, &rset)) { // sink server
				// accept new connection
				clilen = sizeof(cliaddr2);
				newfd2 = accept(listenfd2, (struct sockaddr *) &cliaddr2, &clilen);
				for (int i = 0; i < 15; i++) { // find a space for new client
					if (clilist[i].fd == -1) {
						clilist[i].fd = newfd2;		
						numcli++;

						FD_SET(newfd2, &allset);

						// attain client ip & port
						sprintf(clilist[i].ip_port, "%s", inet_ntoa(cliaddr.sin_addr));

						// server log
						printf("* sink client connected from %s\n", clilist[i].ip_port);

						break;
					}
				}
			}
			if (FD_ISSET(newfd, &rset)) { // command client
				bzero(buf, sizeof(buf));
				byte_read = read(newfd, buf, 100);
				if (byte_read == 0) {
					close(newfd);
					FD_CLR(newfd, &allset);
					continue;
				}

				str_read = calloc(byte_read - 1, sizeof(char));
				strncpy(str_read, buf, byte_read - 1);
				char* to_parse = calloc(strlen(str_read) + 1, sizeof(char));
				strcpy(to_parse, str_read);
				parsed = strtok(to_parse, del);

				if (byte_read - 1 == 0) {
					continue;
				}
				if (strcmp(parsed, "/reset") == 0) {
					bzero(buf, sizeof(buf));
					last_reset = time = gettime();
					sprintf(buf, "%f RESET %d\n", time, counter);
					write(newfd, buf, strlen(buf));
					counter = 0;
				}
				else if (strcmp(parsed, "/ping") == 0) {
					bzero(buf, sizeof(buf));
					time = gettime();
					sprintf(buf, "%f PONG\n", time);
					write(newfd, buf, strlen(buf));
				}
				else if (strcmp(parsed, "/report") == 0) {
					bzero(buf, sizeof(buf));
					time = gettime();
					sprintf(buf, "%f REPORT %d %fs %fMbps\n", time, counter, time - last_reset, 8.0 * counter / 1000000.0 / (time - last_reset));
					write(newfd, buf, strlen(buf));
				}
				else if (strcmp(parsed, "/clients") == 0) {
					bzero(buf, sizeof(buf));
					time = gettime();
					sprintf(buf, "%f CLIENT %d\n", time, numcli);
					write(newfd, buf, strlen(buf));
				}
				else {
					bzero(buf, sizeof(buf));
					sprintf(buf, "Unknown or incomplete command <%s>\n", str_read);
					write(newfd, buf, strlen(buf));
				}
			}
			for (int i = 0; i < 15; i++) {
				if ((sockfd = clilist[i].fd) < 0) {
					continue;
				}
				if (FD_ISSET(sockfd, &rset)) {
					// sink
					bzero(buf, sizeof(buf));
					byte_read = read(sockfd, buf, 100);
					if (byte_read == 0) {
						printf("close %d\n", sockfd);
						FD_CLR(sockfd, &allset);
						if (close(sockfd) < 0) {
							perror("close sockfd error: ");
							exit(-1);
						}
						numcli--;
						initial_client(&clilist[i]);
						continue;
					}
					counter += byte_read;
				}
			}
		}
	}
}