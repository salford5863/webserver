#include "Client.h"

Client::Client(int fd, sockaddr_in addr) {
	socketDesc = fd;
	clientAddr = addr;
}

Client::~Client() {
	clearSendQueue();
}

void Client::addToSendQueue(SendQueueItem* item) {
	sendQueue.push(item);
}

/**
 * @return Integer representing number of items in this clients send queue
 */
unsigned int Client::sendQueueSize() {
	return sendQueue.size();
}

/**
 * @return SendQueueItem object containing the data to send and current offset
 */
SendQueueItem* Client::nextInSendQueue() {
	if (sendQueue.empty())
		return NULL;

	return sendQueue.front();
}

void Client::dequeueFromSendQueue() {
	SendQueueItem* item = nextInSendQueue();
	if (item != NULL) {
		sendQueue.pop();
		delete item;
	}
}

void Client::clearSendQueue() {
	while (!sendQueue.empty()) {
		delete sendQueue.front();
		sendQueue.pop();
	}
}
