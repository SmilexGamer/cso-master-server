#include "packet_charactermanager.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "serverconfig.h"
#include "serverconsole.h"

Packet_CharacterManager packet_CharacterManager;

void Packet_CharacterManager::ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (user == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_CharacterManager ] Client ({}) has sent Packet_RecvCharacter, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	if (user->GetUserStatus() != UserStatus::InLogin) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_CharacterManager ] User ({}) has sent Packet_RecvCharacter, but it's not in character creation!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_CharacterManager ] Parsing Packet_RecvCharacter from user ({})\n", user->GetUserLogName()));

	const string& nickName = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_CharacterManager ] User ({}) has sent Packet_RecvCharacter - nickName: {}\n", user->GetUserLogName(), nickName));

	char userCharacterExistsResult = user->IsUserCharacterExists();
	if (userCharacterExistsResult < 0) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}
	else if (userCharacterExistsResult) {
		return;
	}

	if (nickName.size() < NICKNAME_MIN_SIZE) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ID_TOO_SHORT);
		return;
	}

	if (nickName.size() > NICKNAME_MAX_SIZE) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ID_TOO_LONG);
		return;
	}

	if (nickName.find_first_of("!@#$%^&*()_+-=[]{}|;':\",.<>\\/?`~ ") != string::npos) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::INVALID_CHAR);
		return;
	}

	size_t first_number_pos = nickName.find_first_of("0123456789");
	size_t last_char_pos = nickName.find_last_not_of("0123456789");

	if (first_number_pos != string::npos && last_char_pos > first_number_pos) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ID_DIGIT_BEFORE_CHAR);
		return;
	}

	for (auto& c : nickName) {
		if (count(nickName.begin(), nickName.end(), c) > NICKNAME_MAX_CHAR_COUNT) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::ID_EXCEED_CHAR_COUNT);
			return;
		}
	}

	string nickNameLower = nickName;
	transform(nickNameLower.begin(), nickNameLower.end(), nickNameLower.begin(),
		[](unsigned char c) { return tolower(c); });

	for (auto& name : serverConfig.prohibitedNames) {
		if (nickNameLower.find(name) != string::npos) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::ID_PROHIBITED);
			return;
		}
	}

	char createUserBuymenusResult = user->CreateUserBuymenus();
	if (!createUserBuymenusResult) {
		if (createUserBuymenusResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	char createUserBookmarksResult = user->CreateUserBookmarks();
	if (!createUserBookmarksResult) {
		if (createUserBookmarksResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	char createUserInventoryResult = user->CreateUserInventory();
	if (!createUserInventoryResult) {
		if (createUserInventoryResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	char createUserCharacterResult = user->CreateUserCharacter(nickName);
	if (!createUserCharacterResult) {
		if (createUserCharacterResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	if (!userManager.SendLoginPackets(user, Packet_ReplyType::CreateCharacterSuccess) && connection) {
		connection->DisconnectClient();
		return;
	}
}

void Packet_CharacterManager::SendPacket_Character(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Character });
	if (packet == NULL) {
		return;
	}

	packet->Send();
}