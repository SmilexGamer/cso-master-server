#pragma once
#include "tcp_connection.h"

class Packet_LoginManager {
public:
	void ParsePacket_Login(TCPConnection::Packet::pointer packet);
};

extern Packet_LoginManager packet_LoginManager;