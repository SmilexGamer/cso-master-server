#pragma once
#include "user.h"

class Packet_TransferManager {
public:
	void ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet);
	void ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet);

private:
	void sendPacket_Transfer(User* user, unsigned long ip, unsigned short port);
};

extern Packet_TransferManager packet_TransferManager;