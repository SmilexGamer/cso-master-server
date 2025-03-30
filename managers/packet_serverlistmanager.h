#pragma once
#include "user.h"
#include "serverconfig.h"

enum Packet_LobbyType {
	FullUserList = 0,
	AddUser = 1,
	RemoveUser = 2
};

class Packet_ServerListManager {
public:
	void ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet);
	void SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers);
	void SendPacket_Lobby_FullUserList(TCPConnection::pointer connection, const vector<GameUser>& gameUsers);
	void SendPacket_Lobby_AddUser(TCPConnection::pointer connection, const GameUser& gameUser);
	void SendPacket_Lobby_RemoveUser(TCPConnection::pointer connection, unsigned long userID);

private:
	void buildGameUser(TCPConnection::Packet::pointer packet, const GameUser& gameUser);
};

extern Packet_ServerListManager packet_ServerListManager;