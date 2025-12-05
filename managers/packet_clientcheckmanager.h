#pragma once
#include "tcp_connection.h"

class Packet_ClientCheckManager {
public:
	void ParsePacket_ClientCheck(TCPConnection::Packet::pointer packet);
	void SendPacket_ClientCheck(TCPConnection::pointer connection);
};

extern Packet_ClientCheckManager packet_ClientCheckManager;