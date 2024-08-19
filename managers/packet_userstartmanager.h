#pragma once
#include "tcp_connection.h"

class Packet_UserStartManager {
public:
	void ParsePacket_UserStart(TCPConnection::Packet::pointer packet);
	void SendPacket_UserStart(TCPConnection::pointer connection);
};

extern Packet_UserStartManager packet_UserStartManager;