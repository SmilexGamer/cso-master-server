#pragma once
#include "user.h"

class Packet_UpdateInfoManager {
public:
	void ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet);
	void SendPacket_UpdateInfo(const UserFull& user);
};

extern Packet_UpdateInfoManager packet_UpdateInfoManager;