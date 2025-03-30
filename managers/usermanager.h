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
	void RemoveAllUsers();
	User* GetUserByConnection(TCPConnection::pointer connection);
	User* GetUserByUserID(unsigned long userID);
	void RemoveUserByConnection(TCPConnection::pointer connection);
	bool IsUserLoggedIn(User* user);
	bool SendLoginPackets(User* user, Packet_ReplyType reply = Packet_ReplyType::NoReply);
	void SendFullUserListPacket(TCPConnection::pointer connection);
	void SendAddUserPacketToAll(User* user);
	void SendRemoveUserPacketToAll(User* user);
	void UpdateChannelNumPlayers();
	void OnMinuteTick();

private:
	vector<User*> _users;
};

extern UserManager userManager;