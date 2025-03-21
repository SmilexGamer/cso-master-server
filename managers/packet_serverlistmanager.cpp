#include "packet_serverlistmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include <iostream>

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestServerList, but it's not logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestServerList\n", user->GetConnection()->GetEndPoint());

	SendPacket_ServerList(user->GetConnection(), serverConfig.serverList);
}

void Packet_ServerListManager::SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::ServerList });

	packet->WriteUInt8((unsigned char)servers.size()); // Num. of Servers

	for (auto& server : servers) {
		packet->WriteUInt8(server.id); // ServerID
		packet->WriteBool(server.status); // ServerStatus, 0 - Not ready, 1 - Ready
		packet->WriteUInt8(server.type); // ServerType, 0 - Normal server, 1 - Newbie server
		packet->WriteString(server.name); // ServerName

		packet->WriteUInt8((unsigned char)server.channels.size()); // Num. of Channels

		for (auto& channel : server.channels) {
			packet->WriteUInt8(channel.id); // ChannelID
			packet->WriteString(channel.name); // ChannelName
			packet->WriteUInt16_LE(channel.numPlayers); // ChannelStatus, 0~199 - Smooth, 200~399 - Normal, 400~599 - Crowded, >=600 - Full
		}
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby(TCPConnection::pointer connection, const vector<UserFull>& users) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Lobby });

	packet->WriteUInt8(0); // Packet_Lobby ID 0
	packet->WriteUInt16_LE((unsigned short)users.size()); // Num. of Users

	for (auto& userFull : users) {
		packet->WriteUInt32_LE(userFull.user->GetUserID());
		packet->WriteString(userFull.user->GetUserName());
		packetManager.BuildUserCharacter(packet, userFull.userCharacter);
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_RoomList(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::RoomList });

	packet->WriteUInt8(0); // Packet_RoomList ID 0
	packet->WriteUInt16_LE(1); // Num. of Rooms

	for (unsigned short i = 0; i < 1; i++) {
		packet->WriteUInt16_LE(1); // roomID
		packet->WriteUInt16_LE(0xFFFF); // flag

		if (0xFFFF & 0x1) {
			packet->WriteString("roomName"); // roomName
		}
		if (0xFFFF & 0x2) {
			packet->WriteUInt8(1); // unk
		}
		if (0xFFFF & 0x4) {
			packet->WriteUInt8(0); // isPassworded
		}
		if (0xFFFF & 0x8) {
			packet->WriteUInt8(0); // levelLimit
		}
		if (0xFFFF & 0x10) {
			packet->WriteUInt8(0); // gamemodeID
		}
		if (0xFFFF & 0x20) {
			packet->WriteUInt8(0); // mapID
		}
		if (0xFFFF & 0x40) {
			packet->WriteUInt8(0); // players
		}
		if (0xFFFF & 0x80) {
			packet->WriteUInt8(32); // maxPlayer
		}
		if (0xFFFF & 0x100) {
			packet->WriteUInt8(0); // weaponLimit
		}
		if (0xFFFF & 0x200) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt8(0); // unk
		}
		if (0xFFFF & 0x400) {
			packet->WriteUInt8(0); // unk
		}
		if (0xFFFF & 0x800) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt8(0); // unk
		}
		if (0xFFFF & 0x1000) {
			packet->WriteUInt8(0); // unk
		}
		if (0xFFFF & 0x2000) {
			packet->WriteUInt8(0); // unk
		}
		if (0xFFFF & 0x4000) {
			packet->WriteUInt8(0); // unk
		}
	}

	packet->Send();
}