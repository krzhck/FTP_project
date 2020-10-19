#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include "util.h"
#include "global.h"
#include "data_structure.h"

int INVALID_Handler(struct ThreadParam* data) {
	// TODO
	return 0;
}

int USER_Handler(struct ThreadParam* data) {
	// anonymous
	if (strcmp(data->request.arg, "anonymous") == 0) {
		printf("USER anonymous received, connfd = %d\n", data->connfd);
		data->clientState = HAS_USER;
		char responseStr[RESPONSE_LENGTH] = "331 USER ok, PASS please.\r\n";
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	else {
		printf("Error username(): %s(%d)\n", strerror(errno), errno);
		char responseStr[RESPONSE_LENGTH] = "530 USER needs to be anonymous.\r\n";
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	return 0;
};

int PASS_Handler(struct ThreadParam* data) {
	switch (data->clientState) {
		case NO_USER:
			printf("Error pass(): %s(%d)\n", strerror(errno), errno);
			char responseStr[RESPONSE_LENGTH] = "503 needs USER first.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
		case HAS_USER:
			printf("password received, connfd = %d\n", data->connfd);
			char responseStr[RESPONSE_LENGTH] = "230 login successful, enjoy the server.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
		case HAS_PASS:
		default:
			printf("already logged in, connfd = %d\n", data->connfd);
			char responseStr[RESPONSE_LENGTH] = "530 already logged in.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
	}
	return 0;
};

// connect()
int PORT_Handler(struct ThreadParam* data) {
	if (ParseIPPort(&(data->clientAddr)) {
		data->dataConnectionMode = PORT_MODE; 
	}
};

// accept()
int PASV_Handler(struct ThreadParam* data) {
	// close current connections
	CloseConnection(data->datafd);
	CloseConnection(data->listenfd);
	data->datafd = -1;
	data->listenfd = -1;

	data->dataConnectionMode = PASV_MODE;
	data->dataPort = RandomPort();

	struct sockaddr_in addr;
	if ((data->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		char responseStr[RESPONSE_LENGTH] = "425 PASV command failed.";
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(data->dataPort);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (inet_pton(AF_INET, serverIP, &addr.sin_addr) == -1) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
	}

	if (bind(data->listenfd,(struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
	}

	if (listen(data->listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
	}

	char addrStr[30];
	strcpy(addrStr, AddrToString());
	char responseStr[RESPONSE_LENGTH];
	sprintf(responseStr, "227 passive mode (%s).\r\n", addrStr);

	return WriteResponse(data->connfd, strlen(responseStr), responseStr);

PASV_failed:
	char responseStr[RESPONSE_LENGTH] = "425 PASV command failed.";
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int RETR_Handler(struct ThreadParam* data) {

};

int STOR_Handler(struct ThreadParam* data) {

};

int QUIT_Handler(struct ThreadParam* data) {
	printf("quit, connfd = %d\n", data->connfd);
	char responseStr[RESPONSE_LENGTH] = "221 connection closed, goodbye.\r\n";
	CloseConnection(data->connfd);
	CloseConnection(data->datafd);
	data->connfd = -1;
	data->datafd = -1;
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int SYST_Handler(struct ThreadParam* data) {
	printf("syst command received, connfd = %d\n", data->connfd);
	char responseStr[RESPONSE_LENGTH] = "215 UNIX Type: L8\r\n";
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int TYPE_Handler(struct ThreadParam* data) {

};

int MKD_Handler(struct ThreadParam* data) {

};

int CWD_Handler(struct ThreadParam* data) {

};

int PWD_Handler(struct ThreadParam* data) {

};

int LIST_Handler(struct ThreadParam* data) {

};

int RMD_Handler(struct ThreadParam* data) {

};

int RNFR_Handler(struct ThreadParam* data) {

};

int RNTO_Handler(struct ThreadParam* data) {

};