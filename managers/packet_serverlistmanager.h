#pragma once
#include "user.h"
#include "serverconfig.h"

enum Packet_LobbyType {
	FullUserList = 0
};

class Packet_ServerListManager {
public:
	void ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet);
	void SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers);
	void SendPacket_Lobby_FullUserList(TCPConnection::pointer connection, const vector<UserFull>& users);
};

extern Packet_ServerListManager packet_ServerListManager;