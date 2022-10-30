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

void sig_chld(int signo) {
	pid_t pid;
	int stat;
	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

int haha(int connfd, char* cmd) {
    system(cmd);
    return 0;
}

int main(int argc, char **argv) {
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, 50);

	signal(SIGCHLD, sig_chld);

    char cliaddr_str[1000] = "";

	char cmd[2000] = "";
    for (int i = 0; i < argc - 2; i++) {
        strcat(cmd, argv[i + 2]);
        strcat(cmd, " ");
    }

	while (1) {
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

        inet_ntop(AF_INET, &cliaddr, cliaddr_str, INET_ADDRSTRLEN);

        // printf("connection\n");
        printf("New connection from %s:%s\n", cliaddr_str, argv[1]);

		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			
			int ret = access(cmd, X_OK);
            if(ret < 0 ){
                printf("%s is not executable\n", cmd);
                exit(0);
            }

            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDERR_FILENO);
			haha(connfd, cmd);	/* process the request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}
}