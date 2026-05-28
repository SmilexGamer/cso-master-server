#pragma once
#include "tcp_connection.h"

#ifdef GetUserName
#undef GetUserName
#endif

#define HARDWARE_ID_SIZE 16

#define USERCHARACTER_FLAG_UNK1			0x1
#define USERCHARACTER_FLAG_NICKNAME		0x2
#define USERCHARACTER_FLAG_UNK4			0x4
#define USERCHARACTER_FLAG_LEVEL		0x8
#define USERCHARACTER_FLAG_UNK10		0x10
#define USERCHARACTER_FLAG_EXP			0x20
#define USERCHARACTER_FLAG_CASH			0x40
#define USERCHARACTER_FLAG_POINTS		0x80
#define USERCHARACTER_FLAG_BATTLESTATS	0x100
#define USERCHARACTER_FLAG_LOCATION		0x200
#define USERCHARACTER_FLAG_UNK400		0x400
#define USERCHARACTER_FLAG_UNK800		0x800
#define USERCHARACTER_FLAG_CLAN			0x1000
#define USERCHARACTER_FLAG_ALL			0xFFFF

struct UserCharacter {
	unsigned short flag = 0;
	unsigned char unk1 = 0;
	string nickName = "";
	string unk4_1 = "";
	unsigned char unk4_2 = 0;
	unsigned char unk4_3 = 0;
	unsigned char unk4_4 = 0;
	unsigned char level = 0;
	unsigned char unk10 = 0;
	unsigned long long exp = 0;
	unsigned long long cash = 0;
	unsigned long long points = 0;
	unsigned long battles = 0;
	unsigned long wins = 0;
	unsigned long frags = 0;
	unsigned long deaths = 0;
	unsigned short city = 0;
	unsigned short county = 0;
	unsigned short neighborhood = 0;
	string unk200_5 = "";
	unsigned long unk400 = 0;
	unsigned char unk800 = 0;
	unsigned long clanID = 0;
	unsigned long clanMarkID = 0;
	string clanName = "";
	unsigned char clanUnk4 = 0;
	unsigned char clanUnk5 = 0;
	unsigned char clanUnk6 = 0;
};

struct UserCharacterResult {
	UserCharacter userCharacter;
	char result = 0;
};

enum PortType {
	Guest = 0,
	Host = 1
};

struct UserNetwork {
	unsigned long externalIP = 0;
	unsigned short externalHostPort = 0;
	unsigned short externalGuestPort = 0;
	unsigned long localIP = 0;
	unsigned short localHostPort = 0;
	unsigned short localGuestPort = 0;
};

enum UserStatus {
	InLogin = 0,
	InServerList = 1,
	InLobby = 2,
	InRoom = 3,
	InGame = 4
};

class User {
public:
	User(TCPConnection::pointer connection, unsigned long userID, const string& userName, const UserNetwork& userNetwork = {});

	TCPConnection::pointer GetConnection() const noexcept {
		return _connection;
	}

	const string GetUserLogName() const noexcept {
		return format("{}, {}", _userID, _userName);
	}

	const string& GetUserIPAddress() const noexcept {
		return _connection->GetIPAddress();
	}

	unsigned long GetUserID() const noexcept {
		return _userID;
	}

	const string& GetUserName() const noexcept {
		return _userName;
	}

	const UserNetwork& GetUserNetwork() const noexcept {
		return _userNetwork;
	}

	void SetUserNetwork(PortType portType, unsigned long externalIP, unsigned short externalPort, unsigned long localIP, unsigned short localPort);

	UserStatus GetUserStatus() const noexcept {
		return _userStatus;
	}

	void SetUserStatus(UserStatus userStatus) noexcept {
		_userStatus = userStatus;
	}

	unsigned short GetCurrentRoomID() const noexcept {
		return _currentRoomID;
	}

	void SetCurrentRoomID(unsigned short roomID) noexcept {
		_currentRoomID = roomID;
	}

	char CreateUserCharacter(const string& nickName) const noexcept;
	UserCharacterResult GetUserCharacter(unsigned short flag) const noexcept;
	char CreateUserBuymenus() const noexcept;
	char CreateUserBookmarks() const noexcept;
	char CreateUserInventory() const noexcept;
	char AddUserSession() const noexcept;
	void RemoveUserSession() const noexcept;
	char AddUserTransfer(const string& authToken, unsigned char serverID, unsigned char channelID) const noexcept;
	void RemoveUserTransfer() const noexcept;
	char IsUserCharacterExists() const noexcept;
	bool SaveUserOption(const vector<unsigned char>& userOption) const noexcept;
	const vector<unsigned char> GetUserOption() const noexcept;
	bool SaveUserBuyMenu(unsigned char categoryID, unsigned char slotID, unsigned char itemID) const noexcept;
	const vector<BuyMenu> GetUserBuyMenus() const noexcept;
	bool SaveUserBookMark(const BookMark& userBookMark) const noexcept;
	const vector<BookMark> GetUserBookMarks() const noexcept;
	const vector<InventoryItem> GetUserInventory() const noexcept;

private:
	TCPConnection::pointer _connection;
	unsigned long _userID;
	string _userName;
	UserNetwork _userNetwork;
	UserStatus _userStatus;
	unsigned short _currentRoomID;
};

struct GameUser {
	User* user;
	UserCharacter userCharacter;
};