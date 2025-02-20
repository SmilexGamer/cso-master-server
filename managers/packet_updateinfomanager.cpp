#include "packet_updateinfomanager.h"
#include <iostream>

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UpdateInfoManager] Parsing Packet_UpdateInfo from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			string unk1 = packet->ReadString();

			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", packet->GetConnection()->GetEndPoint(), type & 0xFF, unk1.c_str());
			break;
		}
		case 1: {
			// nothing to read
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}\n", packet->GetConnection()->GetEndPoint(), type & 0xFF);
			break;
		}
		default: {
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent unregistered Packet_UpdateInfo type {}!\n", packet->GetConnection()->GetEndPoint(), type & 0xFF);
			break;
		}
	}
}

void Packet_UpdateInfoManager::SendPacket_UpdateInfo(TCPConnection::pointer connection, unsigned short flag) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::UpdateInfo });

	packet->WriteUInt32_LE(1); // userID
	packet->WriteUInt16_LE(flag); // flag

	if (flag & UserInfoFlag::Unk1) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & UserInfoFlag::NickName) {
		packet->WriteString("nickName"); // nickName
	}
	if (flag & UserInfoFlag::Unk4) {
		packet->WriteString(""); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
	}
	if (flag & UserInfoFlag::Level) {
		packet->WriteUInt8(72); // level
	}
	if (flag & UserInfoFlag::Unk10) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & UserInfoFlag::Exp) {
		packet->WriteUInt64_LE(6937150764); // exp
	}
	if (flag & UserInfoFlag::Cash) {
		packet->WriteUInt64_LE(1000); // cash
	}
	if (flag & UserInfoFlag::Point) {
		packet->WriteUInt64_LE(2000); // points
	}
	if (flag & UserInfoFlag::BattleStats) {
		packet->WriteUInt32_LE(3); // battles
		packet->WriteUInt32_LE(2); // wins
		packet->WriteUInt32_LE(5); // frags
		packet->WriteUInt32_LE(3); // deaths
	}
	if (flag & UserInfoFlag::Location) {
		packet->WriteString("pcBang"); // pcBang
		packet->WriteUInt16_LE(1); // province/city (도시) index
		packet->WriteUInt16_LE(1); // district/county (구군) index
		packet->WriteUInt16_LE(1); // neighborhood (동) index
		packet->WriteString(""); // unk
	}
	if (flag & UserInfoFlag::Unk400) {
		packet->WriteUInt32_LE(0); // unk
	}
	if (flag & UserInfoFlag::Unk800) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & UserInfoFlag::Unk1000) {
		packet->WriteUInt32_LE(0); // unk
		packet->WriteUInt32_LE(0); // unk
		packet->WriteString(""); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
	}

	packet->Send();
}