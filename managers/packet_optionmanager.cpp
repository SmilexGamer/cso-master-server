#include "packet_optionmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_OptionManager packet_OptionManager;

void Packet_OptionManager::ParsePacket_Option(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] Client ({}) has sent Packet_Option, but it's not logged in!\n", connection->GetLogEndpoint()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] Parsing Packet_Option from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_OptionType::UserConfig: {
			parsePacket_Option_UserConfig(user, packet);
			break;
		}
		case Packet_OptionType::UserQuickStart: {
			parsePacket_Option_UserQuickStart(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] User ({}) has sent unregistered Packet_OptionManager type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_OptionManager::SendPacket_Option_UserConfig(TCPConnection::pointer connection, const vector<unsigned char>& userConfig) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Option });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_OptionType::UserConfig);
	packet->WriteUInt16_LE((unsigned short)userConfig.size());
	packet->WriteArray_UInt8(userConfig);

	packet->Send();
}

void Packet_OptionManager::SendPacket_Option_UserQuickStart(TCPConnection::pointer connection, unsigned long userQuickStart) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Option });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_OptionType::UserQuickStart);
	packet->WriteUInt32_LE(userQuickStart);

	packet->Send();
}

void Packet_OptionManager::parsePacket_Option_UserConfig(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	unsigned short size = packet->ReadUInt16_LE();

	if (size == 0 || size > USERCONFIG_MAX_SIZE) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserConfig, but its size is invalid: {}!\n", user->GetUserLogName(), size));
		return;
	}

	const vector<unsigned char>& userConfig = packet->ReadArray_UInt8(size);

	if (user->SaveUserConfig(userConfig)) {
		serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserConfig, userConfig saved successfully!\n", user->GetUserLogName()));
	}
	else {
		serverConsole.Print(PrefixType::Error, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserConfig, failed to save userConfig!\n", user->GetUserLogName()));
	}
}

void Packet_OptionManager::parsePacket_Option_UserQuickStart(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	unsigned long userQuickStart = packet->ReadUInt32_LE();

	if (user->SaveUserQuickStart(userQuickStart)) {
		serverConsole.Print(PrefixType::Info, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserQuickStart, userQuickStart saved successfully!\n", user->GetUserLogName()));
	}
	else {
		serverConsole.Print(PrefixType::Error, format("[ Packet_OptionManager ] User ({}) has sent Packet_Option UserQuickStart, failed to save userQuickStart!\n", user->GetUserLogName()));
	}
}