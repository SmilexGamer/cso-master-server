#include "packet_roommanager.h"
#include "packet_hostmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include <iostream>

Packet_RoomManager packet_RoomManager;

void Packet_RoomManager::ParsePacket_Room(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_RoomManager] Client ({}) has sent Packet_Room, but it's not logged in!\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_RoomManager] Parsing Packet_Room from client ({})\n", user->GetUserIPAddress());

	unsigned char type = packet->ReadUInt8();

	if (user->GetUserStatus() == UserStatus::InLobby && type != Packet_RoomType::RequestCreate && type != Packet_RoomType::RequestJoin) {
		cout << format("[Packet_RoomManager] Client ({}) has sent Packet_Room type {}, but it's not in a room!\n", packet->GetConnection()->GetIPAddress(), type);
		return;
	}

	if (user->GetUserStatus() != UserStatus::InLobby && (type == Packet_RoomType::RequestCreate || type == Packet_RoomType::RequestJoin)) {
		cout << format("[Packet_RoomManager] Client ({}) has sent Packet_Room type {}, but it's already in a room!\n", packet->GetConnection()->GetIPAddress(), type);
		return;
	}

	switch (type) {
		case Packet_RoomType::RequestCreate: {
			parsePacket_Room_RequestCreate(user, packet);
			break;
		}
		case Packet_RoomType::RequestLeave: {
			parsePacket_Room_RequestLeave(user);
			break;
		}
		case Packet_RoomType::RequestStartGame: {
			parsePacket_Room_RequestStartGame(user);
			break;
		}
		default: {
			cout << format("[Packet_RoomManager] Client ({}) has sent unregistered Packet_Room type {}!\n", user->GetUserIPAddress(), type);
			break;
		}
	}
}

void Packet_RoomManager::SendPacket_RoomList_FullRoomList(TCPConnection::pointer connection, const vector<Room*>& rooms, unsigned short flag) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });

	packet->WriteUInt8(Packet_RoomListType::FullRoomList);
	packet->WriteUInt16_LE((unsigned short)rooms.size());

	RoomSettings roomSettings;

	for (auto& room : rooms) {
		packet->WriteUInt16_LE(room->GetRoomID());
		packet->WriteUInt16_LE(flag);

		roomSettings = room->GetRoomSettings();

		if (flag & ROOMLIST_FLAG_ROOMNAME) {
			packet->WriteString(roomSettings.roomName);
		}
		if (flag & ROOMLIST_FLAG_UNK2) {
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_ISPASSWORDED) {
			packet->WriteBool(!roomSettings.password.empty());
		}
		if (flag & ROOMLIST_FLAG_LEVELLIMIT) {
			packet->WriteUInt8(0); // levelLimit
		}
		if (flag & ROOMLIST_FLAG_GAMEMODEID) {
			packet->WriteUInt8(0); // gamemodeID
		}
		if (flag & ROOMLIST_FLAG_MAPID) {
			packet->WriteUInt8(roomSettings.mapID); // mapID
		}
		if (flag & ROOMLIST_FLAG_CURRENTPLAYERS) {
			packet->WriteUInt8((unsigned char)room->GetRoomUsers().size());
		}
		if (flag & ROOMLIST_FLAG_MAXPLAYERS) {
			packet->WriteUInt8(roomSettings.maxPlayers);
		}
		if (flag & ROOMLIST_FLAG_WEAPONLIMIT) {
			packet->WriteUInt8(0); // weaponLimit
		}
		if (flag & ROOMLIST_FLAG_UNK200) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_UNK400) {
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_UNK800) {
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt32_LE(0); // unk
			packet->WriteUInt16_LE(0); // unk
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_UNK1000) {
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_UNK2000) {
			packet->WriteUInt8(0); // unk
		}
		if (flag & ROOMLIST_FLAG_UNK4000) {
			packet->WriteUInt8(0); // unk
		}
	}

	packet->Send();
}

void Packet_RoomManager::parsePacket_Room_RequestCreate(User* user, TCPConnection::Packet::pointer packet) {
	unsigned short roomID = roomManager.GetFreeRoomID();

	if (roomID != 0) {
		Room* room = new Room(roomID, user, packet);
		if (room == NULL) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		//cout << format("[Packet_RoomManager] Client ({}) has sent Packet_Room RequestCreate - roomName: {}, unk2: {}, maxPlayers: {}, gameModeID: {}, mapID: {}, winLimit: {}, killLimit: {}, timeLimit: {}, roundTime: {}, password: {}, unk11: {}, unk12: {}, quickStart: {}, unk14: {}, unk15: {}, unk16: {}\n", user->GetUserIPAddress(), roomSettings.roomName, roomSettings.unk2, roomSettings.maxPlayers, roomSettings.gameModeID, roomSettings.mapID, roomSettings.winLimit, roomSettings.killLimit, roomSettings.timeLimit, roomSettings.roundTime, roomSettings.password, roomSettings.unk11, roomSettings.unk12, roomSettings.quickStart, roomSettings.unk14, roomSettings.unk15, roomSettings.unk16);

		UserCharacterResult result = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (!result.result) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		roomManager.AddRoom(room);
		user->SetUserStatus(UserStatus::InRoom);

		sendPacket_Room_ReplyCreateAndJoin(user->GetConnection(), room, { { user, result.userCharacter } });
	}
}

void Packet_RoomManager::parsePacket_Room_RequestStartGame(User* user) {
	packet_HostManager.SendPacket_Host_StartGame(user);
}

void Packet_RoomManager::parsePacket_Room_RequestLeave(User* user) {
	sendPacket_Room_ReplyLeaveRoomInGame(user->GetConnection());
	sendPacket_Room_ReplyLeaveRoom(user);
}

void Packet_RoomManager::buildRoomSettings(TCPConnection::Packet::pointer packet, const RoomSettings& roomSettings) {
	packet->WriteUInt32_LE(roomSettings.lowFlag);
	packet->WriteUInt8(roomSettings.highFlag);

	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROOMNAME) {
		packet->WriteString(roomSettings.roomName);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK2) {
		packet->WriteUInt8(roomSettings.unk2); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK4) {
		packet->WriteUInt8(roomSettings.unk4_1); // unk
		packet->WriteUInt8(roomSettings.unk4_2); // unk
		packet->WriteUInt8(roomSettings.unk4_3); // unk
		packet->WriteUInt32_LE(roomSettings.unk4_4); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_PASSWORD) {
		packet->WriteString(roomSettings.password);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK10) {
		packet->WriteUInt8(roomSettings.unk10); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK20) {
		packet->WriteUInt8(roomSettings.unk20); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK40) {
		packet->WriteUInt8(roomSettings.unk40); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_MAPID) {
		packet->WriteUInt8(roomSettings.mapID);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_MAXPLAYERS) {
		packet->WriteUInt8(roomSettings.maxPlayers);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_WINLIMIT) {
		packet->WriteUInt8(roomSettings.winLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK400) {
		packet->WriteUInt16_LE(roomSettings.unk400); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_TIMELIMIT) {
		packet->WriteUInt8(roomSettings.timeLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROUNDTIME) {
		packet->WriteUInt8(roomSettings.roundTime);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK2000) {
		packet->WriteUInt8(roomSettings.unk2000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK4000) {
		packet->WriteUInt8(roomSettings.unk4000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK8000) {
		packet->WriteUInt8(roomSettings.unk8000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK10000) {
		packet->WriteUInt8(roomSettings.unk10000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK20000) {
		packet->WriteUInt8(roomSettings.unk20000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK40000) {
		packet->WriteUInt8(roomSettings.unk40000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK80000) {
		packet->WriteUInt8(roomSettings.unk80000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK100000) {
		packet->WriteUInt8(roomSettings.unk100000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK200000) {
		packet->WriteUInt8(roomSettings.unk200000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK400000) {
		packet->WriteUInt8(roomSettings.unk400000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK800000) {
		packet->WriteUInt8(roomSettings.unk800000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK1000000) {
		packet->WriteUInt8(roomSettings.unk1000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK2000000) {
		packet->WriteUInt8(roomSettings.unk2000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK4000000) {
		packet->WriteUInt8(roomSettings.unk4000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK8000000) {
		packet->WriteUInt8(roomSettings.unk8000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK10000000) {
		packet->WriteUInt8(roomSettings.unk10000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK20000000) {
		packet->WriteUInt8(roomSettings.unk20000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK40000000) {
		packet->WriteUInt8(roomSettings.unk40000000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK80000000) {
		packet->WriteUInt8(roomSettings.unk80000000_1); // unk
		for (unsigned char i = 0; i < 2; i++) {
			packet->WriteUInt32_LE(roomSettings.unk80000000_2[i].unk8000000_vec_1); // unk
			packet->WriteUInt32_LE(roomSettings.unk80000000_2[i].unk8000000_vec_2); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_3); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_4); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_5); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_6); // unk
			packet->WriteUInt16_LE(roomSettings.unk80000000_2[i].unk8000000_vec_7); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_8); // unk
			packet->WriteUInt8(roomSettings.unk80000000_2[i].unk8000000_vec_9); // unk
		}
	}
	if (roomSettings.highFlag & ROOMSETTINGS_HFLAG_UNK1) {
		packet->WriteUInt32_LE(roomSettings.unkh1_1); // unk
		packet->WriteString(roomSettings.unkh1_2); // unk
		packet->WriteUInt8(roomSettings.unkh1_3); // unk
		packet->WriteUInt8(roomSettings.unkh1_4); // unk
		packet->WriteUInt8(roomSettings.unkh1_5); // unk
	}
	if (roomSettings.highFlag & ROOMSETTINGS_HFLAG_UNK2) {
		packet->WriteUInt8(roomSettings.unkh2); // unk
	}
}

void Packet_RoomManager::sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, Room* room, const vector<UserFull>& roomUsers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyCreateAndJoin);
	packet->WriteUInt32_LE(room->GetRoomHostUser()->GetUserID());
	packet->WriteUInt16_LE(room->GetRoomID());
	packet->WriteUInt8(0); // unk

	buildRoomSettings(packet, room->GetRoomSettings());

	packet->WriteUInt8((unsigned char)roomUsers.size());

	UserNetwork userNetwork;

	for (auto& roomUser : roomUsers) {
		packet->WriteUInt32_LE(roomUser.user->GetUserID());
		packet->WriteUInt8(0); // unk
		packet->WriteUInt8(0); // unk

		userNetwork = roomUser.user->GetUserNetwork();

		packet->WriteUInt32_LE(userNetwork.externalIP);
		packet->WriteUInt16_LE(userNetwork.externalPortType0);
		packet->WriteUInt16_LE(userNetwork.externalPortType1);
		packet->WriteUInt32_LE(userNetwork.localIP);
		packet->WriteUInt16_LE(userNetwork.localPortType0);
		packet->WriteUInt16_LE(userNetwork.localPortType1);

		packetManager.BuildUserCharacter(packet, roomUser.userCharacter);
	}

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_ReplyLeaveRoom(User* user) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, user->GetConnection(), { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyLeaveRoom);
	packet->WriteUInt32_LE(user->GetUserID());
	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_ReplyLeaveRoomInGame(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyLeaveRoomInGame);
	packet->Send();
}