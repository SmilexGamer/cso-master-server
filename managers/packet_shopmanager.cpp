#include "packet_shopmanager.h"
#include "packetmanager.h"
#include <iostream>

Packet_ShopManager packet_ShopManager;

void Packet_ShopManager::ParsePacket_Shop(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_ShopManager] Parsing Packet_Shop from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_ShopType::Unk0: {
			sendPacket_Shop_Unk0(packet->GetConnection());
			break;
		}
		default: {
			cout << format("[Packet_ShopManager] Client ({}) has sent unregistered Packet_Shop type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}

void Packet_ShopManager::sendPacket_Shop_Unk0(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Shop });

	packet->WriteUInt8(Packet_ShopType::Unk0);
	packet->WriteUInt8(0); // Num. of products

	for (unsigned char i = 0; i < 0; i++) {
		packet->WriteUInt32_LE(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk

		for (unsigned char j = 0; j < 0; j++) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt32_LE(0); // unk
		}
	}

	packet->Send();
}