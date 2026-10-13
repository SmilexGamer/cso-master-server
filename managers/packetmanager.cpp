#include "packetmanager.h"
#include "packet_versionmanager.h"
#include "packet_transfermanager.h"
#include "packet_loginmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_charactermanager.h"
#include "packet_cryptmanager.h"
#include "packet_roommanager.h"
#include "packet_clientcheckmanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_udpmanager.h"
#include "packet_shopmanager.h"
#include "packet_optionmanager.h"
#include "packet_favoritemanager.h"
#include "packet_itemmanager.h"
#include "packet_searchroommanager.h"
#include "packet_reportmanager.h"
#include "serverconsole.h"

PacketManager packetManager;

void PacketManager::ParsePacket(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ PacketManager ] Parsing packet from client ({})\n", connection->GetLogEndpoint()));

	unsigned char packetID = packet->ReadUInt8();

	switch ((PacketID)packetID) {
		case PacketID::Version: {
			packet_VersionManager.ParsePacket_Version(packet);
			break;
		}
		case PacketID::RecvCharacter: {
			packet_CharacterManager.ParsePacket_RecvCharacter(packet);
			break;
		}
		case PacketID::Login: {
			packet_LoginManager.ParsePacket_Login(packet);
			break;
		}
		case PacketID::TransferLogin: {
			packet_TransferManager.ParsePacket_TransferLogin(packet);
			break;
		}
		case PacketID::RequestTransfer: {
			packet_TransferManager.ParsePacket_RequestTransfer(packet);
			break;
		}
		case PacketID::RequestServerList: {
			packet_ServerListManager.ParsePacket_RequestServerList(packet);
			break;
		}
		case PacketID::RecvCrypt: {
			packet_CryptManager.ParsePacket_RecvCrypt(packet);
			break;
		}
		case PacketID::Room: {
			packet_RoomManager.ParsePacket_Room(packet);
			break;
		}
		case PacketID::ClientCheck: {
			packet_ClientCheckManager.ParsePacket_ClientCheck(packet);
			break;
		}
		case PacketID::UMsg: {
			packet_UMsgManager.ParsePacket_UMsg(packet);
			break;
		}
		case PacketID::Host: {
			packet_HostManager.ParsePacket_Host(packet);
			break;
		}
		case PacketID::UpdateInfo: {
			packet_UpdateInfoManager.ParsePacket_UpdateInfo(packet);
			break;
		}
		case PacketID::Udp: {
			packet_UdpManager.ParsePacket_Udp(packet);
			break;
		}
		case PacketID::Shop: {
			packet_ShopManager.ParsePacket_Shop(packet);
			break;
		}
		case PacketID::Option: {
			packet_OptionManager.ParsePacket_Option(packet);
			break;
		}
		case PacketID::Favorite: {
			packet_FavoriteManager.ParsePacket_Favorite(packet);
			break;
		}
		case PacketID::Item: {
			packet_ItemManager.ParsePacket_Item(packet);
			break;
		}
		case PacketID::SearchRoom: {
			packet_SearchRoomManager.ParsePacket_SearchRoom(packet);
			break;
		}
		case PacketID::RecvReport: {
			packet_ReportManager.ParsePacket_RecvReport(packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ PacketManager ] Client ({}) has sent unregistered packet ID {}!\n", connection->GetLogEndpoint(), packetID));
			break;
		}
	}
}

void PacketManager::SendPacket_Reply(TCPConnection::pointer connection, Packet_ReplyType type, const vector<string>& additionalText) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Reply });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(type);
	packet->WriteString(""); // "S_REPLY_OK"? TODO: research this
	packet->WriteUInt8((unsigned char)additionalText.size());
	for (auto& text : additionalText) {
		packet->WriteString(text);
	}

	packet->Send();
}

void PacketManager::SendPacket_UserStart(const GameUser& gameUser) {
	if (gameUser.user == NULL) {
		return;
	}

	auto& connection = gameUser.user->GetConnection();
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::UserStart });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packet->WriteString(gameUser.user->GetUserName());
	packet->WriteString(gameUser.userCharacter.nickName);

	packet->Send();
}

void PacketManager::SendPacket_Inventory(TCPConnection::pointer connection, const vector<InventoryItem>& userInventory) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Inventory });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8((unsigned char)userInventory.size());
	for (auto& item : userInventory) {
		packet->WriteUInt8(item.slotID);
		packet->WriteUInt8(1); // unk, 0 - crash
		packet->WriteUInt8(item.itemID);
		packet->WriteUInt8(item.count);
		packet->WriteBool(item.inUse);
		packet->WriteUInt32_LE(item.obtainTime);
		packet->WriteUInt32_LE(item.expirationTime); // not in use - days, inUse - unix timestamp ; if 0, permanent
	}

	packet->Send();
}

void PacketManager::BuildUserCharacter(TCPConnection::Packet::pointer packet, const UserCharacter& userCharacter) {
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt16_LE(userCharacter.flag);

	if (userCharacter.flag & USERCHARACTER_FLAG_UNK1) {
		packet->WriteUInt8(userCharacter.unk1);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_NICKNAME) {
		packet->WriteString(userCharacter.nickName);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK4) {
		packet->WriteString(userCharacter.unk4_1);
		packet->WriteUInt8(userCharacter.unk4_2);
		packet->WriteUInt8(userCharacter.unk4_3);
		packet->WriteUInt8(userCharacter.unk4_4);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_LEVEL) {
		packet->WriteUInt8(userCharacter.level);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK10) {
		packet->WriteUInt8(userCharacter.unk10);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_EXP) {
		packet->WriteUInt64_LE(userCharacter.exp);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_CASH) {
		packet->WriteUInt64_LE(userCharacter.cash);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_POINTS) {
		packet->WriteUInt64_LE(userCharacter.points);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_BATTLESTATS) {
		packet->WriteUInt32_LE(userCharacter.battles);
		packet->WriteUInt32_LE(userCharacter.wins);
		packet->WriteUInt32_LE(userCharacter.frags);
		packet->WriteUInt32_LE(userCharacter.deaths);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_LOCATION) {
		packet->WriteString(""); // pcBang
		packet->WriteUInt16_LE(userCharacter.city); // province/city (도시) index
		packet->WriteUInt16_LE(userCharacter.county); // district/county (구군) index
		packet->WriteUInt16_LE(userCharacter.neighborhood); // neighborhood (동) index
		packet->WriteString(userCharacter.unk200_5);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK400) {
		packet->WriteUInt32_LE(userCharacter.unk400);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK800) {
		packet->WriteUInt8(userCharacter.unk800);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_CLAN) {
		packet->WriteUInt32_LE(userCharacter.clanID);
		packet->WriteUInt32_LE(userCharacter.clanMarkID);
		packet->WriteString(userCharacter.clanName);
		packet->WriteUInt8(userCharacter.clanUnk4);
		packet->WriteUInt8(userCharacter.clanUnk5);
		packet->WriteUInt8(userCharacter.clanUnk6);
	}
}