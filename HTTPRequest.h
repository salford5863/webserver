#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "HTTPMessage.h"

class HTTPRequest : public HTTPMessage {
private:
    int method;
    std::string requestUri;

protected:
    virtual void init();

public:
    HTTPRequest();
    HTTPRequest(std::string sData);
    HTTPRequest(byte *pData, unsigned int len);
    virtual ~HTTPRequest();

    virtual byte *create();
    virtual bool parse();

    int methodStrToInt(std::string name);
    std::string methodIntToStr(unsigned int mid);

    void setMethod(int m) {
        method = m;
    }

    int getMethod() {
        return method;
    }

    void setRequestUri(std::string u) {
        requestUri = u;
    }

    std::string getRequestUri() {
        return requestUri;
    }
};

#endif
