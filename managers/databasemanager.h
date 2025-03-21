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

	bool Init(const string& server, const string& user, const string& password, const string& database);
	bool AddServerChannel();
	void RemoveServerChannel();
	void UpdateChannelNumPlayers(unsigned short numPlayers);
	void GetChannelsNumPlayers();
	LoginResult Login(const string& userName, const string& password);
	char CreateCharacter(unsigned long userID, const string& nickName);
	UserCharacterResult GetUserCharacter(unsigned long userID, unsigned short flag);
	char AddUserSession(unsigned long userID);
	void RemoveUserSession(unsigned long userID);
	void RemoveAllUserSessions();

private:
	MYSQL* _connection;
	bool _addedServerChannel;
};

extern DatabaseManager databaseManager;