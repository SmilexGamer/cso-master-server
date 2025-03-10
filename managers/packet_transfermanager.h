#pragma once
#include "tcp_connection.h"

class Packet_TransferManager {
public:
	void ParsePacket_Transfer(TCPConnection::Packet::pointer packet);
};

extern Packet_TransferManager packet_transferManager;