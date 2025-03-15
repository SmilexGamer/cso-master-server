#pragma once
#include "tcp_connection.h"
#include "user.h"

class Packet_UpdateInfoManager {
public:
	void ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet);
	void SendPacket_UpdateInfo(TCPConnection::pointer connection, unsigned long userID, UserCharacter& userCharacter);
};

extern Packet_UpdateInfoManager packet_UpdateInfoManager;