#pragma once
#include "user.h"

class UserManager {
public:
	~UserManager();

	const vector<User*>& GetUsers() const noexcept {
		return _users;
	}

	char AddUser(User* user);
	void RemoveUser(User* user);
	User* GetUserByConnection(TCPConnection::pointer connection);
	User* GetUserByUserID(unsigned long userID);
	void RemoveUserByConnection(TCPConnection::pointer connection);
	void SendLoginPackets(User* user, Packet_ReplyType reply = Packet_ReplyType::NoReply);
	void UpdateChannelNumPlayers();
	void OnMinuteTick();

private:
	vector<User*> _users;
};

extern UserManager userManager;