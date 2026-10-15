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
	void DisconnectUser(User* user);
	void DisconnectAllUsers();
	User* GetUserByConnection(TCPConnection::pointer connection);
	User* GetUserByUserID(unsigned long userID);
	void DisconnectUserByConnection(TCPConnection::pointer connection, const boost::system::error_code& ec = {});
	bool IsUserLoggedIn(User* user);
	void SendLoginPackets(User* user, Packet_ReplyType reply = Packet_ReplyType::NoReply);
	void SendFullUserListPacket(TCPConnection::pointer connection) const noexcept;
	void SendAddUserPacketToAll(User* user) const noexcept;
	void SendRemoveUserPacketToAll(User* user) const noexcept;
	void UpdateChannelNumPlayers();
	void OnMinuteTick();

private:
	vector<User*> _users;
};

extern UserManager userManager;