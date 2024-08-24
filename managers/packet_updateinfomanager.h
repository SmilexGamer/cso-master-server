#pragma once
#include "tcp_connection.h"

class Packet_UpdateInfoManager {
public:
	void ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet);
	void SendPacket_UpdateInfo(TCPConnection::pointer connection, unsigned short flag);
};

extern Packet_UpdateInfoManager packet_UpdateInfoManager;