#include "Packet_VersionManager.h"
#include "definitions.h"
#include <iostream>

void Packet_VersionManager::ParsePacket_Version(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_VersionManager] Parsing Packet_Version from {}\n", packet->GetConnection()->GetEndPoint());

	unsigned char launcherVersion = packet->ReadUInt8();
	unsigned short clientVersion = packet->ReadUInt16_LE();
	unsigned long clientCRC = packet->ReadUInt32_LE();
	unsigned long clientNARCRC = packet->ReadUInt32_LE();

	cout << format("[Packet_VersionManager] Client ({}) has sent Packet_Version - launcherVersion: {}, clientVersion: {}, clientCRC: {}, clientNARCRC: {}\n", packet->GetConnection()->GetEndPoint(), launcherVersion & 0xFF, clientVersion, clientCRC, clientNARCRC);

	SendPacket_Version(packet->GetConnection());
}

void Packet_VersionManager::SendPacket_Version(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Version });
	packet->Send();
}