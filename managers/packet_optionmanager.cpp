#include "packet_optionmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "databasemanager.h"
#include "serverconsole.h"

Packet_OptionManager packet_OptionManager;

void Packet_OptionManager::ParsePacket_Option(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrintType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}
	unsigned char type = packet->ReadUInt8();
	switch (type) {
		case 0: {
			unsigned short dataLength = packet->ReadUInt16_LE();
			if (dataLength == 0 || dataLength > 4096) {
				serverConsole.Print(PrintType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option - type: {}, but dataLength is invalid!\n", user->GetUserIPAddress(), type));
				return;
			}
			std::vector<unsigned char> data = packet->ReadArray_UInt8(dataLength);
			if (data.size() != dataLength) {
				serverConsole.Print(PrintType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option - type: {}, but data is invalid!\n", user->GetUserIPAddress(), type));
				return;
			}
			if (databaseManager.SaveUserOptionData(user->GetUserID(), data)) {
				serverConsole.Print(PrintType::Info, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option - type: {}, data saved successfully!\n", user->GetUserIPAddress(), type));
			} else {
				serverConsole.Print(PrintType::Error, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option - type: {}, failed to save data!\n", user->GetUserIPAddress(), type));
			}
			break;
		}
		default: {
			serverConsole.Print(PrintType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option - type: {}. Unknown type!\n", user->GetUserIPAddress(), type));
			break;
		}
	}
}

void Packet_OptionManager::SendPacket_OptionData(const GameUser& gameUser) {
	std::vector<unsigned char> data = databaseManager.GetUserOptionData(gameUser.user->GetUserID());
	if (data.empty()) {
		serverConsole.Print(PrintType::Error, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option, but data is empty!\n", gameUser.user->GetConnection()->GetIPAddress()));
		return;
	}
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, gameUser.user->GetConnection(), { (unsigned char)PacketID::Option });
	packet->WriteUInt8(0);
	packet->WriteUInt16_LE(data.size());
	packet->WriteArray_UInt8(data);
	packet->Send();
}