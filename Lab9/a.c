#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdbool.h>

#define s_recv_len 10000
#define s_send_len 10

char maze[9][9], table[9][9];
char recv_msg[10000];
char send_msg[10];

void recv_table(char *recv_msg){
    int j = 0, k = 0;
    
    for (int i = 0; i < 81; i++) {
        recv_msg[i] = recv_msg[i + 4];
    }

    for (int i = 0; i < 9; i++){
        for (int j = 0; j < 9; j++) {
            table[i][j] = maze[i][j] = recv_msg[i * 9 + j];
        }
    }
}

bool isValid(int r, int c, char n) {
    for( int i = 0; i< 9 ;i++){
        if(maze[r][i] == n) {
            return false;
        }
        if(maze[i][c] == n){
            return false;
        }
        if(maze[(r/3)*3 + i/3][(c/3)*3 + i%3] == n){
            return false;
        }
        
    }
    return true;
}
bool backtrack(int i, int j) {
    if (j == 9) {
        return backtrack( i + 1, 0);
    }
    if (i == 9) {
        return true;
    }
    if( maze[i][j] != '.'){
        return backtrack(i, j + 1);
    }
    
    for (char ch = '1'; ch <= '9'; ch++){
        if (!isValid(i, j, ch)){
            continue;
        }
        maze[i][j] = ch;
        if( backtrack(i, j + 1)){
            return true;
        }
        maze[i][j] = '.';
    }
    return false;

}

int main() {
	int sock = 0;
	int data_len = 0;
	struct sockaddr_un remote;

	if( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1  )
	{
		printf("Client: Error on socket() call \n");
		return 1;
	}

	remote.sun_family = AF_UNIX;
	strcpy( remote.sun_path,  "/sudoku.sock" );
	data_len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    connect(sock, (struct sockaddr*)&remote, data_len);
    sprintf(send_msg, "S\n");
    send(sock, send_msg, s_send_len, 0);
    recv(sock, recv_msg, s_recv_len, 0);
    recv_table(recv_msg);

    bool flag = true;
    for (int i = 0; i < 81 && flag == true; i++) {
        int r = i / 9;
        int c = i % 9;

        flag = true;
        if(maze[r][c] == '.'){
            flag = false;
            backtrack(0, 0);
            // continue;
        }
    }

    for (int i = 0; i < 9; i++){
        for (int j = 0; j < 9; j++){
            if (table[i][j]=='.'){
                bzero(send_msg, sizeof(send_msg));
                sprintf(send_msg, "V %d %d %c\n", i, j, maze[i][j]);
                send(sock, send_msg, sizeof(send_msg), 0);
                
                bzero(recv_msg, sizeof(recv_msg));
                recv(sock, recv_msg, s_recv_len, 0);
            }
        }
    }
    bzero(recv_msg, sizeof(recv_msg));
    sprintf(send_msg, "C\n");
    send(sock, send_msg, s_send_len, 0);

	bzero(send_msg, sizeof(send_msg));
    recv(sock, recv_msg, s_recv_len, 0);

	printf("Done \n");

	return 0;
}
