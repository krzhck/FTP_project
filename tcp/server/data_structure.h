#ifndef DATA_STRUCTURE
#define DATA_STRUCTURE
#include <netinet/in.h>
#include "global.h"
enum RequestType {
	USER,
	PASS,
	RETR,
	STOR,
	QUIT,
	SYST,
	TYPE,
	PORT,
	PASV,
	MKD,
	CWD,
	PWD,
	LIST,
	RMD,
	RNFR,
	RNTO,
	REST
};

enum ClientState {
	NO_USER,
	HAS_USER,
	HAS_PASS
};

enum DataConnectionMode {
	NO_CONNECTION,
	PASV_MODE,
	PORT_MODE
};

struct Request {
	enum RequestType type;
	char arg[SENTENCE_LENGTH]; // = {0};
};

struct ClientAddr {
	char IP[30]; // = {0};
	int port; // = -1;
};

struct ThreadParam {
	int connfd; // = -1; // for command
	int datafd; // = -1; // for data transfer
	int listenfd; // = -1; // for pasv
	int dataPort; // = -1;
	char sentence[SENTENCE_LENGTH]; // = {0};
	char currDir[PATH_LENGTH]; // = {0}; // relative to rootPath
	enum ClientState clientState; // = NO_USER;
	enum DataConnectionMode dataConnectionMode; // = NO_CONNECTION;
	struct Request request;
	struct ClientAddr clientAddr;
	int readPos; // file read position
	struct sockaddr_in dataAddr;
};

#endif