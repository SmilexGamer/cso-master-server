#include "packet_favoritemanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_FavoriteManager packet_FavoriteManager;

void Packet_FavoriteManager::ParsePacket_Favorite(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] Client ({}) has sent Packet_Favorite, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] Parsing Packet_Favorite from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_FavoriteType::UserBuyMenu: {
			unsigned char categoryID = packet->ReadUInt8();
			unsigned char slotID = packet->ReadUInt8();
			unsigned char itemID = packet->ReadUInt8();

			if (categoryID >= BUYMENU_MAX_CATEGORY * 2) {
				serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, but its categoryID is invalid: {}!\n", user->GetUserLogName(), categoryID));
				return;
			}

			if (categoryID >= BUYMENU_MAX_SLOT) {
				serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, but its slotID is invalid: {}!\n", user->GetUserLogName(), slotID));
				return;
			}

			if (user->SaveUserBuyMenu(categoryID, slotID, itemID)) {
				serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, userBuyMenu saved successfully!\n", user->GetUserLogName()));
			}
			else {
				serverConsole.Print(PrefixType::Error, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBuyMenu, failed to save userBuyMenu!\n", user->GetUserLogName()));
			}

			break;
		}
		case Packet_FavoriteType::UserBookMark: {
			unsigned char slotID = packet->ReadUInt8();
			string name = packet->ReadString();
			unsigned char primaryItemID = packet->ReadUInt8();
			bool primaryAmmo = packet->ReadBool();
			unsigned char secondaryItemID = packet->ReadUInt8();
			bool secondaryAmmo = packet->ReadBool();
			unsigned char flashbang = packet->ReadUInt8();
			bool hegrenade = packet->ReadBool();
			bool smokegrenade = packet->ReadBool();
			bool defusekit = packet->ReadBool();
			bool nightvision = packet->ReadBool();
			unsigned char kevlar = packet->ReadUInt8();
			unsigned char unk1 = packet->ReadUInt8();

			if (slotID >= BOOKMARK_MAX_SLOT) {
				serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, but its slotID is invalid: {}!\n", user->GetUserLogName(), slotID));
				return;
			}

			if (name.size() > BOOKMARK_NAME_MAX_SIZE) {
				name.resize(BOOKMARK_NAME_MAX_SIZE);
			}

			if (flashbang > 2) {
				flashbang = 2;
			}

			if (kevlar > 2) {
				kevlar = 2;
			}

			BookMark userBookMark { slotID, name, primaryItemID, primaryAmmo, secondaryItemID, secondaryAmmo, flashbang, hegrenade, smokegrenade, defusekit, nightvision, kevlar, unk1 };

			if (user->SaveUserBookMark(userBookMark)) {
				serverConsole.Print(PrefixType::Info, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, userBookMark saved successfully!\n", user->GetUserLogName()));
			}
			else {
				serverConsole.Print(PrefixType::Error, format("[ Packet_FavoriteManager ] User ({}) has sent Packet_Favorite UserBookMark, failed to save UserBookMark!\n", user->GetUserLogName()));
			}

			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_FavoriteManager ] User ({}) has sent unregistered Packet_Favorite type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_FavoriteManager::SendPacket_Favorite_UserBuyMenu(TCPConnection::pointer connection, const vector<BuyMenu>& userBuyMenus) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Favorite });

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
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Favorite });

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