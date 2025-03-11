#pragma once
#include "tcp_connection.h"

class Packet_TransferManager {
public:
	void ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet);
	void ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet);
};

extern Packet_TransferManager packet_TransferManager;