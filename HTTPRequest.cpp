#include "HTTPMessage.h"
#include "HTTPRequest.h"

HTTPRequest::HTTPRequest() : HTTPMessage() {
	this->init();
}

HTTPRequest::HTTPRequest(std::string sData) : HTTPMessage(sData) {
	this->init();
}

HTTPRequest::HTTPRequest(byte* pData, unsigned int len) : HTTPMessage(pData, len) {
	this->init();
}

HTTPRequest::~HTTPRequest() {
}

void HTTPRequest::init() {
	method = 0;
	requestUri = "";
}

/**
 * @param name String representation of the Method
 * @return Corresponding Method ID, -1 if unable to find the method
 */
int HTTPRequest::methodStrToInt(std::string name) {
if (name.empty() || (name.size() >= 10))
		return -1;

	int ret = -1;
	for (unsigned int i = 0; i < NUM_METHODS; i++) {
		if (strcmp(requestMethodStr[i], name.c_str()) == 0) {
			ret = i;
			break;
		}
	}
	return ret;
}

/**
 @param mid Method ID to lookup
 * @return The method name in the from of a std::string. Blank if unable to find the method
 */
std::string HTTPRequest::methodIntToStr(unsigned int mid) {
	if (mid >= NUM_METHODS)
		return "";
	return requestMethodStr[mid];
}

/**
 * @return Byte array of this HTTPRequest to be sent over the wire
 */
byte* HTTPRequest::create() {
	clear();
	std::string mstr = "";
	mstr = methodIntToStr(method);
	if (mstr.empty()) {
		std::cout << "Could not create HTTPRequest, unknown method id: " << method << std::endl;
		return NULL;
	}
	putLine(mstr + " " + requestUri + " " + version);
	putHeaders();
	if ((data != NULL) && dataLen > 0) {
		putBytes(data, dataLen);
	}
	byte* createRetData = new byte[size()];
	setReadPos(0);
	getBytes(createRetData, size());

	return createRetData;
}

/**
 * @param True if successful. If false, sets parseErrorStr for reason of failure
 */
bool HTTPRequest::parse() {
	std::string initial = "", methodName = "";
	methodName = getStrElement();
	requestUri = getStrElement();
	version = getLine(); 
	method = methodStrToInt(methodName);
	if (method == -1) {
		parseErrorStr = "Invalid Method: " + methodName;
		return false;
	}
	parseHeaders();
	if ((method != POST) && (method != PUT))
		return true;
	if (!parseBody())
		return false;

	return true;
}

