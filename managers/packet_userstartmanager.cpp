#include "packet_userstartmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_UserStartManager packet_UserStartManager;

void Packet_UserStartManager::ParsePacket_UserStart(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UserStartManager ] Client ({}) has sent Packet_UserStart, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UserStartManager ] Parsing Packet_UserStart from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();
	unsigned long unk1 = packet->ReadUInt32_LE();
	const string& unk2 = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_UserStartManager ] User ({}) has sent Packet_UserStart - type: {}, unk1: {}, unk2: {}\n", connection->GetIPAddress(), type, unk1, unk2));
}

void Packet_UserStartManager::SendPacket_UserStart(const GameUser& gameUser) {
	if (gameUser.user == NULL) {
		return;
	}

	auto connection = gameUser.user->GetConnection();
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