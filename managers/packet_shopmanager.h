#pragma once
#include "tcp_connection.h"

enum Packet_ShopType {
	Unk0 = 0
};

class Packet_ShopManager {
public:
	void ParsePacket_Shop(TCPConnection::Packet::pointer packet);

private:
	void sendPacket_Shop_Unk0(TCPConnection::pointer connection);
};

extern Packet_ShopManager packet_ShopManager;