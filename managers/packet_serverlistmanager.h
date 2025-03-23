#pragma once
#include "tcp_connection.h"
#include "user.h"

class Packet_ServerListManager {
public:
	void ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet);
	void SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers);
	void SendPacket_Lobby_UserList(TCPConnection::pointer connection, const vector<UserFull>& users);
	void SendPacket_RoomList(TCPConnection::pointer connection);
};

extern Packet_ServerListManager packet_ServerListManager;