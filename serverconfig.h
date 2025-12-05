#pragma once
#include "tcp_connection.h"
#include <vector>
#include <string>

using namespace std;

enum ServerType {
	Normal = 0,
	Newbie = 1
};

enum ServerStatus {
	NotReady = 0,
	Ready = 1
};

struct Channel {
	unsigned char id = 0;
	string name = "";
	unsigned short numPlayers = 0;
	unsigned short maxPlayers = 0;
	string ip = "";
	unsigned short port = 0;
	bool isOnline = true;
	unsigned long long lastHeartBeat = 0;
};

struct Server {
	unsigned char id = 0;
	ServerStatus status = ServerStatus::NotReady;
	ServerType type = ServerType::Normal;
	string name = "";
	vector<Channel> channels;
};

class ServerConfig {
public:
	ServerConfig();

	bool Load();

	unsigned short port;
	unsigned short maxPlayers;
	unsigned char serverID;
	unsigned char channelID;
	vector<Server> serverList;
	string sqlServer;
	string sqlUser;
	string sqlPassword;
	string sqlDatabase;
	vector<string> prohibitedNames;
	CipherMethod decryptCipherMethod;
	CipherMethod encryptCipherMethod;
	vector<BuyMenu> defaultBuyMenus;
};

extern ServerConfig serverConfig;