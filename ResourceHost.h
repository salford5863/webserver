#ifndef _RESOURCEHOST_H_
#define _RESOURCEHOST_H_

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "Resource.h"
const static char* const validIndexes[] = {
	"index.html",
	"index.htm"
};

class ResourceHost {
private:
	std::string baseDiskPath;
	std::unordered_map<std::string, std::string> mimeMap = {
#define STR_PAIR(K,V) std::pair<std::string, std::string>(K,V)
#include "MimeTypes.inc"
	};

private:
	std::string lookupMimeType(std::string ext);
	Resource* readFile(std::string path, struct stat sb);
	Resource* readDirectory(std::string path, struct stat sb);
	std::string generateDirList(std::string dirPath);

public:
	ResourceHost(std::string base);
	~ResourceHost();
	void putResource(Resource* res, bool writeToDisk);
	Resource* getResource(std::string uri);
};


 #endif
