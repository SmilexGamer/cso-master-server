#include "packet_serverlistmanager.h"
#include <iostream>

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_ServerList(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_ServerListManager] Parsing Packet_ServerList from client ({})\n", packet->GetConnection()->GetEndPoint());

	string unk1 = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk2 = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_ServerList - unk1: {}, hardwareID:{}, pcBang: {}, unk2: {}\n", packet->GetConnection()->GetEndPoint(), unk1.c_str(), hardwareIDStr.c_str(), pcBang, unk2 & 0xFF);
}

void Packet_ServerListManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet)
{
	cout << format("[Packet_ServerListManager] Parsing Packet_RequestTransfer from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", packet->GetConnection()->GetEndPoint(), serverID & 0xFF, channelID & 0xFF);

	SendPacket_Lobby(packet->GetConnection());
	SendPacket_RoomList(packet->GetConnection());
}

void Packet_ServerListManager::SendPacket_ServerList(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::ServerList });

	packet->WriteUInt8(1); // Num. of Servers

	for (unsigned char i = 0; i < 1; i++) {
		packet->WriteUInt8(i + 1); // ServerID
		packet->WriteUInt8(1); // ServerStatus, 0 - Not ready, 1 - Ready
		packet->WriteUInt8(0); // ServerType, 0 - Normal server, 1 - Newbie server
		packet->WriteString(format("Server {}", i + 1)); // ServerName

		packet->WriteUInt8(1); // Num. of Channels

		for (unsigned char j = 0; j < 2; j++) {
			packet->WriteUInt8(j + 1); // ChannelID
			packet->WriteString(format("Channel {}-{}", i + 1, j + 1)); // ChannelName
			packet->WriteUInt16_LE(0); // ChannelStatus, 0 - Smooth, 1 - Normal, 2 - Crowded, 3 - Full
		}
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Lobby });

	packet->WriteUInt8(0); // Packet_Lobby ID 0
	packet->WriteUInt16_LE(1); // Num. of Users

	for (unsigned short i = 0; i < 1; i++) {
		packet->WriteUInt32_LE(1); // userID
		packet->WriteString("userName"); // userName
		packet->WriteUInt16_LE(UserInfoFlag::All);

		if (UserInfoFlag::All & UserInfoFlag::Unk1) {
			packet->WriteUInt8(0); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::NickName) {
			packet->WriteString("nickName"); // nickName
		}
		if (UserInfoFlag::All & UserInfoFlag::Unk4) {
			packet->WriteString(""); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::Level) {
			packet->WriteUInt8(72); // level
		}
		if (UserInfoFlag::All & UserInfoFlag::Unk10) {
			packet->WriteUInt8(0); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::Exp) {
			packet->WriteUInt64_LE(6937150764); // exp
		}
		if (UserInfoFlag::All & UserInfoFlag::Cash) {
			packet->WriteUInt64_LE(1000); // cash
		}
		if (UserInfoFlag::All & UserInfoFlag::Point) {
			packet->WriteUInt64_LE(2000); // points
		}
		if (UserInfoFlag::All & UserInfoFlag::BattleStats) {
			packet->WriteUInt32_LE(3); // battles
			packet->WriteUInt32_LE(2); // wins
			packet->WriteUInt32_LE(5); // frags
			packet->WriteUInt32_LE(3); // deaths
		}
		if (UserInfoFlag::All & UserInfoFlag::Location) {
			packet->WriteString("pcBang"); // pcBang
			packet->WriteUInt16_LE(1); // province/city (도시) index
			packet->WriteUInt16_LE(1); // district/county (구군) index
			packet->WriteUInt16_LE(1); // neighborhood (동) index
			packet->WriteString(""); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::Unk400) {
			packet->WriteUInt32_LE(0); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::Unk800) {
			packet->WriteUInt8(0); // unk
		}
		if (UserInfoFlag::All & UserInfoFlag::Unk1000) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt32_LE(0); // unk
			packet->WriteString(""); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
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