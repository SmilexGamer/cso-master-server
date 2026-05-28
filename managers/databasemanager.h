#pragma once
#include "mysql/mysql.h"
#include "user.h"

struct LoginResult {
	unsigned long userID = 0;
	Packet_ReplyType reply = Packet_ReplyType::LoginSuccess;
};

struct TransferLoginResult {
	unsigned long userID = 0;
	string userName = "";
	UserNetwork userNetwork;
	Packet_ReplyType reply = Packet_ReplyType::LoginSuccess;
};

class DatabaseManager {
public:
	DatabaseManager();
	~DatabaseManager();

	bool Init(const string& server, const string& user, const string& password, const string& database);
	void Shutdown();
	const LoginResult Login(const string& userName, const string& password);
	char CreateUserCharacter(unsigned long userID, const string& nickName);
	const UserCharacterResult GetUserCharacter(unsigned long userID, unsigned short flag);
	bool CreateUserBuymenus(unsigned long userID);
	bool CreateUserBookmarks(unsigned long userID);
	bool CreateUserInventory(unsigned long userID);
	char AddUserSession(unsigned long userID);
	void RemoveUserSession(unsigned long userID);
	void RemoveAllUserSessions(unsigned char serverID = 0, unsigned char channelID = 0);
	const TransferLoginResult TransferLogin(const string& authToken);
	char AddUserTransfer(unsigned long userID, const string& authToken, unsigned char serverID, unsigned char channelID, const UserNetwork& userNetwork);
	void RemoveUserTransfer(unsigned long userID);
	void RemoveOldUserTransfers();
	void RemoveAllUserTransfers(unsigned char serverID = 0, unsigned char channelID = 0);
	bool SaveUserOption(unsigned long userID, const vector<unsigned char>& userOption);
	const vector<unsigned char> GetUserOption(unsigned long userID);
	bool SaveUserBuyMenu(unsigned long userID, unsigned char categoryID, unsigned char slotID, unsigned char itemID);
	const vector<BuyMenu> GetUserBuyMenus(unsigned long userID);
	bool SaveUserBookMark(unsigned long userID, const BookMark& userBookMark);
	const vector<BookMark> GetUserBookMarks(unsigned long userID);
	const vector<InventoryItem> GetUserInventory(unsigned long userID);

private:
	MYSQL* _connection;
	bool _addedServerChannel;
};

extern DatabaseManager databaseManager;