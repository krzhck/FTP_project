#ifndef CONMMAND_HANDLERS
#define CONMMAND_HANDLERS

#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>

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
	// TODO maybe
	char responseStr[RESPONSE_LENGTH] = {0};
	printf("invalid command.\n");
	strcpy(responseStr, "500 invalid command.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
}

int USER_Handler(struct ThreadParam* data) {
	// anonymous
	//printf("enter user_handler\n");
	//fflush(stdout);
	printf("request arg = %s\n", data->request.arg);
	printf("arg length = %ld\n", strlen(data->request.arg));
	//fflush(stdout);
	char responseStr[RESPONSE_LENGTH] = {0};
	if (!strcmp(data->request.arg, "anonymous")) {
		printf("USER anonymous received, connfd = %d\n", data->connfd);
		data->clientState = HAS_USER;
		// char responseStr[RESPONSE_LENGTH] = "331 USER ok, PASS please.\r\n";
		strcpy(responseStr, "331 USER ok, PASS please.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	else {
		printf("Error username(): %s(%d)\n", strerror(errno), errno);
		// char responseStr[RESPONSE_LENGTH] = "530 USER needs to be anonymous.\r\n";
		strcpy(responseStr, "530 USER needs to be anonymous.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
};

int PASS_Handler(struct ThreadParam* data) {
	printf("enter PASS, password = %s.\n", data->request.arg);
	char responseStr[RESPONSE_LENGTH] = {0};
	switch (data->clientState) {
		case NO_USER:
			printf("Error pass(): %s(%d)\n", strerror(errno), errno);
			strcpy(responseStr, "503 needs USER first.\r\n");
			// char responseStr[RESPONSE_LENGTH] = "503 needs USER first.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
		case HAS_USER:
			printf("password received, connfd = %d\n", data->connfd);
			strcpy(responseStr, "230 login successful, enjoy the server.\r\n");
			data->clientState = HAS_PASS;
			// char responseStr[RESPONSE_LENGTH] = "230 login successful, enjoy the server.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
		case HAS_PASS:
		default:
			printf("already logged in, connfd = %d\n", data->connfd);
			strcpy(responseStr, "530 already logged in.\r\n");
			// char responseStr[RESPONSE_LENGTH] = "530 already logged in.\r\n";
			return WriteResponse(data->connfd, strlen(responseStr), responseStr);
			break;
	}
};

// connect()
int PORT_Handler(struct ThreadParam* data) {
	// close current connections
	CloseConnection(data->datafd);
	CloseConnection(data->listenfd);
	data->datafd = -1;
	data->listenfd = -1;

	char responseStr[RESPONSE_LENGTH] = {0};
	if (ParseIPPort(data)) {
		printf("port mode on, connfd = %d\n", data->connfd);// s
		printf("client addr: IP = %s, port = %d\n", data->clientAddr.IP, data->clientAddr.port);
		data->dataConnectionMode = PORT_MODE;
		memset(&(data->dataAddr), 0, sizeof(data->dataAddr));
		data->dataAddr.sin_family = AF_INET;
		data->dataAddr.sin_port = htons(data->clientAddr.port);
		inet_pton(AF_INET, data->clientAddr.IP, &((data->dataAddr).sin_addr.s_addr));
		strcpy(responseStr, "200 port success.\r\n");
	}
	else {
		printf("Error ParseIPPort(): %s(%d)\n", strerror(errno), errno);
		strcpy(responseStr, "500 invalid arguments.\r\n");
	}
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

// accept()
int PASV_Handler(struct ThreadParam* data) {
	// close current connections
	CloseConnection(data->datafd);
	CloseConnection(data->listenfd);
	data->datafd = -1;
	data->listenfd = -1;


	char responseStr[RESPONSE_LENGTH] = {0};
	data->dataConnectionMode = PASV_MODE;
	data->dataPort = RandomPort();

	if ((data->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		goto PASV_failed;
	}

	memset(&(data->dataAddr), 0, sizeof(data->dataAddr));
	data->dataAddr.sin_family = AF_INET;
	//data->dataAddr.sin_port = htons(data->dataPort);
	data->dataAddr.sin_port = htons(RandomPort());
	//data->dataAddr.sin_port = htons(0);
	data->dataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, serverIP, &((data->dataAddr).sin_addr.s_addr));

	if (bind(data->listenfd, (struct sockaddr*)&(data->dataAddr), sizeof(data->dataAddr)) < 0) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		goto PASV_failed;
	}

	if (listen(data->listenfd, 1) < 0) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		goto PASV_failed;
	}

	printf("passive mode on, connfd = %d\n", data->connfd);

	socklen_t socklen = sizeof(struct sockaddr);
	if (getsockname(data->listenfd, (struct sockaddr *)&(data->dataAddr), &socklen) < 0) {
		printf("Error getsockname(): %s(%d)\n", strerror(errno), errno);
		goto PASV_failed;
    }
	int port = ntohs(data->dataAddr.sin_port);

	char addrStr[30];
	strcpy(addrStr, AddrToString(port));
	// char responseStr[RESPONSE_LENGTH];
	sprintf(responseStr, "227 passive mode (%s).\r\n", addrStr);
	printf("addr: %s\n", addrStr);
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);

PASV_failed:
	strcpy(responseStr, "425 PASV command failed.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

// write()
int RETR_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	if (data->dataConnectionMode == NO_CONNECTION) {
		printf("no connection, connfd = %d\n", data->connfd);
		strcpy(responseStr, "425 no data connection.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	char filePath[PATH_LENGTH] = {0};
	if (!AbsPath(filePath, rootPath, data->currDir, data->request.arg)) {
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	struct stat s = {0};
	stat(filePath, &s);
	if (!(s.st_mode & S_IFREG)) {
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	strcpy(responseStr, "150 RETR start.\r\n");
	if (!WriteResponse(data->connfd, strlen(responseStr), responseStr)) {
		return 0;
	}
	return WriteFile(data, filePath);
};

// read()
int STOR_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	if (data->dataConnectionMode == NO_CONNECTION) {
		printf("no connection, connfd = %d\n", data->connfd);
		strcpy(responseStr, "425 no data connection.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	char filePath[PATH_LENGTH] = {0};
	if (!AbsPath(filePath, rootPath, data->currDir, data->request.arg)) {
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	
	strcpy(responseStr, "150 STOR start.\r\n");
	if (!WriteResponse(data->connfd, strlen(responseStr), responseStr)) {
		return 0;
	}
	return ReadFile(data, filePath);
};

int QUIT_Handler(struct ThreadParam* data) {
	printf("quit, connfd = %d\n", data->connfd);
	char responseStr[RESPONSE_LENGTH] = "221 connection closed, goodbye.\r\n";
	WriteResponse(data->connfd, strlen(responseStr), responseStr);
	CloseConnection(data->connfd);
	CloseConnection(data->datafd);
	// data->connfd = -1;
	// data->datafd = -1;
	return 1;
};

int SYST_Handler(struct ThreadParam* data) {
	printf("syst command received, connfd = %d\n", data->connfd);
	char responseStr[RESPONSE_LENGTH] = "215 UNIX Type: L8\r\n";
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int TYPE_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	if (strcmp(data->request.arg, "I") == 0) {
		// char responseStr[RESPONSE_LENGTH] = "200 type set to I.\r\n";
		strcpy(responseStr, "200 Type set to I.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	// char responseStr[RESPONSE_LENGTH] = "425 invalid type.\r\n";
	strcpy(responseStr, "425 invalid type.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int MKD_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	if (MakeDir(data)) {
		// char responseStr[RESPONSE_LENGTH] = "250 MKD succeeded.\r\n";
		strcpy(responseStr, "250 MKD succeeded.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	// char responseStr[RESPONSE_LENGTH] = "550 MKD failed.\r\n";
	strcpy(responseStr, "550 MKD failed.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int CWD_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	if (ChangeDir(data)) {
		// char responseStr[RESPONSE_LENGTH] = "250 CWD succeeded.\r\n";
		strcpy(responseStr, "250 CWD succeeded.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	// char responseStr[RESPONSE_LENGTH] = "550 CWD failed.\r\n";
	strcpy(responseStr, "550 CWD failed.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int PWD_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	sprintf(responseStr, "257 \"%s\".\r\n", data->currDir);
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int LIST_Handler(struct ThreadParam* data) {
	// TODO add login authorization
	char responseStr[RESPONSE_LENGTH] = {0};
	char lsDir[PATH_LENGTH] = {0};

	if (!AbsPath(lsDir, rootPath, data->currDir, data->request.arg))
	{
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	printf("ls dir is %s\n", lsDir);

	// non exist
	struct stat s = {0};
	stat(lsDir, &s);
	if (!(s.st_mode & S_IFDIR))
	{
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	char lsTmpFile[PATH_LENGTH] = {0};
	strcpy(lsTmpFile, lsDir);
	char *pos = lsTmpFile + strlen(lsTmpFile);
	if (*(pos - 1) != '/')
	{
		*(pos++) = '/';
		*(pos) = '\0';
	}
	*(pos++) = '.';
	while (1)
	{
		*(pos++) = 't';
		*pos = '\0';
		struct stat ts = {0};
		stat(lsTmpFile, &ts);
		if (!S_ISDIR(ts.st_mode) && !S_ISREG(ts.st_mode))
		{
			break;
		}
	}
	ListDir(lsTmpFile, lsDir);
	/*
	if (data->dataConnectionMode == PASV_MODE)
	{
		data->datafd = accept(data->listenfd, NULL, NULL);
	}
	*/
	strcpy(responseStr, "150 LIST start.\r\n");
	if (!WriteResponse(data->connfd, strlen(responseStr), responseStr))
	{
		return 0;
	}
	printf("ready write\n");
	fflush(stdout);
	if (!WriteFile(data, lsTmpFile)) {
		printf("error ls\n");
		return 0;
	}
	printf("write file finished\n");
	if(remove(lsTmpFile) < 0) {
		printf("error remove\n");
		return 0;
	}
	return 1;
};

int RMD_Handler(struct ThreadParam* data) {
	char responseStr[RESPONSE_LENGTH] = {0};
	char rmDir[PATH_LENGTH] = {0};

	if (!AbsPath(rmDir, rootPath, data->currDir, data->request.arg))
	{
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	printf("rm dir is %s\n", rmDir);

	struct stat s = {0};
	stat(rmDir, &s);
	if (!(s.st_mode & S_IFDIR))
	{
		strcpy(responseStr, "530 invalid path.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}

	// TODO what if rm cwd?

	if (!rmdir(rmDir)) {
		// char responseStr[RESPONSE_LENGTH] = "250 RMD succeeded.\r\n";
		strcpy(responseStr, "250 RMD succeeded.\r\n");
		return WriteResponse(data->connfd, strlen(responseStr), responseStr);
	}
	// char responseStr[RESPONSE_LENGTH] = "550 RMD failed.\r\n";
	strcpy(responseStr, "550 RMD failed.\r\n");
	return WriteResponse(data->connfd, strlen(responseStr), responseStr);
};

int RNFR_Handler(struct ThreadParam* data) {

};

int RNTO_Handler(struct ThreadParam* data) {

};

int REST_Handler(struct ThreadParam* data)
{
	// TODO
}
#endif