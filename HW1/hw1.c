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

struct client{
	int			fd;
	char 		name[50];
	char		ip_port[30]; // only ip now
	char 		channel[50];
};

void initial_client(struct client *client) {
	client->fd = -1;
	bzero(client->channel, sizeof(client->channel));
	strcpy(client->channel, "");
	bzero(client->name, sizeof(client->name));
	strcpy(client->name, "");
	bzero(client->ip_port, sizeof(client->ip_port));
	strcpy(client->ip_port, "");
}

struct channel{
	int			inuse;
	char		name[50]; // "NOT" prefixed with #
	char		topic[50];
	int			people;
};

void initial_channel(struct channel* channel) {
	channel->inuse = 0;
	bzero(channel->name, sizeof(channel->name));
	strcpy(channel->name, "");
	channel->people = 0;
	bzero(channel->topic, sizeof(channel->topic));
	strcpy(channel->topic, "");
}

void printbyte(char* str) {
	for (int i = 0; i < sizeof(str) + 3; i++) {
		printf("%d ", str[i]);
	}
	printf("\n");
}

void de_r_n(char *str) {
	for (int i = sizeof(str) + 3; i >= 0; i--) {
		if (str[i] == '\r' || str[i] == '\n') {
			str[i] = '\0';
		}
		else if (str[i] != 0) {
			break;
		}
	}
}

void de_sharp(char *str) {
	if (str[0] == '#') {
		int temp = strlen(str) - 1 + 1;
		for (int i = 0; i < temp; i++) {
			str[i] = str[i + 1];
		}
	}
}

void response(char* buf, char *code, char* name, char* param) {
	char temp[500];
	bzero(temp, sizeof(temp));
	if (param == NULL && name == NULL) { // name == NULL means this is not known for an associated connection
		sprintf(temp, ":hehe %s\n", code);
	}
	else if (name == NULL) {
		sprintf(temp, ":hehe %s %s\n", code, param);
	}
	else if (param == NULL) {
		sprintf(temp, ":hehe %s %s\n", code, name);
	}
	else {
		sprintf(temp, ":hehe %s %s %s\n", code, name, param);
	}
	strcat(buf, temp);
}

void motd(struct client *cli, char *buf) {
	memset(buf, 0, sizeof(buf));
	response(buf, "001", cli->name, ":Welcome to hehe!");
	response(buf, "375", cli->name, ":- MOTD -");
	response(buf, "372", cli->name, ": /   Why         so         ?  \\");
	response(buf, "372", cli->name, ": \\        you        weak      / ");
	response(buf, "372", cli->name, ": --------------------------------");
	response(buf, "372", cli->name, ":        \\   ,__,          ");
	response(buf, "372", cli->name, ":         \\  (oo)____      ");	
	response(buf, "372", cli->name, ":            (__)    )\\    ");
	response(buf, "372", cli->name, ":               ||--|| * ");
	response(buf, "376", cli->name, ":- End of MOTD -");
}

void channel_list(struct channel *cha, struct client *cli, char* buf, char* param) {
	memset(buf, 0, sizeof(buf));
	char temp[500];
	response(buf, "321", cli->name, "- Channel list -");
	if (param == NULL) {
		for (int i = 0; i < 1000; i++) {
			if (cha[i].inuse) {
				bzero(temp, sizeof(temp));
				sprintf(temp, "%s %d :%s", cha[i].name, cha[i].people, cha[i].topic);
				response(buf, "322", cli->name, temp);
			}
		}
	}
	else {
		for (int i = 0; i < 1000; i++) {
			if (strncmp(cha[i].name, param, strlen(cha[i].name)) == 0 && cha[i].inuse) {
				bzero(temp, sizeof(temp));
				sprintf(temp, "%s %d :%s", cha[i].name, cha[i].people, cha[i].topic);
				response(buf, "322", cli->name, temp);
				break;
			}
		}
	}
	response(buf, "323", cli->name, "- End of channel list -");
}

void join(struct channel *cha, struct client *cli, char* buf, char* param) { // param: channel name
	memset(buf, 0, sizeof(buf));
	// topic
	char temp[300], topic[300], code[10];
	if (strcmp(cha->topic, "") == 0) {
		strcpy(topic, "No topic is set");
		strcpy(code, "331");
	}
	else {
		strcpy(topic, cha->topic);
		strcpy(code, "332");
	}
	bzero(temp, sizeof(temp));
	sprintf(temp, "#%s :%s", param, topic);
	response(buf, code, cli->name, temp);

	// users
	int flag = 0;
	bzero(temp, sizeof(temp));
	sprintf(temp, "#%s :", param);
	for (int i = 0; i < 1000; i++) {
		if (strncmp(cli[i].channel, param, strlen(param)) == 0) {
			strcat(temp, cli[i].name);
			strcat(temp, " ");
			flag = 1;
		}
	}
	if (flag)
		response(buf, "353", cli->name, temp);

	// end of users
	bzero(temp, sizeof(temp));
	sprintf(temp, "#%s :End of Names List", param);
	response(buf, "366", cli->name, temp);
}

void users(struct client *clilist, struct client *cli, char *buf) {
	bzero(buf, sizeof(buf));
	char temp[100];
	// char term[20];
	bzero(temp, sizeof(temp));
	// bzero(term, sizeof(term));
	// strcpy(term, "Terminal");
	sprintf(temp, ":- %-19s %-10s %s -", "UserID", "Terminal", "Host");
	response(buf, "392", cli->name, temp);
	for (int i = 0; i < 1000; i++) {
		if (clilist[i].fd != -1) {
			bzero(temp, sizeof(temp));
			sprintf(temp, ":%-20s -          %s", clilist[i].name, clilist[i].ip_port);
			response(buf, "393", cli->name, temp);
		}
	}
	response(buf, "394", cli->name, ":- End of users -");
}

void names(struct client *clilist, struct client *cli, struct channel *cha, char *buf) {
	bzero(buf, sizeof(buf));
	char temp[200];
	printf("cha people: %d\n", cha->people);
	if (cha->people > 0) {
		// get client list string
		char people_string[100];
		bzero(people_string, sizeof(people_string));
		for (int i = 0; i < 1000; i++) {
			if (strcmp(clilist[i].channel, cha->name) == 0) {
				strcat(people_string, clilist[i].name);
				strcat(people_string, " ");
			}
		}
		bzero(temp, sizeof(temp));
		sprintf(temp, "#%s :%s", cha->name, people_string);
		response(buf, "355", cli->name, temp);
	}
	bzero(temp, sizeof(temp));
	sprintf(temp, "#%s :End of Names List", cha->name);
	response(buf, "366", cli->name, temp);
}

int main(int argc, char **argv) {
	int					listenfd, newfd, sockfd;
	int	 				optval;
	int 				fdmax, nready;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	struct client		clilist[1000];
	int 				numcli, climax;
	struct channel		chalist[1000];
	int					numcha, chamax;
	char				buf[500];
	char				temp[500];
	char				time[21];
	char*				str_read;
	char* 				del = " ";
	char* 				parsed;
	int					byte_read;
	fd_set 				rset, allset;
	struct timeval 		waittime = {0, 0}; // 0.1s?

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	optval = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
		perror("setsockopt");
		exit(-1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) {
		perror("setsockopt");
		exit(-1);
	}

	listen(listenfd, 50);

	signal(SIGPIPE, handle);

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	fdmax = listenfd + 1;

	// initial client list
	for (int i = 0; i < 1000; i++) {
		initial_client(&clilist[i]);
	}
	numcli = 0;
	climax = 0;

	// initialize channel list
	for (int i = 0; i < 1000; i++) {
		initial_channel(&chalist[i]);
	}
	numcha = 0;
	chamax = -1;

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
							sprintf(clilist[i].ip_port, "%s", inet_ntoa(cliaddr.sin_addr));

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
							de_r_n(p);
							strcpy(clilist[i].name, p);
							// USER <username> <hostname> <servername> <realname> (not used)
							bzero(buf, sizeof(buf));
							read(newfd, buf, 100); 

							// motd
							motd(&clilist[i], buf);
							write(newfd, buf, strlen(buf));

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
					// client disconnected
					else if (byte_read == 0) {
						// server log
						printf("* client %s disconnected\n", clilist[i].ip_port);

						clilist[i].fd = -1;
						FD_CLR(sockfd, &allset);
						numcli--;
						close(sockfd);
					}
					else {
						char str_read[100];
						strcpy(str_read, buf);
						de_r_n(str_read);
						char* to_parse = calloc(strlen(str_read) + 1, sizeof(char));
						strcpy(to_parse, str_read);
						parsed = strtok(to_parse, del);
						de_r_n(parsed);

						if (byte_read - 1 == 0) {
							continue;
						}

						printf("%s", str_read);

						// read commands
						if (strncmp(parsed, "NICK", 4) == 0) {
							parsed = strtok(NULL, del);
							bzero(clilist[i].name, sizeof(clilist[i].name));
							strcpy(clilist[i].name, parsed);
							printf("changed cli %d nick\n", i);
							response(buf, "001", parsed, "new nick!");
						}
						else if (strncmp(parsed, "PING", 4) == 0) {
							parsed = strtok(NULL, del);
							bzero(buf, sizeof(buf));
							sprintf(buf, "PONG %s\n", parsed);
							write(sockfd, buf, sizeof(buf));
						}
						else if (strncmp(parsed, "LIST", 4) == 0) {
							parsed = strtok(NULL, del);
							bzero(buf, sizeof(buf));
							channel_list(&chalist[0], &clilist[i], buf, parsed);
							write(sockfd, buf, strlen(buf));
							printf("%s", buf);
						}
						else if (strcmp(parsed, "JOIN") == 0) {
							parsed = strtok(NULL, del);
							de_r_n(parsed);
							de_sharp(parsed);

							// :cli JOIN #cha
							bzero(buf, sizeof(buf));
							sprintf(buf, ":%s JOIN #%s\n", clilist[i].name, parsed);
							char pre[500];
							bzero(pre, sizeof(pre));
							strcpy(pre, buf);

							int channel_index = -1;
							// find if channel exist
							for (int j = 0; j < 1000; j++) {
								if ((strcmp(chalist[j].name, parsed) == 0) && (chalist[j].inuse == 1)) {
									channel_index = j;
									bzero(clilist[i].channel, sizeof(clilist[i].channel));
									strcpy(clilist[i].channel, parsed);
									printf("go to previously created channel!\n");
									break;
								}
							}
							// create if not exist
							if (channel_index == -1) { // not found
								printf("create new channel!\n");
								for (int j = 0; j < 1000; j++) {
									if (chalist[j].inuse == 0) {
										initial_channel(&chalist[j]);
										if (j > chamax) {
											chamax = j;
										}
										numcha++;
										channel_index = j;
										chalist[j].inuse = 1;
										strcpy(chalist[j].name, parsed);

										bzero(clilist[i].channel, sizeof(clilist[i].channel));
										strcpy(clilist[i].channel, parsed);
										break;
									}
								}
							}
							chalist[channel_index].people++;
							join(&chalist[channel_index], clilist, buf, parsed);
							strcat(pre, buf);
							write(sockfd, pre, strlen(pre));
							printf("%s", pre);
						}
						else if (strcmp(parsed, "TOPIC") == 0) {
							// get <channel> [topic]
							parsed = strtok(NULL, del);
							de_r_n(parsed);
							char cha[50];
							bzero(cha, sizeof(cha));
							strcpy(cha, parsed);
							de_r_n(cha);
							de_sharp(cha);
							parsed = strtok(NULL, del);

							// 332 RPL_TOPIC
							if (parsed != NULL) { // change topic of the channel the user is present
								char *tpc = str_read + 5 + 1 + 1 + strlen(cha) + 1 + 1;
								de_r_n(tpc);
								for (int k = 0; k < 1000; k++) {
									if (chalist[k].inuse && strcmp(cha, chalist[k].name) == 0) {
										strcpy(chalist[k].topic, tpc);
									}
								}
								char temp[50];
								bzero(temp, sizeof(temp));
								sprintf(temp, "#%s :%s", cha, tpc);
								response(buf, "332", clilist[i].name, temp);
							}
							else {
								for (int k = 0; k < 1000; k++) {
									if (chalist[k].inuse && strcmp(cha, chalist[k].name) == 0) {
										char tpc_prm[100], tpccode[10];
										bzero(tpc_prm, sizeof(tpc_prm));
										bzero(buf, sizeof(buf));
										if (strcmp(chalist[k].topic, "") == 0) {
											sprintf(tpc_prm, "#%s :No topic is set", chalist[k].name);
											strcpy(tpccode, "331");
										}
										else {
											sprintf(tpc_prm, "#%s :%s", chalist[k].name, chalist[k].topic);
											strcpy(tpccode, "332");
										}
										de_r_n(tpc_prm);
										response(buf, tpccode, clilist[i].name, tpc_prm);
										write(clilist[i].fd, buf, strlen(buf));
									}
								}
							}
						}
						else if (strcmp(parsed, "NAMES") == 0) {
							parsed = strtok(NULL, del); // channel name
							de_r_n(parsed);
							de_sharp(parsed); 
							// find out the index of the channel the user is currently in
							int channel_index;
							for (int j = 0; j < 1000; j++) {
								if (strcmp(chalist[j].name, parsed) == 0) {
									channel_index = j;
									break;
								}
							}
 							names(clilist, &clilist[i], &chalist[channel_index], buf);
							write(sockfd, buf, strlen(buf));
						}
						else if (strcmp(parsed, "PART") == 0) {
							// get channel name
							parsed = strtok(NULL, del);
							if (parsed != NULL) {
								response(buf, "411", clilist[i].name, ":No recipient given (PRIVMSG)");
							}
							// announce user
							bzero(buf, sizeof(buf));
							sprintf(buf, ":%s PART :#%s\n", clilist[i].name, clilist[i].channel);
							write(clilist[i].fd, buf, strlen(buf));
							// find that channel
							for (int j = 0; j < 1000; j++) {
								if (strcmp(parsed, chalist[j].name) == 0) {
									// people decrement
									chalist[j].people--;
									break;
								}
							}
							// clear client's channel info
							bzero(clilist[i].channel, sizeof(clilist[i].channel));
							strcpy(clilist[i].channel, "");
						}
						else if (strncmp(parsed, "USERS", 5) == 0) {
							users(clilist, &clilist[i], buf);
							write(sockfd, buf, strlen(buf));
							printf("%s", buf);
						}
						else if (strcmp(parsed, "PRIVMSG") == 0) {
							// get channel name
							parsed = strtok(NULL, del);
							de_r_n(parsed);
							char cha[50];
							bzero(cha, sizeof(cha));
							strcpy(cha, parsed);
							de_r_n(cha);
							de_sharp(cha);
							// get message
							parsed = strtok(NULL, del);
							char *msg = str_read + 7 + 1 + 1 + strlen(cha) + 1 + 1;
							if (parsed != NULL) { // change topic of the channel the user is present
								de_r_n(msg);
								char temp[50];
							}
							// send to all users beside the sender
							char temp[500];
							bzero(temp, sizeof(temp));
							sprintf(temp, ":%s PRIVMSG #%s :%s\n", clilist[i].name, cha, msg);
							for (int j = 0; j < 1000; j++) {
								if (strcmp(clilist[j].channel, cha) == 0 && j != i) {
									write(clilist[j].fd, temp, strlen(temp));
									printf("write to %s\n", clilist[j].name);
									printf("%s", temp);
								}
							}
						}
						else if (strcmp(parsed, "QUIT") == 0) {
							// find that channel
							for (int j = 0; j < 1000; j++) {
								if (strcmp(clilist[i].channel, chalist[j].name) == 0) {
									// people decrement
									chalist[j].people--;
									break;
								}
							}
							FD_CLR(clilist[i].fd, &allset);
							numcli--;
							close(clilist[i].fd);
							// clear client's channel info
							initial_channel(&clilist[i]);
						}
						else {
							char t[100];
							bzero(t, sizeof(t));
							sprintf(t, "%s :Unknown command", parsed);
							bzero(buf, sizeof(buf));
							response(buf, "421", clilist[i].name, t);
							write(clilist[i].fd, buf, strlen(buf));
						}
					}
				}
			}
		}
	}
}