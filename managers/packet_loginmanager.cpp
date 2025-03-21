#include "packet_loginmanager.h"
#include "packetmanager.h"
#include "packet_charactermanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include "databasemanager.h"
#include <iostream>

Packet_LoginManager packet_LoginManager;

void Packet_LoginManager::ParsePacket_Login(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user != NULL) {
		cout << format("[Packet_LoginManager] Client ({}) has sent Packet_Login, but it's already logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_LoginManager] Parsing Packet_Login from client ({})\n", packet->GetConnection()->GetEndPoint());

	string userName = packet->ReadString();
	string password = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_LoginManager] Client ({}) has sent Packet_Login - userName: {}, password: {}, hardwareID:{}, pcBang: {}\n", packet->GetConnection()->GetEndPoint(), userName.c_str(), password.c_str(), hardwareIDStr.c_str(), pcBang);

	if (userManager.GetUsers().size() >= serverConfig.maxPlayers) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::EXCEED_MAX_CONNECTION);
		return;
	}

	LoginResult loginResult = databaseManager.Login(userName, password);

	if (loginResult.reply > Packet_ReplyType::LoginSuccess) {
		packetManager.SendPacket_Reply(packet->GetConnection(), loginResult.reply);
		return;
	}

	User* newUser = new User(packet->GetConnection(), loginResult.userID, userName);
	char userResult = userManager.AddUser(newUser);
	if (!userResult) {
		if (userResult < 0) {
			packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::Playing);
		return;
	}

	char userCharacterExistsResult = newUser->IsUserCharacterExists();
	if (!userCharacterExistsResult) {
		if (userCharacterExistsResult < 0) {
			packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::LoginSuccess);
		packet_CharacterManager.SendPacket_Character(newUser->GetConnection());
		return;
	}

	userManager.SendLoginPackets(newUser, Packet_ReplyType::LoginSuccess);
}