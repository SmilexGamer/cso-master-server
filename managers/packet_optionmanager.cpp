#include "packet_optionmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_OptionManager packet_OptionManager;

void Packet_OptionManager::ParsePacket_Option(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] Parsing Packet_Option from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_OptionType::UserOption: {
			unsigned short size = packet->ReadUInt16_LE();

			if (size == 0 || size > OPTION_MAX_SIZE) {
				serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserOption, but its size is invalid: {}!\n", user->GetUserLogName(), size));
				return;
			}

			const vector<unsigned char>& userOption = packet->ReadArray_UInt8(size);

			if (user->SaveUserOption(userOption)) {
				serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserOption, userOption saved successfully!\n", user->GetUserLogName()));
			}
			else {
				serverConsole.Print(PrefixType::Error, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserOption, failed to save userOption!\n", user->GetUserLogName()));
			}

			break;
		}
		case Packet_OptionType::Unk1: {
			unsigned long unk1 = packet->ReadUInt32_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option - type: {}, unk1: {}\n", user->GetUserLogName(), type, unk1));
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] User ({}) has sent unregistered Packet_OptionManager type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_OptionManager::SendPacket_Option_UserOption(TCPConnection::pointer connection, const vector<unsigned char>& userOption) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Option });

	packet->WriteUInt8(Packet_OptionType::UserOption);
	packet->WriteUInt16_LE((unsigned short)userOption.size());
	packet->WriteArray_UInt8(userOption);

	packet->Send();
}