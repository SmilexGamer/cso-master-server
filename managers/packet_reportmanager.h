#pragma once
#include "tcp_connection.h"

class Packet_ReportManager {
public:
	void ParsePacket_RecvReport(TCPConnection::Packet::pointer packet);
};

extern Packet_ReportManager packet_ReportManager;