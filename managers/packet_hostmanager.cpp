#include "packet_hostmanager.h"
#include <iostream>

Packet_HostManager packet_HostManager;

void Packet_HostManager::ParsePacket_Host(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_HostManager] Parsing Packet_Host from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		default: {
			cout << format("[Packet_HostManager] Client ({}) has sent unregistered Packet_Host type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}

void Packet_HostManager::SendPacket_Host_StartGame(TCPConnection::pointer connection, unsigned long userID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Host });

	packet->WriteUInt8(Packet_HostType::StartGame);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}

void Packet_HostManager::SendPacket_Host_JoinGame(TCPConnection::pointer connection, unsigned long userID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Host });

	packet->WriteUInt8(Packet_HostType::JoinGame);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}