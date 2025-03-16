#pragma once
#include "user.h"

class UserManager {
public:
	~UserManager();

	vector<User*> GetUsers() const noexcept {
		return _users;
	}

	char AddUser(User* user);
	void RemoveUser(User* user);
	User* GetUserByConnection(TCPConnection::pointer connection);
	void RemoveUserByConnection(TCPConnection::pointer connection);
	void SendLoginPackets(User* user, Packet_ReplyType reply);
	void OnMinuteTick();

private:
	vector<User*> _users;
};

extern UserManager userManager;