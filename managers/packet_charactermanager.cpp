#include "packet_charactermanager.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "serverconfig.h"
#include <iostream>

Packet_CharacterManager packet_CharacterManager;

void Packet_CharacterManager::ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_CharacterManager] Client ({}) has sent Packet_RecvCharacter, but it's not logged in\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_CharacterManager] Parsing Packet_RecvCharacter from client ({})\n", user->GetUserIPAddress());

	string nickName = packet->ReadString();

	cout << format("[Packet_CharacterManager] Client ({}) has sent Packet_RecvCharacter - nickName: {}\n", user->GetUserIPAddress(), nickName);

	char userCharacterExistsResult = user->IsUserCharacterExists();
	if (userCharacterExistsResult < 0) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
		return;
	}
	else if (userCharacterExistsResult) {
		return;
	}

	if (nickName.size() < 4) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ID_TOO_SHORT);
		return;
	}

	if (nickName.size() > 16) {
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
		if (count(nickName.begin(), nickName.end(), c) > 3) {
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

	char createCharacterResult = user->CreateCharacter(nickName);
	if (!createCharacterResult) {
		if (createCharacterResult < 0) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::ALREADY_EXIST);
		return;
	}

	userManager.SendLoginPackets(user, Packet_ReplyType::CreateCharacterSuccess);
}

void Packet_CharacterManager::SendPacket_Character(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Character });

	packet->Send();
}