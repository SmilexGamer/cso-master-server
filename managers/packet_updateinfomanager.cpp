#include "packet_updateinfomanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		serverConsole.Print(PrintType::Warn, format("[ Packet_UpdateInfoManager ] Client ({}) has sent Packet_UpdateInfo, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrintType::Info, format("[ Packet_UpdateInfoManager ] Parsing Packet_UpdateInfo from client ({})\n", user->GetUserIPAddress()));

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			string unk1 = packet->ReadString();

			serverConsole.Print(PrintType::Info, format("[ Packet_UpdateInfoManager ] Client ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", user->GetUserIPAddress(), type, unk1));
			break;
		}
		case 1: {
			// nothing to read
			serverConsole.Print(PrintType::Info, format("[ Packet_UpdateInfoManager ] Client ({}) has sent Packet_UpdateInfo - type: {}\n", user->GetUserIPAddress(), type));
			break;
		}
		default: {
			serverConsole.Print(PrintType::Warn, format("[ Packet_UpdateInfoManager ] Client ({}) has sent unregistered Packet_UpdateInfo type {}!\n", user->GetUserIPAddress(), type));
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