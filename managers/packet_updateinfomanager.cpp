#include "packet_updateinfomanager.h"
#include <iostream>

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UpdateInfoManager] Parsing Packet_UpdateInfo from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			string unk1 = packet->ReadString();

			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", packet->GetConnection()->GetEndPoint(), type, unk1.c_str());
			break;
		}
		case 1: {
			// nothing to read
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
		default: {
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent unregistered Packet_UpdateInfo type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}

void Packet_UpdateInfoManager::SendPacket_UpdateInfo(TCPConnection::pointer connection, GameUser gameUser) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::UpdateInfo });

	packet->WriteUInt32_LE(gameUser.userID); // userID
	packet->WriteUInt16_LE(gameUser.flag); // flag

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

	packet->Send();
}