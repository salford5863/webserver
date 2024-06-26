#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <string>

typedef unsigned char byte;

class Resource {

private:
	byte* data; 
	unsigned int size;
	std::string mimeType;
	std::string location; 
	bool directory;

public:
	Resource(std::string loc, bool dir = false);
	~Resource();

	void setData(byte* d, unsigned int s) {
		data = d;
		size = s;
	}

	void setMimeType(std::string mt) {
		mimeType = mt;
	}

	std::string getMimeType() {
		return mimeType;
	}

	std::string getLocation() {
		return location;
	}

	bool isDirectory() {
		return directory;
	}

	byte* getData() {
		return data;
	}

	unsigned int getSize() {
		return size;
	}
	std::string getName() {
		std::string name = "";
		size_t slash_pos = location.find_last_of("/");
		if (slash_pos != std::string::npos)
			name = location.substr(slash_pos + 1);
		return name;
	}
	std::string getExtension() {
		std::string ext = "";
		size_t ext_pos = location.find_last_of(".");
		if (ext_pos != std::string::npos)
			ext = location.substr(ext_pos + 1);
		return ext;
	}
};

#endif
