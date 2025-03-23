#pragma once
#include "tcp_connection.h"

#ifdef GetUserName
#undef GetUserName
#endif

enum UserCharacterFlag {
	Unk1 = 0x1,
	NickName = 0x2,
	Unk4 = 0x4,
	Level = 0x8,
	Unk10 = 0x10,
	Exp = 0x20,
	Cash = 0x40,
	Points = 0x80,
	BattleStats = 0x100,
	Location = 0x200,
	Unk400 = 0x400,
	Unk800 = 0x800,
	Unk1000 = 0x1000,
	All = 0xFFFF
};

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

class User {
public:
	User(TCPConnection::pointer connection, unsigned long userID, const string& userName);

	TCPConnection::pointer GetConnection() const noexcept {
		return _connection;
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

	UserNetwork GetUserNetwork() const noexcept {
		return _userNetwork;
	}

	void SetUserNetwork(unsigned char portType, unsigned long localIP, unsigned short localPort, unsigned short externalPort);

	char CreateCharacter(const string& nickName);
	UserCharacterResult GetUserCharacter(unsigned short flag);
	char IsUserCharacterExists();

private:
	TCPConnection::pointer _connection;
	unsigned long _userID;
	string _userName;
	UserNetwork _userNetwork;
};

struct UserFull {
	User* user;
	UserCharacter userCharacter;
};