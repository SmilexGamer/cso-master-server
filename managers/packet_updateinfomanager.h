#pragma once
#include "user.h"

class Packet_UpdateInfoManager {
public:
	void ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet);
	void SendPacket_UpdateInfo(const GameUser& gameUser);
};

extern Packet_UpdateInfoManager packet_UpdateInfoManager;