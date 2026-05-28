#include "packet_shopmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ShopManager packet_ShopManager;

void Packet_ShopManager::ParsePacket_Shop(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ShopManager ] Client ({}) has sent Packet_Shop, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_ShopManager ] Parsing Packet_Shop from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_ShopType::Unk0: {
			sendPacket_Shop_Unk0(connection);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_ShopManager ] User ({}) has sent unregistered Packet_Shop type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_ShopManager::sendPacket_Shop_Unk0(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Shop });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_ShopType::Unk0);
	packet->WriteUInt8(0); // Num. of products

	for (unsigned char i = 0; i < 0; i++) {
		packet->WriteUInt32_LE(0); // itemID (actually it's productID, but most items are itemID)
		packet->WriteUInt8(0); // currency; 0 - cash, 1 - point, 2 - clan point, 3+ - crashes client
		packet->WriteUInt8(0); // Num. of offers

		for (unsigned char j = 0; j < 0; j++) {
			packet->WriteUInt32_LE(0); // itemID
			packet->WriteUInt16_LE(0); // duration, 0 - permanent
			packet->WriteUInt16_LE(0); // if permanent, this is count
			packet->WriteUInt32_LE(0); // price
		}
	}

	packet->Send();
}