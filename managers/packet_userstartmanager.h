#pragma once
#include "tcp_connection.h"
#include "user.h"

class Packet_UserStartManager {
public:
	void ParsePacket_UserStart(TCPConnection::Packet::pointer packet);
	void SendPacket_UserStart(User* user, const UserCharacter& userCharacter);
};

extern Packet_UserStartManager packet_UserStartManager;