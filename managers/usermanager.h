#pragma once
#include "user.h"

class UserManager {
public:
	UserManager();
	~UserManager();

	vector<User*> GetUsers() const noexcept {
		return _users;
	}

	void AddUser(User* user);
	void RemoveUser(User* user);
	void DisconnectUserByConnection(TCPConnection::pointer connection);
	void DisconnectUserByConnection(TCPConnection::pointer connection, boost::system::error_code ec);
	User* GetUserByConnection(TCPConnection::pointer connection);
	void OnMinuteTick();

private:
	vector<User*> _users;
};

extern UserManager userManager;