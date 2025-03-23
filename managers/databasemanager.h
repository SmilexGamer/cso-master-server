#pragma once
#include "mysql/mysql.h"
#include "user.h"

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
	char GetChannelNumPlayers(unsigned char serverID, unsigned channelID);
	void GetAllChannelsNumPlayers();
	LoginResult Login(const string& userName, const string& password);
	char CreateCharacter(unsigned long userID, const string& nickName);
	UserCharacterResult GetUserCharacter(unsigned long userID, unsigned short flag);
	char AddUserSession(unsigned long userID);
	void RemoveUserSession(unsigned long userID);
	void RemoveAllUserSessions();
	LoginResult TransferLogin(const string& userName, const string& userIP);
	char AddUserTransfer(const string& userName, const string& userIP, unsigned char serverID, unsigned channelID);
	void RemoveUserTransfer(const string& userName);
	void RemoveOldUserTransfers();
	void RemoveAllUserTransfers();

private:
	MYSQL* _connection;
	bool _addedServerChannel;
};

extern DatabaseManager databaseManager;