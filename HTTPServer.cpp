#include "HTTPServer.h"

/**
 * @param vhost_aliases List of hostnames the HTTP server will respond to
 * @param port Port the vhost listens on
 * @param diskpath Path to the folder the vhost serves up
 * @param drop_uid UID to setuid to after bind().  Ignored if 0
 * @param drop_gid GID to setgid to after bind().  Ignored if 0
 */
HTTPServer::HTTPServer(std::vector<std::string> vhost_aliases, int port, std::string diskpath, int drop_uid, int drop_gid) {
	canRun = false;
	listenSocket = INVALID_SOCKET;
	listenPort = port;
	kqfd = -1;
	dropUid = drop_uid;
	dropGid = drop_gid;

	std::cout << "Port: " << port << std::endl;
	std::cout << "Disk path: " << diskpath.c_str() << std::endl;
	ResourceHost* resHost = new ResourceHost(diskpath);
	hostList.push_back(resHost);

	char tmpstr[128] = {0};
	sprintf(tmpstr, "localhost:%i", listenPort);
	vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));
	sprintf(tmpstr, "127.0.0.1:%i", listenPort);
	vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));

	for (std::string vh : vhost_aliases) {
		if (vh.length() >= 122) {
			std::cout << "vhost " << vh << " too long, skipping!" << std::endl;
			continue;
		}

		std::cout << "vhost: " << vh << std::endl;
		sprintf(tmpstr, "%s:%i", vh.c_str(), listenPort);
		vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));
	}
}
HTTPServer::~HTTPServer() {
	while (!hostList.empty()) {
		ResourceHost* resHost = hostList.back();
		delete resHost;
		hostList.pop_back();
	}
	vhosts.clear();
}

/**
 * @return True if initialization succeeded. False if otherwise
 */
bool HTTPServer::start() {
	canRun = false;
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Could not create socket!" << std::endl;
		return false;
	}
	fcntl(listenSocket, F_SETFL, O_NONBLOCK);

	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET; 
	serverAddr.sin_port = htons(listenPort); 
	serverAddr.sin_addr.s_addr = INADDR_ANY; 
	if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
		std::cout << "Failed to bind to the address!" << std::endl;
		return false;
	}
	if (dropUid > 0 && dropGid > 0) {
		if (setgid(dropGid) != 0) {
			std::cout << "setgid to " << dropGid << " failed!" << std::endl;
			return false;
		}
		
		if (setuid(dropUid) != 0) {
			std::cout << "setuid to " << dropUid << " failed!" << std::endl;
			return false;
		}

		std::cout << "Successfully dropped uid to " << dropUid << " and gid to " << dropGid << std::endl;
	}
if (listen(listenSocket, SOMAXCONN) != 0) {
		std::cout << "Failed to put the socket in a listening state" << std::endl;
		return false;
	}
	kqfd = kqueue();
	if (kqfd == -1) {
		std::cout << "Could not create the kernel event queue!" << std::endl;
		return false;
	}
	updateEvent(listenSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

	canRun = true;
	std::cout << "Server ready. Listening on port " << listenPort << "..." << std::endl;
	return true;
}
void HTTPServer::stop() {
	canRun = false;

	if (listenSocket != INVALID_SOCKET) {
		for (auto& x : clientMap)
			disconnectClient(x.second, false);
		clientMap.clear();
		updateEvent(listenSocket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		shutdown(listenSocket, SHUT_RDWR);
		close(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	if (kqfd != -1) {
		close(kqfd);
		kqfd = -1;
	}

	std::cout << "Server shutdown!" << std::endl;
}
void HTTPServer::updateEvent(int ident, short filter, u_short flags, u_int fflags, int data, void *udata) {
	struct kevent kev;
	EV_SET(&kev, ident, filter, flags, fflags, data, udata);
	kevent(kqfd, &kev, 1, NULL, 0, NULL);
}
void HTTPServer::process() {
	int nev = 0; 
	Client* cl = NULL;

	while (canRun) {
		nev = kevent(kqfd, NULL, 0, evList, QUEUE_SIZE, &kqTimeout);

		if (nev <= 0)
			continue;
for (int i = 0; i < nev; i++) {
			if (evList[i].ident == (unsigned int)listenSocket) { 
				acceptConnection();
			} else { 
				cl = getClient(evList[i].ident); 
				if (cl == NULL) {
					std::cout << "Could not find client" << std::endl;
					updateEvent(evList[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
					updateEvent(evList[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
					close(evList[i].ident);

					continue;
				}
				if (evList[i].flags & EV_EOF) {
					disconnectClient(cl, true);
					continue;
				}

				if (evList[i].filter == EVFILT_READ) {
					readClient(cl, evList[i].data); 
					updateEvent(evList[i].ident, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
					updateEvent(evList[i].ident, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
				} else if (evList[i].filter == EVFILT_WRITE) {if (!writeClient(cl, evList[i].data)) { // data contains number of bytes that can be written
						updateEvent(evList[i].ident, EVFILT_READ, EV_ENABLE, 0, 0, NULL);
						updateEvent(evList[i].ident, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
					}
				}
			}
}

void HTTPServer::acceptConnection() {
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	int clfd = INVALID_SOCKET;
	clfd = accept(listenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);
	if (clfd == INVALID_SOCKET)
		return;
	fcntl(clfd, F_SETFL, O_NONBLOCK);
	Client *cl = new Client(clfd, clientAddr);
updateEvent(clfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	updateEvent(clfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL); // Disabled initially

	// Add the client object to the client map
	clientMap.insert(std::pair<int, Client*>(clfd, cl));

	// Print the client's IP on connect
	std::cout << "[" << cl->getClientIP() << "] connected" << std::endl;
}

/**
 * @param clfd Client socket descriptor
 * @return Pointer to Client object if found. NULL otherwise
 */
Client* HTTPServer::getClient(int clfd) {
	auto it = clientMap.find(clfd);

	if (it == clientMap.end())
		return NULL;
	return it->second;
}

/**
 * @param cl Pointer to Client object
 * @param mapErase When true, remove the client from the client map. Needed if operations on the
 */
void HTTPServer::disconnectClient(Client *cl, bool mapErase) {
	if (cl == NULL)
		return;

	std::cout << "[" << cl->getClientIP() << "] disconnected" << std::endl;
	updateEvent(cl->getSocket(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
	updateEvent(cl->getSocket(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	close(cl->getSocket());
	if (mapErase)
		clientMap.erase(cl->getSocket());

	// Delete the client object from memory
	delete cl;
}

/**
 * @param cl Pointer to Client that sent the data
 * @param data_len Number of bytes waiting to be read
 */
void HTTPServer::readClient(Client *cl, int data_len) {
	if (cl == NULL)
		return;
	if (data_len <= 0)
		data_len = 1400;

	HTTPRequest* req;
	char* pData = new char[data_len];
	bzero(pData, data_len);
	int flags = 0;
	ssize_t lenRecv = recv(cl->getSocket(), pData, data_len, flags);
	if (lenRecv == 0) {
		std::cout << "[" << cl->getClientIP() << "] has opted to close the connection" << std::endl;
		disconnectClient(cl, true);
	} else if (lenRecv < 0) {
		disconnectClient(cl, true);
	} else {
		req = new HTTPRequest((byte*)pData, lenRecv);
		handleRequest(cl, req);
		delete req;
	}

	delete [] pData;
}

bool HTTPServer::writeClient(Client* cl, int avail_bytes) {
	if (cl == NULL)
		return false;

	int actual_sent = 0; 
	int attempt_sent = 0; 
	int remaining = 0; 
	bool disconnect = false;
	byte* pData = NULL;
if (avail_bytes > 1400) {
		avail_bytes = 1400;
	} else if (avail_bytes == 0) {
		avail_bytes = 64;
	}

	SendQueueItem* item = cl->nextInSendQueue();
	if (item == NULL)
		return false;

	pData = item->getData();
	remaining = item->getSize() - item->getOffset();
	disconnect = item->getDisconnect();

	if (avail_bytes >= remaining) {
		attempt_sent = remaining;
	} else {
		attempt_sent = avail_bytes;
	}

	actual_sent = send(cl->getSocket(), pData + (item->getOffset()), attempt_sent, 0);
	if (actual_sent >= 0)
		item->setOffset(item->getOffset() + actual_sent);
	else
		disconnect = true;
if (item->getOffset() >= item->getSize())
		cl->dequeueFromSendQueue();

	if (disconnect) {
		disconnectClient(cl, true);
		return false;
	}

	return true;
}

/**
 * @param cl Client object where request originated from
 * @param req HTTPRequest object filled with raw packet data
 */
void HTTPServer::handleRequest(Client *cl, HTTPRequest* req) {
	if (!req->parse()) {
		std::cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << std::endl;
		std::cout << req->getParseError().c_str() << std::endl;
		sendStatusResponse(cl, Status(BAD_REQUEST));
		return;
	}

	std::cout << "[" << cl->getClientIP() << "] " << req->methodIntToStr(req->getMethod()) << " " << req->getRequestUri() << std::endl;
	
	switch (req->getMethod()) {
	case Method(HEAD):
	case Method(GET):
		handleGet(cl, req);
		break;
	case Method(OPTIONS):
		handleOptions(cl, req);
		break;
	case Method(TRACE):
		handleTrace(cl, req);
		break;
	default:
		std::cout << "[" << cl->getClientIP() << "] Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << std::endl;
		sendStatusResponse(cl, Status(NOT_IMPLEMENTED));
		break;
	}
}

/**
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleGet(Client* cl, HTTPRequest* req) {
	std::string uri;
	Resource* r = NULL;
	ResourceHost* resHost = this->getResourceHostForRequest(req);
if (resHost == NULL) {
		sendStatusResponse(cl, Status(BAD_REQUEST), "Invalid/No Host specified");
		return;
	}

	uri = req->getRequestUri();
	r = resHost->getResource(uri);

	if (r != NULL) { 
		std::cout << "[" << cl->getClientIP() << "] " << "Sending file: " << uri << std::endl;

		HTTPResponse* resp = new HTTPResponse();
		resp->setStatus(Status(OK));
		resp->addHeader("Content-Type", r->getMimeType());
		resp->addHeader("Content-Length", r->getSize());
if (req->getMethod() == Method(GET))
			resp->setData(r->getData(), r->getSize());

		bool dc = false;

		if (req->getVersion().compare(HTTP_VERSION_10) == 0)
			dc = true;
std::string connection_val = req->getHeaderValue("Connection");
		if (connection_val.compare("close") == 0)
			dc = true;

		sendResponse(cl, resp, dc);
		delete resp;
		delete r;
	} else { 
		std::cout << "[" << cl->getClientIP() << "] " << "File not found: " << uri << std::endl;
		sendStatusResponse(cl, Status(NOT_FOUND));
	}
}

void HTTPServer::handleOptions(Client* cl, HTTPRequest* req) {
	std::string allow = "HEAD, GET, OPTIONS, TRACE";

	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(OK));
	resp->addHeader("Allow", allow.c_str());
	resp->addHeader("Content-Length", "0"); // Required

	sendResponse(cl, resp, true);
	delete resp;
}

/**
 * TRACE: send back the request as received by the server verbatim
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleTrace(Client* cl, HTTPRequest *req) {
	unsigned int len = req->size();
	byte* buf = new byte[len];
	bzero(buf, len);
	req->setReadPos(0); 
	req->getBytes(buf, len);
	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(OK));
	resp->addHeader("Content-Type", "message/http");
	resp->addHeader("Content-Length", len);
	resp->setData(buf, len);
	sendResponse(cl, resp, true);

	delete resp;
	delete[] buf;
}

/**
 * @param cl Client to send the status code to
 * @param status Status code corresponding to the enum in HTTPMessage.h
 * @param msg An additional message to append to the body text
 */
void HTTPServer::sendStatusResponse(Client* cl, int status, std::string msg) {
	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(status));
	std::string body = resp->getReason();
	if (msg.length() > 0)
		body +=  ": " + msg;

	unsigned int slen = body.length();
	char* sdata = new char[slen];
	bzero(sdata, slen);
	strncpy(sdata, body.c_str(), slen);

	resp->addHeader("Content-Type", "text/plain");
	resp->addHeader("Content-Length", slen);
	resp->setData((byte*)sdata, slen);

	sendResponse(cl, resp, true);

	delete resp;
}

/**
 * @param cl Client to send data to
 * @param buf ByteBuffer containing data to be sent
 * @param disconnect Should the server disconnect the client after sending (Optional, default = false)
 */
void HTTPServer::sendResponse(Client* cl, HTTPResponse* resp, bool disconnect) {
	resp->addHeader("Server", "httpserver/1.0");
	std::string tstr;
	char tbuf[36] = {0};
	time_t rawtime;
	struct tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	strftime(tbuf, 36, "%a, %d %b %Y %H:%M:%S GMT", ptm);
	tstr = tbuf;
	resp->addHeader("Date", tstr);
if (disconnect)
		resp->addHeader("Connection", "close");
byte* pData = resp->create();

	cl->addToSendQueue(new SendQueueItem(pData, resp->size(), disconnect));
}

ResourceHost* HTTPServer::getResourceHostForRequest(HTTPRequest* req) {
	ResourceHost* resHost = NULL;
	std::string host = "";
if (req->getVersion().compare(HTTP_VERSION_11) == 0) {
		host = req->getHeaderValue("Host");
if (host.find(":") == std::string::npos) {
			host.append(":" + std::to_string(listenPort));
		}

		std::unordered_map<std::string, ResourceHost*>::const_iterator it = vhosts.find(host);

		if (it != vhosts.end())
			resHost = it->second;
	} else {
		if (hostList.size() > 0)
			resHost = hostList[0];
	}
	
	return resHost;
}
