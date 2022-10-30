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

void handle(int signal) {
	// printf("broken pipe;(\n");
	return;
}

void gettime(char* str) {
	time_t rawtime;
	struct tm* timeinfo;

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	char strtemp[21];
	sprintf(strtemp, "%d-%d-%d %d:%d:%d ", 
			timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	strcpy(str, strtemp);
}

int main(int argc, char **argv) {
	int					listenfd, newfd, sockfd;
	int 				fdmax, nready;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	struct client		clilist[1024];
	int 				numcli, climax;
	char				buf[500];
	char				time[21];
	char*				str_read;
	char* 				del = " ";
	char* 				parsed;
	int					byte_read;
	fd_set 				rset, allset;
	struct timeval 		waittime = {0, 0}; // 0.1s?

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, 50);

	// handle broken pipe
	signal(SIGPIPE, handle);

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	fdmax = listenfd + 1;

	// initial client list
	for (int i = 0; i < 1024; i++) {
		clilist[i].fd = -1;
	}
	numcli = 0;
	climax = 0;

	while (1) {
		rset = allset;
		nready = select(fdmax + 1, &rset, NULL, NULL, &waittime);

		if (nready < 0) {
			printf("error?\n");
			exit(1);
		}
		else if (nready == 0) { // no fd is ready
			continue;
		}
		else {
			if (FD_ISSET(listenfd, &rset)) { // new connection
				// accept new connection
				clilen = sizeof(cliaddr);
				newfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

				if (newfd == -1) {
					printf("%s\n", strerror(errno));
				}
				else {
					for (int i = 0; i < 1024; i++) { // find a space for new client
						if (clilist[i].fd == -1) {
							clilist[i].fd = newfd;		

							FD_SET(newfd, &allset);
							fdmax = (newfd > fdmax) ? newfd : fdmax;

							numcli++;
							climax = (i > climax) ? i : climax;

							// attain client ip & port
							sprintf(clilist[i].ip_port, "%s:%d", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

							// new name
							sprintf(clilist[i].name, "haha_%d", newfd);

							bzero(buf, sizeof(buf));
							gettime(time);
							// welcome
							sprintf(buf, "%-19s*** Welcome to the simple CHAT server\n%s *** Total %d users online now. Your name is <%s>\n", time, time, numcli, clilist[i].name);
							write(newfd, buf, strlen(buf));

							// announce other user
							sprintf(buf, "%-19s*** User <%s> has just landed on the server\n", time, clilist[i].name);
							for (int j = 0; j < climax + 1; j++) {
								if ((clilist[j].fd) < 0 || i == j) {
									continue;
								}
								else {
									write(clilist[j].fd, buf, strlen(buf));
								}
							}

							// server log
							printf("* client connected from %s\n", clilist[i].ip_port);
							break;
						}
					}
				}
			}
			for (int i = 0; i < climax + 1; i++) {
				if ((sockfd = clilist[i].fd) < 0) {
					continue;
				}
				if (FD_ISSET(sockfd, &rset)) {
					bzero(buf, sizeof(buf));
					byte_read = read(sockfd, buf, 100);
					if (byte_read < 0) {
						printf("%s\n", strerror(errno));
					}
					else if (byte_read == 0) {
						bzero(buf, sizeof(buf));
						gettime(time);
						// announce other user
						sprintf(buf, "%-19s*** User <%s> has left the server\n", time, clilist[i].name);
						for (int j = 0; j < climax + 1; j++) {
							if ((clilist[j].fd) < 0 || i == j) {
								continue;
							}
							else {
								write(clilist[j].fd, buf, strlen(buf));
							}
						}

						// server log
						printf("* client %s disconnected\n", clilist[i].ip_port);

						clilist[i].fd = -1;
						FD_CLR(sockfd, &allset);
						numcli--;
						close(sockfd);
					}
					else {
						str_read = calloc(byte_read - 1, sizeof(char));
						strncpy(str_read, buf, byte_read - 1);
						char* to_parse = calloc(strlen(str_read) + 1, sizeof(char));
						strcpy(to_parse, str_read);
						parsed = strtok(to_parse, del);

						if (byte_read - 1 == 0) {
							continue;
						}
						if (strcmp(parsed, "/name") == 0) {
							parsed = strtok(NULL, del);
							if (!parsed) {
								gettime(time);
								sprintf(buf, "%-19s*** Unknown or incomplete command <%s>\n", time, str_read);
								write(clilist[i].fd, buf, strlen(buf));
								continue;
							}

							gettime(time);
							// broadcast that name changed to other user
							sprintf(buf, "%-19s*** User <%s> renamed to <%s>\n", time, clilist[i].name, parsed);
							for (int j = 0; j < climax + 1; j++) {
								if ((clilist[j].fd) < 0 || i == j) {
									continue;
								}
								else {
									write(clilist[j].fd, buf, strlen(buf));
								}
							}

							bzero(buf, sizeof(buf));
							// announce the user that name has changed
							sprintf(buf, "%-19s*** Nickname changed to <%s>\n", time, parsed);
							write(clilist[i].fd, buf, strlen(buf));

							// change name
							bzero(clilist[i].name, 50);
							strcpy(clilist[i].name, parsed);
						}
						else if (strcmp(parsed, "/who") == 0) {
							bzero(buf, sizeof(buf));
							strcpy(buf, "--------------------------------------------------\n");
							write(clilist[i].fd, buf, strlen(buf));
							for (int j = 0; j < climax + 1; j++) {
								bzero(buf, sizeof(buf));
								if ((clilist[j].fd) < 0) {
									continue;
								}
								else if (i == j) {
									sprintf(buf, "* %-21s%s\n",clilist[j].name, clilist[j].ip_port);
									write(clilist[i].fd, buf, strlen(buf));
								}
								else {
									sprintf(buf, "  %-21s%s\n",clilist[j].name, clilist[j].ip_port);
									write(clilist[i].fd, buf, strlen(buf));
								}
							}
							bzero(buf, sizeof(buf));
							strcpy(buf, "--------------------------------------------------\n");
							write(clilist[i].fd, buf, strlen(buf));
						}
						else if (parsed[0] == '/') {
							gettime(time);
							sprintf(buf, "%-19s*** Unknown or incomplete command <%s>\n", time, str_read);
							write(clilist[i].fd, buf, strlen(buf));
						}
						else {
							bzero(buf, sizeof(buf));
							gettime(time);
							// broadcast message to other user
							sprintf(buf, "%-19s<%s> %s\n", time, clilist[i].name, str_read);
							for (int j = 0; j < climax + 1; j++) {
								if ((clilist[j].fd) < 0 || i == j) {
									continue;
								}
								else {
									write(clilist[j].fd, buf, strlen(buf));
								}
							}
						}
					}
				}
			}
		}
	}
}