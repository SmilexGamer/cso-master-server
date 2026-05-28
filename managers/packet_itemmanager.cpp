#include "packet_itemmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ItemManager packet_ItemManager;

void Packet_ItemManager::ParsePacket_Item(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ItemManager ] Client ({}) has sent Packet_Item, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_ItemManager ] Parsing Packet_Item from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_ItemType::SelectClass: {
			unsigned char classItemID = packet->ReadUInt8();

			serverConsole.Print(PrefixType::Info, format("[ Packet_ItemManager ] User ({}) has sent Packet_Item SelectClass - classItemID: {}\n", user->GetUserLogName(), classItemID));
			break;
		}
		case Packet_ItemType::BuyWeapon: {
			unsigned char primaryItemID = packet->ReadUInt8();
			unsigned char secondaryItemID = packet->ReadUInt8();

			serverConsole.Print(PrefixType::Info, format("[ Packet_ItemManager ] User ({}) has sent Packet_Item BuyWeapon - primaryItemID: {}, secondaryItemID: {}\n", user->GetUserLogName(), primaryItemID, secondaryItemID));
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_ItemManager ] User ({}) has sent unregistered Packet_Item type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}