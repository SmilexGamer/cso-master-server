#include "packet_versionmanager.h"
#include "packetmanager.h"
#include <iostream>

Packet_VersionManager packet_VersionManager;

void Packet_VersionManager::ParsePacket_Version(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_VersionManager] Parsing Packet_Version from {}\n", packet->GetConnection()->GetEndPoint());

	unsigned char launcherVersion = packet->ReadUInt8();
	unsigned short clientVersion = packet->ReadUInt16_LE();
	unsigned long clientBuildTimestamp = packet->ReadUInt32_LE();
	unsigned long clientNARCRC = packet->ReadUInt32_LE();

	struct tm date;
	time_t t = clientBuildTimestamp;
	localtime_s(&date, &t);
	static char dateStr[10];
	strftime(dateStr, sizeof(dateStr), "%d.%m.%y", &date);

	cout << format("[Packet_VersionManager] Client ({}) has sent Packet_Version - launcherVersion: {}, clientVersion: {}, clientBuildTimestamp: {}, clientNARCRC: {}\n", packet->GetConnection()->GetEndPoint(), launcherVersion & 0xFF, clientVersion, dateStr, clientNARCRC);

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

	if (clientNARCRC != CLIENT_NAR_CRC) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::INVALID_CLIENT_VERSION);
		return;
	}

	SendPacket_Version(packet->GetConnection());
}

void Packet_VersionManager::SendPacket_Version(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Version });

	packet->Send();
}