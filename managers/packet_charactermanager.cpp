#include "packet_charactermanager.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "serverconfig.h"
#include "serverconsole.h"

Packet_CharacterManager packet_CharacterManager;

void Packet_CharacterManager::ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_CharacterManager ] Client ({}) has sent Packet_RecvCharacter, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
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
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
		return;
	}
	else if (userCharacterExistsResult) {
		return;
	}

	if (nickName.size() < NICKNAME_MIN_SIZE) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_TOO_SHORT);
		return;
	}

	if (nickName.size() > NICKNAME_MAX_SIZE) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_TOO_LONG);
		return;
	}

	if (nickName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != string::npos) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::INVALID_CHAR);
		return;
	}

	size_t first_number_pos = nickName.find_first_of("0123456789");
	size_t last_char_pos = nickName.find_last_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

	if (first_number_pos != string::npos && last_char_pos > first_number_pos) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_DIGIT_BEFORE_CHAR);
		return;
	}

	for (auto& c : nickName) {
		if (count(nickName.begin(), nickName.end(), c) > NICKNAME_MAX_CHAR_COUNT) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_EXCEED_CHAR_COUNT);
			return;
		}
	}

	string nickNameLower = nickName;
	transform(nickNameLower.begin(), nickNameLower.end(), nickNameLower.begin(),
		[](unsigned char c) { return tolower(c); });

	for (auto& name : serverConfig.prohibitedNames) {
		if (nickNameLower.find(name) != string::npos) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_PROHIBITED);
			return;
		}
	}

	char createUserCharacterResult = user->CreateUserCharacter(nickName);
	if (!createUserCharacterResult) {
		if (createUserCharacterResult < 0) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	userManager.SendLoginPackets(user, Packet_ReplyType::CreateCharacterSuccess);
}

void Packet_CharacterManager::SendPacket_Character(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Character });

	packet->Send();
}