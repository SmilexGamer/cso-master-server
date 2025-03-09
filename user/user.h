#pragma once
#include "tcp_connection.h"
#include "gameuser.h"

#ifdef GetUserName
#undef GetUserName
#endif

class User {
public:
	User(TCPConnection::pointer connection, unsigned long _userID);

	TCPConnection::pointer GetConnection() {
		return _connection;
	}

	unsigned long GetUserID() const noexcept {
		return _userID;
	}

	string GetUserName() const noexcept {
		return _userName;
	}

	GameUser GetGameUser() {
		return _gameUser;
	}

private:
	TCPConnection::pointer _connection;
	unsigned long _userID;
	string _userName;
	GameUser _gameUser;
};