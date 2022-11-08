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

void response(char* buf, char *code, char* name, char* param) {
	char temp[500];
	bzero(temp, sizeof(temp));
	if (param == NULL && name == NULL) { // name == NULL means this is not known for an associated connection
		sprintf(temp, ":hehe %s\r\n", code);
	}
	else if (name == NULL) {
		sprintf(temp, ":hehe %s %s\r\n", code, param);
	}
	else if (param == NULL) {
		sprintf(temp, ":hehe %s %s\r\n", code, name);
	}
	else {
		sprintf(temp, ":hehe %s %s %s\r\n", code, name, param);
	}
	strcat(buf, temp);
}

int main() {
    char haha[500];
    bzero(haha, sizeof(haha));

    response(haha, "333", "name", "some");
    response(haha, "999", "name", "papaparam");

	printf("%s", haha);

}
