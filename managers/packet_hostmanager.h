#pragma once
#include "tcp_connection.h"

class Packet_HostManager {
public:
	void ParsePacket_Host(TCPConnection::Packet::pointer packet);
	void SendPacket_Host_StartGame(TCPConnection::pointer connection, int userID);
	void SendPacket_Host_JoinGame(TCPConnection::pointer connection, int userID);
};

extern Packet_HostManager packet_HostManager;