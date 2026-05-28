#pragma once
#include "tcp_connection.h"

enum Packet_ItemType {
	SelectClass = 1,
	BuyWeapon = 2
};

class Packet_ItemManager {
public:
	void ParsePacket_Item(TCPConnection::Packet::pointer packet);
};

extern Packet_ItemManager packet_ItemManager;