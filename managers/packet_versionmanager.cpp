#include "packet_versionmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_VersionManager packet_VersionManager;

void Packet_VersionManager::ParsePacket_Version(TCPConnection::Packet::pointer packet) {
	if (packet->GetConnection()->IsVersionReceived()) {
		serverConsole.Print(PrintType::Warn, format("[ Packet_VersionManager ] Client ({}) has sent Packet_Version, but it already sent Packet_Version!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrintType::Warn, format("[ Packet_VersionManager ] Client ({}) has sent Packet_Version, but it's already logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrintType::Info, format("[ Packet_VersionManager ] Parsing Packet_Version from client ({})\n", packet->GetConnection()->GetIPAddress()));

	unsigned char launcherVersion = packet->ReadUInt8();
	unsigned short clientVersion = packet->ReadUInt16_LE();
	unsigned long clientBuildTimestamp = packet->ReadUInt32_LE();
	unsigned long clientNARChecksum = packet->ReadUInt32_LE();

	struct tm date;
	time_t t = clientBuildTimestamp;
	localtime_s(&date, &t);
	char dateStr[9];
	strftime(dateStr, sizeof(dateStr), "%d.%m.%y", &date);

	serverConsole.Print(PrintType::Info, format("[ Packet_VersionManager ] Client ({}) has sent Packet_Version - launcherVersion: {}, clientVersion: {}, clientBuildTimestamp: {}, clientNARChecksum: {}\n", packet->GetConnection()->GetIPAddress(), launcherVersion, clientVersion, dateStr, clientNARChecksum));

	if (launcherVersion != LAUNCHER_VERSION) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::INVALID_CLIENT_VERSION);
		return;
	}

	if (clientVersion != CLIENT_VERSION) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::INVALID_CLIENT_VERSION);
		return;
	}

	if (strcmp(dateStr, CLIENT_BUILD_TIMESTAMP) != 0) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::INVALID_CLIENT_VERSION);
		return;
	}

	if (clientNARChecksum != CLIENT_NAR_CHECKSUM) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::INVALID_CLIENT_VERSION);
		return;
	}

	packet->GetConnection()->SetVersionReceived(true);
	sendPacket_Version(packet->GetConnection());
}

void Packet_VersionManager::sendPacket_Version(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Version });

	packet->Send();
}