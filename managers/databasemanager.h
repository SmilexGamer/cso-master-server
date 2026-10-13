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
	char IsUserSessionAdded(unsigned long userID);
	void RemoveAllUserSessions();
	const TransferLoginResult TransferLogin(const string& authToken);
	char AddUserTransfer(unsigned long userID, const string& authToken, unsigned char serverID, unsigned char channelID);
	void RemoveUserTransfer(unsigned long userID);
	void RemoveOldUserTransfers();
	void RemoveAllUserTransfers();
	bool SaveUserConfig(unsigned long userID, const vector<unsigned char>& userConfig);
	const vector<unsigned char> GetUserConfig(unsigned long userID);
	bool SaveUserQuickStart(unsigned long userID, unsigned long userQuickStart);
	unsigned long GetUserQuickStart(unsigned long userID);
	bool SaveUserBuyMenu(unsigned long userID, unsigned char categoryID, unsigned char slotID, unsigned char itemID);
	const vector<BuyMenu> GetUserBuyMenus(unsigned long userID);
	bool SaveUserBookMark(unsigned long userID, const BookMark& userBookMark);
	const vector<BookMark> GetUserBookMarks(unsigned long userID);
	const vector<InventoryItem> GetUserInventory(unsigned long userID);
	char AddRoom(unsigned short roomID, bool clanBattle, bool isPassworded, unsigned char levelLimit, unsigned char gameModeID, unsigned char mapID, unsigned char currentPlayers, unsigned char maxPlayers);
	bool UpdateRoom(unsigned short roomID, bool clanBattle, bool isPassworded, unsigned char levelLimit, unsigned char gameModeID, unsigned char mapID, unsigned char currentPlayers, unsigned char maxPlayers);
	SearchRoomResult SearchRoom(unsigned char level, unsigned char gameModeID, unsigned char mapID, unsigned char levelLimit, unsigned char clanBattle);
	void RemoveRoom(unsigned short roomID);
	void RemoveAllRooms();

private:
	MYSQL* _connection;
};

extern DatabaseManager databaseManager;