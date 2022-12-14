#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#define SA struct sockaddr
#define sink_num 100
#define MAXMSG 10000
using namespace std;

time_t cur_tm;
fd_set rset, master;
int maxfd, listenfd, listenfd2, nbytes ,TotalBytes, n;
char cur_time[19], name_list[1024][20], ip_list[1024][20];
char buf[MAXMSG], tmp[MAXMSG];

void timer();
void SEND_resp(int connfd);
void sig_pipe(int handler);

int main(int argc, char **argv)
{
    int i, j, maxi, connfd, connfd2, sockfd, cnt;
    int nready, client[50], visited[50], clilist[sink_num];
    int client_nums = 0;
    int port = atoi(argv[1]);
    ssize_t n;
    string cmd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr, servaddr2 ;
    struct timeval  rs;
    struct timeval  tv;
    double time_in_micros_tv, time_in_micros_rs, elapsed_time, measured_megabitsPs;
    signal(SIGPIPE, sig_pipe);

    for(int i = 0; i < sink_num; i++){
        clilist[i] = -1;
    }
    //serv 1
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0){
        puts("bind successfully");
    }
    else{
        perror("bind error:");
    }
    if (listen(listenfd, 2000) == 0){
        puts("listen successfully");
    }
    //serv2
    listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr2, sizeof(servaddr2));
    servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr2.sin_port = htons(port + 1);
    if (bind(listenfd2, (struct sockaddr *)&servaddr2, sizeof(servaddr2)) == 0){
        puts("bind successfully");
    }
    else{
        perror("bind error:");
    }
    if (listen(listenfd2, 2000) == 0){
        puts("listen successfully");
    }

    maxfd = listenfd; 
    for (i = 0; i < sink_num; i++)
        client[i] = -1; /* -1 indicates available entry */
    FD_ZERO(&master);
    FD_ZERO(&rset);
    FD_SET(listenfd, &master);
    FD_SET(listenfd2, &master);
    memset(visited, 0, sizeof(visited));

    for (;;)
    {
        rset = master;

        if (select(500, &rset, NULL, NULL, NULL) == -1){

            perror("select");
            exit(4);
        }
        if (FD_ISSET(listenfd, &rset)){

            connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
            FD_SET(connfd, &master);
            if (connfd > maxfd){
                maxfd = connfd;
            }
        }
        else if(FD_ISSET(listenfd2, &rset)){
            connfd2 = accept(listenfd2, (SA *)&cliaddr, &clilen);
            for(int i = 0; i < sink_num ; i++){
                if(clilist[i] == -1){
                    clilist[i] = connfd2;
                    FD_SET(connfd2, &master);
                    break;
                }
            }
        }
        else if(FD_ISSET(connfd, &rset)){
            if ((n = recv(connfd, buf, sizeof(buf), MSG_NOSIGNAL)) <= 0){
                close(connfd);
                FD_CLR(connfd, &master);
            }    
            else{
                if (strncmp(buf, "/reset", 6) == 0){
                    printf("reset\n");
                    memset(tmp, '\0', sizeof(tmp));
                    gettimeofday(&rs, NULL);
                    time_in_micros_rs =  rs.tv_sec + 0.000001 * rs.tv_usec;
                    sprintf(tmp, "%f RESET %d\n", time_in_micros_rs, TotalBytes);
                    nbytes = 0;
                    TotalBytes = 0;
                    SEND_resp(connfd);
                }
                else if(strncmp(buf, "/ping", 5) == 0){
                    printf("ping\n");
                    memset(tmp, '\0', sizeof(tmp));
                    gettimeofday(&tv, NULL);
                    time_in_micros_tv  = tv.tv_sec + 0.000001 *tv.tv_usec;
                    sprintf(tmp, "%f PONG\n", time_in_micros_tv);
                    SEND_resp(connfd);
                }
                else if(strncmp(buf, "/report", 7) == 0){
                    printf("report\n");
                    memset(tmp, '\0', sizeof(tmp));
                    gettimeofday(&tv, NULL);
                    time_in_micros_tv = tv.tv_sec + 0.000001 *tv.tv_usec;
                    elapsed_time = time_in_micros_tv - time_in_micros_rs;
                    printf("%d", TotalBytes);
                    measured_megabitsPs = 8.0 * TotalBytes /10000000 / elapsed_time;
                    sprintf(tmp, "%f REPORT %d %f %f\n", time_in_micros_tv, TotalBytes,elapsed_time, measured_megabitsPs);
                    SEND_resp(connfd);
                }                               
                else if(strncmp(buf, "/clients", 8) == 0){
                    printf("client\n");
                    memset(tmp, '\0', sizeof(tmp));
                    gettimeofday(&tv, NULL);
                    time_in_micros_tv = tv.tv_sec + 0.000001 *tv.tv_usec;
                    for(int i=0 ; i < sink_num; i++){
                        if(clilist[i] != -1){
                            cnt ++;
                        }
                    }
                    sprintf(tmp, "%f CLIENTS %d\n", time_in_micros_tv, cnt);
                    SEND_resp(connfd);
                }
            }
        }
        else {
            for(int i=0; i < sink_num ;i++){
                if(clilist[i] < 0){
                    continue;
                }
                else if(FD_ISSET(clilist[i], &rset)){
                    nbytes = recv(clilist[i], buf, sizeof(buf), MSG_NOSIGNAL);    
                    TotalBytes += nbytes; 
                    printf("%s", buf);
                    printf("%d", TotalBytes);
                    if(nbytes <= 0 ){
                        close(connfd2);
                    }
                }
            }
        }
    }
    return 0;

}
void sig_pipe(int handler){
    return;
}
void SEND_resp(int connfd){
    if(send(connfd, tmp, 100, 0 ) < 0){
        printf("Response failed.\n");
    }
    else{
        printf("%s", tmp);
    }
    memset(buf, '\0', sizeof(tmp));    
    return;
}