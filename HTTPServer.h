#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <unordered_map>
#include <vector>
#include <string>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#ifdef __linux__
#include <kqueue/sys/event.h> 
#else
#include <sys/event.h> 
#endif

#include "Client.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "ResourceHost.h"

#define INVALID_SOCKET -1
#define QUEUE_SIZE 1024

class HTTPServer {
	// Server Socket
	int listenPort;
	int listenSocket;
	struct sockaddr_in serverAddr; 
	int dropUid;
	int dropGid; 
	struct timespec kqTimeout = {2, 0}; 
	int kqfd; 
	struct kevent evList[QUEUE_SIZE]; 
	std::unordered_map<int, Client*> clientMap;
	std::vector<ResourceHost*> hostList; 
	std::unordered_map<std::string, ResourceHost*> vhosts;

	void updateEvent(int ident, short filter, u_short flags, u_int fflags, int data, void *udata);
	void acceptConnection();
	Client *getClient(int clfd);
	void disconnectClient(Client* cl, bool mapErase = true);
	void readClient(Client* cl, int data_len); 
	bool writeClient(Client* cl, int avail_bytes);
	ResourceHost* getResourceHostForRequest(HTTPRequest* req);

	// Request handling
	void handleRequest(Client* cl, HTTPRequest* req);
	void handleGet(Client* cl, HTTPRequest* req);
	void handleOptions(Client* cl, HTTPRequest* req);
	void handleTrace(Client* cl, HTTPRequest* req);

	// Response
	void sendStatusResponse(Client* cl, int status, std::string msg = "");
	void sendResponse(Client* cl, HTTPResponse* resp, bool disconnect);

public:
	bool canRun;

public:
	HTTPServer(std::vector<std::string> vhost_aliases, int port, std::string diskpath, int drop_uid=0, int drop_gid=0);
	~HTTPServer();

	bool start();
	void stop();
	void process();
};

#endif
