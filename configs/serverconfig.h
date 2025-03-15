#pragma once
#include "definitions.h"

class ServerConfig {
public:
	ServerConfig();

	bool Load();

	unsigned short tcpPort;
	unsigned short udpPort;
	unsigned short maxPlayers;
	unsigned char serverID;
	unsigned char channelID;
	vector<Server> serverList;
	string sqlServer;
	string sqlUser;
	string sqlPassword;
	string sqlDatabase;
	vector<string> prohibitedNames;
};

extern ServerConfig serverConfig;