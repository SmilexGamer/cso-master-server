#include "packet_updateinfomanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UpdateInfoManager ] Client ({}) has sent Packet_UpdateInfo, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UpdateInfoManager ] Parsing Packet_UpdateInfo from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			const string& unk1 = packet->ReadString();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UpdateInfoManager ] User ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", user->GetUserLogName(), type, unk1));
			break;
		}
		case 1: {
			// nothing to read
			serverConsole.Print(PrefixType::Info, format("[ Packet_UpdateInfoManager ] User ({}) has sent Packet_UpdateInfo - type: {}\n", user->GetUserLogName(), type));
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_UpdateInfoManager ] User ({}) has sent unregistered Packet_UpdateInfo type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_UpdateInfoManager::SendPacket_UpdateInfo(const GameUser& gameUser) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, gameUser.user->GetConnection(), { (unsigned char)PacketID::UpdateInfo });

	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packetManager.BuildUserCharacter(packet, gameUser.userCharacter);

	packet->Send();
}