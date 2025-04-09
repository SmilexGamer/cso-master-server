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
	void Shutdown();
	bool AddServerChannel();
	void RemoveServerChannel();
	void UpdateChannelNumPlayers(unsigned short numPlayers);
	char GetChannelNumPlayers(unsigned char serverID, unsigned char channelID);
	void GetAllChannelsNumPlayers();
	const LoginResult Login(const string& userName, const string& password);
	char CreateCharacter(unsigned long userID, const string& nickName);
	const UserCharacterResult GetUserCharacter(unsigned long userID, unsigned short flag);
	char AddUserSession(unsigned long userID);
	void RemoveUserSession(unsigned long userID);
	void RemoveAllUserSessions();
	const LoginResult TransferLogin(const string& userName, const string& userIP);
	char AddUserTransfer(const string& userName, const string& userIP, unsigned char serverID, unsigned char channelID);
	void RemoveUserTransfer(const string& userName);
	void RemoveOldUserTransfers();
	void RemoveAllUserTransfers();
	bool SaveUserOption(unsigned long userID, const vector<unsigned char>& userOption);
	const vector<unsigned char> GetUserOption(unsigned long userID);

private:
	MYSQL* _connection;
	bool _addedServerChannel;
};

extern DatabaseManager databaseManager;