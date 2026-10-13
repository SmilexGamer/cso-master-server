#include "packet_favoritemanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_FavoriteManager packet_FavoriteManager;

void Packet_FavoriteManager::ParsePacket_Favorite(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] Client ({}) has sent Packet_Favorite, but it's not logged in!\n", connection->GetLogEndpoint()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] Parsing Packet_Favorite from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_FavoriteType::UserBuyMenu: {
			parsePacket_Favorite_UserBuyMenu(user, packet);
			break;
		}
		case Packet_FavoriteType::UserBookMark: {
			parsePacket_Favorite_UserBookMark(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent unregistered Packet_Favorite type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_FavoriteManager::SendPacket_Favorite_UserBuyMenu(TCPConnection::pointer connection, const vector<BuyMenu>& userBuyMenus) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Favorite });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_FavoriteType::UserBuyMenu);

	for (auto& buyMenu : userBuyMenus) {
		packet->WriteUInt8(buyMenu.categoryID);

		for (auto& itemID : buyMenu.items) {
			packet->WriteUInt8(itemID);
		}
	}

	packet->Send();
}

void Packet_FavoriteManager::SendPacket_Favorite_UserBookMark(TCPConnection::pointer connection, const vector<BookMark>& userBookMarks) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Favorite });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_FavoriteType::UserBookMark);

	for (auto& bookMark : userBookMarks) {
		packet->WriteUInt8(bookMark.slotID);
		packet->WriteString(bookMark.name);
		packet->WriteUInt8(bookMark.primaryItemID);
		packet->WriteBool(bookMark.primaryAmmo);
		packet->WriteUInt8(bookMark.secondaryItemID);
		packet->WriteBool(bookMark.secondaryAmmo);
		packet->WriteUInt8(bookMark.flashbang);
		packet->WriteBool(bookMark.hegrenade);
		packet->WriteBool(bookMark.smokegrenade);
		packet->WriteBool(bookMark.defusekit);
		packet->WriteBool(bookMark.nightvision);
		packet->WriteUInt8(bookMark.kevlar);
		packet->WriteUInt8(bookMark.unk1);
	}

	packet->Send();
}

void Packet_FavoriteManager::parsePacket_Favorite_UserBuyMenu(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	unsigned char categoryID = packet->ReadUInt8();

	if (categoryID >= BUYMENU_MAX_CATEGORY * 2) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, but its categoryID is invalid: {}!\n", user->GetUserLogName(), categoryID));
		return;
	}

	unsigned char slotID = packet->ReadUInt8();

	if (slotID >= BUYMENU_MAX_SLOT) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, but its slotID is invalid: {}!\n", user->GetUserLogName(), slotID));
		return;
	}

	unsigned char itemID = packet->ReadUInt8();

	if (user->SaveUserBuyMenu(categoryID, slotID, itemID)) {
		serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, userBuyMenu saved successfully!\n", user->GetUserLogName()));
	}
	else {
		serverConsole.Print(PrefixType::Error, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, failed to save userBuyMenu!\n", user->GetUserLogName()));
	}
}

void Packet_FavoriteManager::parsePacket_Favorite_UserBookMark(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	unsigned char slotID = packet->ReadUInt8();

	if (slotID >= BOOKMARK_MAX_SLOT) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, but its slotID is invalid: {}!\n", user->GetUserLogName(), slotID));
		return;
	}

	string name = packet->ReadString();

	if (name.size() > BOOKMARK_NAME_MAX_SIZE) {
		name.resize(BOOKMARK_NAME_MAX_SIZE);
	}

	unsigned char primaryItemID = packet->ReadUInt8();
	bool primaryAmmo = packet->ReadBool();
	unsigned char secondaryItemID = packet->ReadUInt8();
	bool secondaryAmmo = packet->ReadBool();
	unsigned char flashbang = packet->ReadUInt8();

	if (flashbang > BOOKMARK_FLASHBANG_MAX) {
		flashbang = BOOKMARK_FLASHBANG_MAX;
	}

	bool hegrenade = packet->ReadBool();
	bool smokegrenade = packet->ReadBool();
	bool defusekit = packet->ReadBool();
	bool nightvision = packet->ReadBool();
	unsigned char kevlar = packet->ReadUInt8();

	if (kevlar > BOOKMARK_KEVLAR_MAX) {
		kevlar = BOOKMARK_KEVLAR_MAX;
	}

	unsigned char unk1 = packet->ReadUInt8();

	BookMark userBookMark{ slotID, name, primaryItemID, primaryAmmo, secondaryItemID, secondaryAmmo, flashbang, hegrenade, smokegrenade, defusekit, nightvision, kevlar, unk1 };

	if (user->SaveUserBookMark(userBookMark)) {
		serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, userBookMark saved successfully!\n", user->GetUserLogName()));
	}
	else {
		serverConsole.Print(PrefixType::Error, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, failed to save UserBookMark!\n", user->GetUserLogName()));
	}
}