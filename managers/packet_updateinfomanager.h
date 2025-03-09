#pragma once
#include "tcp_connection.h"
#include "gameuser.h"

class Packet_UpdateInfoManager {
public:
	void ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet);
	void SendPacket_UpdateInfo(TCPConnection::pointer connection, GameUser gameUser);
};

extern Packet_UpdateInfoManager packet_UpdateInfoManager;