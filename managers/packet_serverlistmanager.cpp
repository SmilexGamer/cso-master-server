﻿#include "packet_serverlistmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_ServerList(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_ServerListManager] Parsing Packet_ServerList from client ({})\n", packet->GetConnection()->GetEndPoint());

	string unk1 = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk2 = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_ServerList - unk1: {}, hardwareID:{}, pcBang: {}, unk2: {}\n", packet->GetConnection()->GetEndPoint(), unk1.c_str(), hardwareIDStr.c_str(), pcBang, unk2);
}

void Packet_ServerListManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet)
{
	cout << format("[Packet_ServerListManager] Parsing Packet_RequestTransfer from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", packet->GetConnection()->GetEndPoint(), serverID, channelID);

	SendPacket_Lobby(packet->GetConnection(), userManager.GetUsers());
	SendPacket_RoomList(packet->GetConnection());
}

void Packet_ServerListManager::SendPacket_ServerList(TCPConnection::pointer connection, vector<Server> servers) {
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

void Packet_ServerListManager::SendPacket_Lobby(TCPConnection::pointer connection, vector<User*> users) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Lobby });

	packet->WriteUInt8(0); // Packet_Lobby ID 0
	packet->WriteUInt16_LE((unsigned short)users.size()); // Num. of Users

	for (auto& user : users) {
		GameUser gameUser = user->GetGameUser();

		packet->WriteUInt32_LE(user->GetUserID()); // userID
		packet->WriteString(user->GetUserName()); // userName
		packet->WriteUInt16_LE(gameUser.flag);

		if (gameUser.flag & UserInfoFlag::Unk1) {
			packet->WriteUInt8(gameUser.unk1); // unk
		}
		if (gameUser.flag & UserInfoFlag::NickName) {
			packet->WriteString(gameUser.nickName); // nickName
		}
		if (gameUser.flag & UserInfoFlag::Unk4) {
			packet->WriteString(gameUser.unk4_1); // unk
			packet->WriteUInt8(gameUser.unk4_2); // unk
			packet->WriteUInt8(gameUser.unk4_3); // unk
			packet->WriteUInt8(gameUser.unk4_4); // unk
		}
		if (gameUser.flag & UserInfoFlag::Level) {
			packet->WriteUInt8(gameUser.level); // level
		}
		if (gameUser.flag & UserInfoFlag::Unk10) {
			packet->WriteUInt8(gameUser.unk10); // unk
		}
		if (gameUser.flag & UserInfoFlag::Exp) {
			packet->WriteUInt64_LE(gameUser.exp); // exp
		}
		if (gameUser.flag & UserInfoFlag::Cash) {
			packet->WriteUInt64_LE(gameUser.cash); // cash
		}
		if (gameUser.flag & UserInfoFlag::Points) {
			packet->WriteUInt64_LE(gameUser.points); // points
		}
		if (gameUser.flag & UserInfoFlag::BattleStats) {
			packet->WriteUInt32_LE(gameUser.battles); // battles
			packet->WriteUInt32_LE(gameUser.wins); // wins
			packet->WriteUInt32_LE(gameUser.frags); // frags
			packet->WriteUInt32_LE(gameUser.deaths); // deaths
		}
		if (gameUser.flag & UserInfoFlag::Location) {
			packet->WriteString(gameUser.pcBang); // pcBang
			packet->WriteUInt16_LE(gameUser.city); // province/city (도시) index
			packet->WriteUInt16_LE(gameUser.county); // district/county (구군) index
			packet->WriteUInt16_LE(gameUser.neighborhood); // neighborhood (동) index
			packet->WriteString(gameUser.unk200_5); // unk
		}
		if (gameUser.flag & UserInfoFlag::Unk400) {
			packet->WriteUInt32_LE(gameUser.unk400); // unk
		}
		if (gameUser.flag & UserInfoFlag::Unk800) {
			packet->WriteUInt8(gameUser.unk800); // unk
		}
		if (gameUser.flag & UserInfoFlag::Unk1000) {
			packet->WriteUInt32_LE(gameUser.unk1000_1); // unk
			packet->WriteUInt32_LE(gameUser.unk1000_2); // unk
			packet->WriteString(gameUser.unk1000_3); // unk
			packet->WriteUInt8(gameUser.unk1000_4); // unk
			packet->WriteUInt8(gameUser.unk1000_5); // unk
			packet->WriteUInt8(gameUser.unk1000_6); // unk
		}
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