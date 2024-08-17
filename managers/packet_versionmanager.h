#pragma once
#include "tcp_connection.h"

class Packet_VersionManager {
public:
	void ParsePacket_Version(TCPConnection::Packet::pointer);
	void SendPacket_Version(TCPConnection::pointer);
};