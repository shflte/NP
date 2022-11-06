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

void handle(int signal) {
	// printf("broken pipe;(\n");
	return;
}

void sigint(int signal, int fd) {
	close(4);
	printf("close\n");
	return;
}

struct client{
	int			fd;
	char 		name[50];
	char		ip_port[30];
	int			state; // 0 for lobby, others for channel
};

struct channel{
	int			id;
	char		name[50];
	char		topic[50];

};

void response(int fd, char *code, char* name, char* param) {
	char buf[500];
	bzero(buf, sizeof(buf));
	if (param == NULL && name == NULL) { // name == NULL means this is not known for an associated connection
		sprintf(buf, ":hehe %s", code);
	}
	else if (name == NULL) {
		sprintf(buf, ":hehe %s %s", code, param);
	}
	else if (param == NULL) {
		sprintf(buf, ":hehe %s %s", code, name);
	}
	else {
		sprintf(buf, ":hehe %s %s %s", code, name, param);
	}
	write(fd, buf, sizeof(buf));
	printf("%s\n", buf);
}

// void motd(int fd, struct client *cli) {
// 	char buf[500];
// 	bzero(buf, sizeof(buf));
// 	response(fd, "001", cli->name, ":Welcome to hehe!");
// 	// response(fd, "251", cli->name, ":There are 7 users and 0 invisible on 1 server");
// 	response(fd, "375", cli->name, ":- MOTD -");
// 	response(fd, "372", cli->name, ":- /   Why         so          ? \\ ");
// 	response(fd, "372", cli->name, ":- \\        you        weak      / ");
// 	response(fd, "372", cli->name, ":- --------------------------------");
// 	response(fd, "372", cli->name, ":-       \\   ,__,          ");
// 	response(fd, "372", cli->name, ":-        \\  (oo)____      ");	
// 	response(fd, "372", cli->name, ":-           (__)    )\\    ");
// 	response(fd, "372", cli->name, ":-              ||--|| * ");
// 	response(fd, "376", cli->name, ":End of MOTD");
// }

void motd(int fd, struct client *cli) {
	write(fd, ":mircd 001 shflte :Welcome to the minimized IRC daemon!\n", 200);
	write(fd, ":mircd 375 shflte :- mircd Message of the day -", 200);
	write(fd, ":mircd 372 shflte :-  Hello, World!", 200);
	write(fd, ":mircd 376 shflte :End of message of the day", 200);
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
	signal(SIGINT, sigint);

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

							// server log
							printf("* client connected from %s\n", clilist[i].ip_port);

							// CAP LS XXX (not used)
							bzero(buf, sizeof(buf));
							read(newfd, buf, 100); 
							// NICK <nickname>
							bzero(buf, sizeof(buf));
							read(newfd, buf, 100); 
							char* p = strtok(buf, " ");
							p = strtok(NULL, " ");
							p[strlen(p) - 1] = '\0';
							strcpy(clilist[i].name, p);
							// USER <username> <hostname> <servername> <realname> (not used)
							bzero(buf, sizeof(buf));
							read(newfd, buf, 100); 

							// motd
							motd(newfd, &clilist[i]);

							printf("name: %s\n", clilist[i].name);

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
						/*
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
						*/
					}
				}
			}
		}
	}
}