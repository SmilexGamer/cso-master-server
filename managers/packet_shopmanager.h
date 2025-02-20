#pragma once
#include "tcp_connection.h"

class Packet_ShopManager {
public:
	void ParsePacket_Shop(TCPConnection::Packet::pointer packet);
	void SendPacket_Shop_Unk0(TCPConnection::pointer connection);
};

extern Packet_ShopManager packet_ShopManager;