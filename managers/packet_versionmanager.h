#pragma once
#include "tcp_connection.h"

class Packet_VersionManager {
public:
	void ParsePacket_Version(TCPConnection::Packet::pointer packet);
	void SendPacket_Version(TCPConnection::pointer connection);
};

extern Packet_VersionManager packet_VersionManager;