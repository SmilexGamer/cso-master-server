#pragma once
#include "tcp_connection.h"

#define LAUNCHER_VERSION 67

#define CLIENT_VERSION 11

#define CLIENT_BUILD_TIMESTAMP "11.06.08"

#define CLIENT_NAR_CHECKSUM 1860010506

class Packet_VersionManager {
public:
	void ParsePacket_Version(TCPConnection::Packet::pointer packet);

private:
	void sendPacket_Version(TCPConnection::pointer connection);
};

extern Packet_VersionManager packet_VersionManager;