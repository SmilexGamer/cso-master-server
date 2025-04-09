#include "packet_shopmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ShopManager packet_ShopManager;

void Packet_ShopManager::ParsePacket_Shop(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ShopManager ] Client ({}) has sent Packet_Shop, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_ShopManager ] Parsing Packet_Shop from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_ShopType::Unk0: {
			sendPacket_Shop_Unk0(user->GetConnection());
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_ShopManager ] User ({}) has sent unregistered Packet_Shop type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_ShopManager::sendPacket_Shop_Unk0(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Shop });

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