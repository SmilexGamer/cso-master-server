#pragma once
#include "tcp_connection.h"

#ifdef GetUserName
#undef GetUserName
#endif

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

class User {
public:
	User(TCPConnection::pointer connection, unsigned long userID, string userName);

	TCPConnection::pointer GetConnection() const noexcept {
		return _connection;
	}

	unsigned long GetUserID() const noexcept {
		return _userID;
	}

	const string& GetUserName() const noexcept {
		return _userName;
	}

	UserCharacterResult GetUserCharacter(unsigned short flag);
	char IsUserCharacterExists();

private:
	TCPConnection::pointer _connection;
	unsigned long _userID;
	string _userName;
};