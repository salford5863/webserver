#ifndef _SENDQUEUEITEM_H_
#define _SENDQUEUEITEM_H_

#include <cstdlib>

typedef unsigned char byte;
class SendQueueItem {

private:
	byte* sendData;
	unsigned int sendSize;
	unsigned int sendOffset;
	bool disconnect; 

public:
	SendQueueItem(byte* data, unsigned int size, bool dc) {
		sendData = data;
		sendSize = size;
		disconnect = dc;
		sendOffset = 0;
	}

	~SendQueueItem() {
		if (sendData != NULL) {
			delete [] sendData;
			sendData = NULL;
		}
	}

	void setOffset(unsigned int off) {
		sendOffset = off;
	}

	byte* getData() {
		return sendData;
	}

	unsigned int getSize() {
		return sendSize;
	}

	bool getDisconnect() {
		return disconnect;
	}

	unsigned int getOffset() {
		return sendOffset;
	}

};

#endif
