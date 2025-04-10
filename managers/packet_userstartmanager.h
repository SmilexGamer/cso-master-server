#pragma once
#include "user.h"

class Packet_UserStartManager {
public:
	void ParsePacket_UserStart(TCPConnection::Packet::pointer packet);
	void SendPacket_UserStart(const GameUser& gameUser);
};

extern Packet_UserStartManager packet_UserStartManager;