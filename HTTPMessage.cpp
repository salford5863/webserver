#include "HTTPMessage.h"

HTTPMessage::HTTPMessage() : ByteBuffer(4096) {
	this->init();
}

HTTPMessage::HTTPMessage(std::string sData) : ByteBuffer(sData.size() + 1) {
	putBytes((byte*)sData.c_str(), sData.size() + 1);
	this->init();
}

HTTPMessage::HTTPMessage(byte* pData, unsigned int len) : ByteBuffer(pData, len) {
	this->init();
}

HTTPMessage::~HTTPMessage() {
	headers->clear();
	delete headers;
}

void HTTPMessage::init() {
	parseErrorStr = "";

	data = NULL;
	dataLen = 0;

	version = DEFAULT_HTTP_VERSION; 

	headers = new std::map<std::string, std::string>();
}

/**
 * @param str String to put into the byte buffer
 * @param crlf_end If true (default), end the line with a \r\n
 */
void HTTPMessage::putLine(std::string str, bool crlf_end) {

	if (crlf_end)
		str += "\r\n";
	putBytes((byte*)str.c_str(), str.size());
}

void HTTPMessage::putHeaders() {
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers->begin(); it != headers->end(); it++) {
		std::string final = it->first + ": " + it->second;
		putLine(final, true);
	}
	putLine();
}

/**
 * @return Contents of the line in a string (without CR or LF)
 */
std::string HTTPMessage::getLine() {
	std::string ret = "";
	int startPos = getReadPos();
	bool newLineReached = false;
	char c = 0;
for (unsigned int i = startPos; i < size(); i++) {
		// If the next byte is a \r or \n, we've reached the end of the line and should break out of the loop
		c = peek();
		if ((c == 13) || (c == 10)) {
			newLineReached = true;
			break;
		}
ret += getChar();
	}
if (!newLineReached) {
		setReadPos(startPos); 
		ret = "";
		return ret;
	}
unsigned int k = 0;
	for (unsigned int i = getReadPos(); i < size(); i++) {
		if (k++ >= 2)
			break;
		c = getChar();
		if ((c != 13) && (c != 10)) {
			setReadPos(getReadPos() - 1);
			break;
		}
	}

	return ret;
}

/**
 * @param delim The delimiter to stop at when retriving the element. By default, it's a space
 * @return Token found in the buffer. Empty if delimiter wasn't reached
 */
std::string HTTPMessage::getStrElement(char delim) {
	std::string ret = "";
	int startPos = getReadPos();
	unsigned int size = 0;
	int endPos = find(delim, startPos);
	size = (endPos + 1) - startPos;

	if ((endPos == -1) || (size <= 0))
		return "";
	char *str = new char[size];
	bzero(str, size);
	getBytes((byte*)str, size);
	str[size - 1] = 0x00; 
	ret.assign(str);
	setReadPos(endPos + 1);

	return ret;
}

void HTTPMessage::parseHeaders() {
	std::string hline = "", app = "";
	hline = getLine();
while (hline.size() > 0) {
		app = hline;
		while (app[app.size() - 1] == ',') {
			app = getLine();
			hline += app;
		}

		addHeader(hline);
		hline = getLine();
	}
}

/**
 * @return True if successful. False on error, parseErrorStr is set with a reason
 */
bool HTTPMessage::parseBody() {
	std::string hlenstr = "";
	unsigned int contentLen = 0;
	hlenstr = getHeaderValue("Content-Length");
	if (hlenstr.empty())
		return true;

	contentLen = atoi(hlenstr.c_str());
if (contentLen > bytesRemaining() + 1) {
		std::stringstream pes;
		pes << "Content-Length (" << hlenstr << ") is greater than remaining bytes (" << bytesRemaining() << ")";
		parseErrorStr = pes.str();
		return false;
	} else {
		dataLen = contentLen;
	}

	unsigned int dIdx = 0, s = size();
	data = new byte[dataLen];
	for (unsigned int i = getReadPos(); i < s; i++) {
		data[dIdx] = get(i);
		dIdx++;
	}
	return true;
}

/**
 * @param string containing formatted header: value
 */
void HTTPMessage::addHeader(std::string line) {
	std::string key = "", value = "";
	size_t kpos;
	int i = 0;
	kpos = line.find(':');
	if (kpos == std::string::npos) {
		std::cout << "Could not addHeader: " << line.c_str() << std::endl;
		return;
	}
	key = line.substr(0, kpos);
	value = line.substr(kpos + 1, line.size() - kpos - 1);

	while (i < value.size() && value.at(i) == 0x20) {
		i++;
	}
	value = value.substr(i, value.size());
	addHeader(key, value);
}

/**
 * @param key String representation of the Header Key
 * @param value String representation of the Header value
 */
void HTTPMessage::addHeader(std::string key, std::string value) {
	headers->insert(std::pair<std::string, std::string>(key, value));
}

/**
 * @param key String representation of the Header Key
 * @param value Integer representation of the Header value
 */
void HTTPMessage::addHeader(std::string key, int value) {
	std::stringstream sz;
	sz << value;
	headers->insert(std::pair<std::string, std::string>(key, sz.str()));
}

/**
 * @param key Key to identify the header
 */
std::string HTTPMessage::getHeaderValue(std::string key) {

	char c;
	std::string key_lower = "";

	auto it = headers->find(key);

	if (it == headers->end()) {

		for (int i = 0; i < key.length(); i++) {
			c = key.at(i);
			key_lower += tolower(c);
		}

		it = headers->find(key_lower);
		if (it == headers->end())
			return "";
	}

	return it->second;
}

std::string HTTPMessage::getHeaderStr(int index) {
	int i = 0;
	std::string ret = "";
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers->begin(); it != headers->end(); it++) {
		if (i == index) {
			ret = it->first + ": " + it->second;
			break;
		}

		i++;
	}
	return ret;
}

/**
 * @return size of the map
 */
int HTTPMessage::getNumHeaders() {
	return headers->size();
}

void HTTPMessage::clearHeaders() {
	headers->clear();
}

