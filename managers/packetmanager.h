#pragma once
#include "user.h"

class PacketManager {
public:
	void ParsePacket(TCPConnection::Packet::pointer packet);
	void SendPacket_Reply(TCPConnection::pointer connection, Packet_ReplyType type, const vector<string>& additionalText = {});
	void SendPacket_UserStart(const GameUser& gameUser);
	void SendPacket_Inventory(TCPConnection::pointer connection, const vector<InventoryItem>& userInventory);
	void BuildUserCharacter(TCPConnection::Packet::pointer packet, const UserCharacter& userCharacter);
};

extern PacketManager packetManager;