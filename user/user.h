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
#define USERCHARACTER_FLAG_UNK1000		0x1000
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
	unsigned long unk1000_1 = 0;
	unsigned long unk1000_2 = 0;
	string unk1000_3 = "";
	unsigned char unk1000_4 = 0;
	unsigned char unk1000_5 = 0;
	unsigned char unk1000_6 = 0;
};

struct UserCharacterResult {
	UserCharacter userCharacter;
	char result = 0;
};

struct UserNetwork {
	unsigned long localIP = 0;
	unsigned short localPortType0 = 0;
	unsigned short localPortType1 = 0;
	unsigned long externalIP = 0;
	unsigned short externalPortType0 = 0;
	unsigned short externalPortType1 = 0;
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
	User(TCPConnection::pointer connection, unsigned long userID, const string& userName);

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

	void SetUserNetwork(unsigned char portType, unsigned long localIP, unsigned short localPort, unsigned short externalPort);

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

	char CreateCharacter(const string& nickName) const noexcept;
	UserCharacterResult GetUserCharacter(unsigned short flag) const noexcept;
	char IsUserCharacterExists() const noexcept;
	bool SaveUserOption(const vector<unsigned char>& userOption) const noexcept;
	const vector<unsigned char> GetUserOption() const noexcept;

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