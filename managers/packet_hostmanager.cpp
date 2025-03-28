#include "packet_hostmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_HostManager packet_HostManager;

void Packet_HostManager::ParsePacket_Host(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_HostManager] Client ({}) has sent Packet_Host, but it's not logged in!\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_HostManager] Parsing Packet_Host from client ({})\n", user->GetUserIPAddress());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		default: {
			cout << format("[Packet_HostManager] Client ({}) has sent unregistered Packet_Host type {}!\n", user->GetUserIPAddress(), type);
			break;
		}
	}
}

void Packet_HostManager::SendPacket_Host_StartGame(User* user) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, user->GetConnection(), { (unsigned char)PacketID::Host });

	packet->WriteUInt8(Packet_HostType::StartGame);
	packet->WriteUInt32_LE(user->GetUserID());

	packet->Send();
}

void Packet_HostManager::SendPacket_Host_JoinGame(TCPConnection::pointer connection, unsigned long userID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });

	packet->WriteUInt8(Packet_HostType::JoinGame);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}