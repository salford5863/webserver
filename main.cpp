#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <signal.h>

#include "HTTPServer.h"
#include "ResourceHost.h"

static HTTPServer* svr;
void handleSigPipe(int snum) {
	return;
}

void handleTermSig(int snum) {
	svr->canRun = false;
}

int main (int argc, const char * argv[])
{
	std::map<std::string, std::string> config;
	std::fstream cfile;
	std::string line, key, val;
	int epos = 0;
	int drop_uid = 0, drop_gid = 0;
	cfile.open("server.config");
	if (!cfile.is_open()) {
		std::cout << "Unable to open server.config file in working directory" << std::endl;
		return -1;
	}
	while (getline(cfile, line)) {
		if (line.length() == 0 || line.rfind("#", 0) == 0)
			continue;

		epos = line.find("=");
		key = line.substr(0, epos);
		val = line.substr(epos + 1, line.length());
		config.insert(std::pair<std::string, std::string> (key, val));
	}
	cfile.close();
	auto it_vhost = config.find("vhost");
	auto it_port = config.find("port");
	auto it_path = config.find("diskpath");
	if (it_vhost == config.end() || it_port == config.end() || it_path == config.end()) {
		std::cout << "vhost, port, and diskpath must be supplied in the config, at a minimum" << std::endl;
		return -1;
	}std::vector<std::string> vhosts;
	std::string vhost_alias_str = config["vhost"];
	std::string delimiter = ",";
	std::string token;
	size_t pos = vhost_alias_str.find(delimiter);
	do {
		pos = vhost_alias_str.find(delimiter);
		token = vhost_alias_str.substr(0, pos);
		vhosts.push_back(token);
		vhost_alias_str.erase(0, pos + delimiter.length());
	} while (pos != std::string::npos);
	if (config.find("drop_uid") != config.end() && config.find("drop_gid") != config.end()) {
		drop_uid = atoi(config["drop_uid"].c_str());
		drop_gid = atoi(config["drop_gid"].c_str());

		if (drop_uid <= 0 || drop_gid <= 0) {
			drop_uid = drop_gid = 0;
		}
	}
	signal(SIGPIPE, handleSigPipe);
	signal(SIGABRT, &handleTermSig);
	signal(SIGINT, &handleTermSig);
	signal(SIGTERM, &handleTermSig);
	svr = new HTTPServer(vhosts, atoi(config["port"].c_str()), config["diskpath"], drop_uid, drop_gid);
	if (!svr->start()) {
		svr->stop();
		delete svr;
		return -1;
	}
	svr->process();
	svr->stop();
	delete svr;

	return 0;
}
