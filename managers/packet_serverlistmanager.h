#pragma once
#include "tcp_connection.h"

class Packet_ServerListManager {
public:
	void ParsePacket_ServerList(TCPConnection::Packet::pointer packet);
	void SendPacket_ServerList(TCPConnection::pointer connection);
};

extern Packet_ServerListManager packet_ServerListManager;