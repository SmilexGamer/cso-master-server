#include "packet_serverlistmanager.h"
#include <iostream>

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_ServerList(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_ServerListManager] Parsing Packet_ServerList from {}\n", packet->GetConnection()->GetEndPoint());

	string unk1 = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk2 = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto c : hardwareID) {
		hardwareIDStr += format(" {:#x}", c & 0xFF);
	}

	cout << format("[Packet_ServerListManager] Client ({}) has sent Packet_ServerList - unk1: {}, hardwareID:{}, pcBang: {}, unk2: {}\n", packet->GetConnection()->GetEndPoint(), unk1.c_str(), hardwareIDStr.c_str(), pcBang, unk2 & 0xFF);
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

		for (unsigned char j = 0; j < 1; j++) {
			packet->WriteUInt8(j + 1); // ChannelID
			packet->WriteString(format("Channel {}-{}", i + 1, j + 1)); // ChannelName
			packet->WriteUInt16_LE(0); // ChannelStatus, 0 - Smooth, 1 - Normal, 2 - Crowded, 3 - Full
		}
	}

	packet->Send();
}