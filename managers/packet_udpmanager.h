#pragma once
#include "tcp_connection.h"

class Packet_UdpManager {
public:
	void ParsePacket_Udp(TCPConnection::Packet::pointer packet);
};

extern Packet_UdpManager packet_UdpManager;