#include "packet_roommanager.h"
#include "packet_hostmanager.h"
#include <iostream>

Packet_RoomManager packet_RoomManager;

void Packet_RoomManager::ParsePacket_Room(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_RoomManager] Parsing Packet_Room from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_RoomType::RequestCreate: {
			parsePacket_Room_RequestCreate(packet);
			break;
		}
		case Packet_RoomType::RequestStartGame: {
			parsePacket_Room_RequestStartGame(packet);
			break;
		}
		default: {
			cout << format("[Packet_RoomManager] Client ({}) has sent unregistered Packet_Room type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}

void Packet_RoomManager::parsePacket_Room_RequestCreate(TCPConnection::Packet::pointer packet) {
	RoomSettings roomSettings = RoomSettings(packet);

	cout << format("[Packet_RoomManager] Client ({}) has sent Packet_Room RequestCreate - roomName: {}, unk2: {}, maxPlayers: {}, gameModeID: {}, mapID: {}, winLimit: {}, killLimit: {}, timeLimit: {}, roundTime: {}, password: {}, unk11: {}, unk12: {}, quickStart: {}, unk14: {}, unk15: {}, unk16: {}\n", packet->GetConnection()->GetEndPoint(), roomSettings.roomName.c_str(), roomSettings.unk2, roomSettings.maxPlayers, roomSettings.gameModeID, roomSettings.mapID, roomSettings.winLimit, roomSettings.killLimit, roomSettings.timeLimit, roomSettings.roundTime, roomSettings.password.c_str(), roomSettings.unk11, roomSettings.unk12, roomSettings.quickStart, roomSettings.unk14, roomSettings.unk15, roomSettings.unk16);

	sendPacket_Room_ReplyCreateAndJoin(packet->GetConnection(), roomSettings);
}

void Packet_RoomManager::parsePacket_Room_RequestStartGame(TCPConnection::Packet::pointer packet) {
	packet_HostManager.SendPacket_Host_StartGame(packet->GetConnection(), 1);
}

void Packet_RoomManager::sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, RoomSettings roomSettings) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyCreateAndJoin);
	packet->WriteUInt32_LE(1); // roomHost UserID
	packet->WriteUInt16_LE(1); // roomID
	packet->WriteUInt8(0); // unk
	packet->WriteUInt32_LE(roomSettings.lowFlag);
	packet->WriteUInt8(roomSettings.highFlag);

	if (roomSettings.lowFlag & 0x1) {
		packet->WriteString(roomSettings.roomName);
	}
	if (roomSettings.lowFlag & 0x2) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x4) {
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt32_LE(0); // unk
	}
	if (roomSettings.lowFlag & 0x8) {
		packet->WriteString(roomSettings.password);
	}
	if (roomSettings.lowFlag & 0x10) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x20) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x40) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x80) {
		packet->WriteUInt8(roomSettings.mapID);
	}
	if (roomSettings.lowFlag & 0x100) {
		packet->WriteUInt8(roomSettings.maxPlayers);
	}
	if (roomSettings.lowFlag & 0x200) {
		packet->WriteUInt8(roomSettings.winLimit);
	}
	if (roomSettings.lowFlag & 0x400) {
		packet->WriteUInt16_LE(0); // unk
	}
	if (roomSettings.lowFlag & 0x800) {
		packet->WriteUInt8(roomSettings.timeLimit);
	}
	if (roomSettings.lowFlag & 0x1000) {
		packet->WriteUInt8(roomSettings.roundTime);
	}
	if (roomSettings.lowFlag & 0x2000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x4000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x8000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x10000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x20000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x40000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x80000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x100000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x200000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x400000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x800000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x1000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x2000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x4000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x8000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x10000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x20000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x40000000) {
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.lowFlag & 0x80000000) {
		packet->WriteUInt8(0); // unk
		for (unsigned char i = 0; i < 2; i++) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt8(0); // unk
			packet->WriteUInt8(0); // unk
		}
	}
	if (roomSettings.highFlag & 0x1) {
		packet->WriteUInt32_LE(0); // unk
		packet->WriteString(""); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
	}
	if (roomSettings.highFlag & 0x2) {
		packet->WriteUInt8(0); // unk
	}

	packet->WriteUInt8(1); // Num. of Users

	for (unsigned char i = 0; i < 1; i++) {
		packet->WriteUInt32_LE(1); // userID
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk
		packet->WriteUInt32_LE(0); // some ip
		packet->WriteUInt16_LE(0); // some port
		packet->WriteUInt16_LE(0); // some port
		packet->WriteUInt32_LE(0); // some ip
		packet->WriteUInt16_LE(0); // some port
		packet->WriteUInt16_LE(0); // some port
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
		if (UserInfoFlag::All & UserInfoFlag::Points) {
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