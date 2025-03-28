#include "packet_serverlistmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestServerList, but it's not logged in!\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestServerList\n", user->GetUserIPAddress());

	SendPacket_ServerList(user->GetConnection(), serverConfig.serverList);
}

void Packet_ServerListManager::SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::ServerList });

	packet->WriteUInt8((unsigned char)servers.size());

	for (auto& server : servers) {
		packet->WriteUInt8(server.id);
		packet->WriteBool(server.status);
		packet->WriteUInt8(server.type);
		packet->WriteString(server.name);

		packet->WriteUInt8((unsigned char)server.channels.size());

		for (auto& channel : server.channels) {
			packet->WriteUInt8(channel.id);
			packet->WriteString(channel.name);
			packet->WriteUInt16_LE(channel.numPlayers);
		}
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby_FullUserList(TCPConnection::pointer connection, const vector<UserFull>& users) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Lobby });

	packet->WriteUInt8(Packet_LobbyType::FullUserList);
	packet->WriteUInt16_LE((unsigned short)users.size());

	for (auto& userFull : users) {
		packet->WriteUInt32_LE(userFull.user->GetUserID());
		packet->WriteString(userFull.user->GetUserName());
		packetManager.BuildUserCharacter(packet, userFull.userCharacter);
	}

	packet->Send();
}