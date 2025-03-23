#pragma once
#include "tcp_connection.h"
#include "user.h"

class Packet_UserStartManager {
public:
	void ParsePacket_UserStart(TCPConnection::Packet::pointer packet);
	void SendPacket_UserStart(const UserFull& userFull);
};

extern Packet_UserStartManager packet_UserStartManager;