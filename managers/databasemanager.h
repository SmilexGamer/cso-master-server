#pragma once
#include "definitions.h"
#include "mysql/mysql.h"
#include "user.h"
#include <string>

using namespace std;

struct LoginResult {
	unsigned long userID;
	Packet_ReplyType reply;
};

class DatabaseManager {
public:
	DatabaseManager();
	~DatabaseManager();

	bool Init(string server, string user, string password, string database);
	bool AddServerChannel();
	void RemoveServerChannel();
	void UpdateChannelNumPlayers(unsigned short numPlayers);
	void GetChannelsNumPlayers();
	LoginResult Login(string userName, string password);
	char CreateCharacter(unsigned long userID, string nickName);
	UserCharacterResult GetUserCharacter(unsigned long userID, unsigned short flag);
	char AddUserSession(unsigned long userID);
	void RemoveUserSession(unsigned long userID);
	void RemoveAllUserSessions();

private:
	MYSQL* _connection;
	bool _addedServerChannel;
};

extern DatabaseManager databaseManager;