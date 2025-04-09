#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "serverconsole.h"

Packet_RoomManager packet_RoomManager;

void Packet_RoomManager::ParsePacket_Room(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] Client ({}) has sent Packet_Room, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] Parsing Packet_Room from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_RoomType::RequestCreate: {
			parsePacket_Room_RequestCreate(user, packet);
			break;
		}
		case Packet_RoomType::RequestJoin: {
			parsePacket_Room_RequestJoin(user, packet);
			break;
		}
		case Packet_RoomType::RequestLeave: {
			parsePacket_Room_RequestLeave(user, packet);
			break;
		}
		case Packet_RoomType::RequestStartGame: {
			parsePacket_Room_RequestStartGame(user);
			break;
		}
		case Packet_RoomType::RequestUpdateRoomSettings: {
			parsePacket_Room_RequestUpdateRoomSettings(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent unregistered Packet_Room type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_RoomManager::SendPacket_RoomList_FullRoomList(TCPConnection::pointer connection, const vector<Room*>& rooms, unsigned short flag) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });

	packet->WriteUInt8(Packet_RoomListType::FullRoomList);
	packet->WriteUInt16_LE((unsigned short)rooms.size());

	for (auto& room : rooms) {
		buildRoomInfo(packet, room, flag);
	}

	packet->Send();
}

void Packet_RoomManager::SendPacket_RoomList_AddRoom(TCPConnection::pointer connection, Room* room, unsigned short flag) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });

	packet->WriteUInt8(Packet_RoomListType::AddRoom);
	buildRoomInfo(packet, room, flag);

	packet->Send();
}

void Packet_RoomManager::SendPacket_RoomList_RemoveRoom(TCPConnection::pointer connection, unsigned short roomID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });

	packet->WriteUInt8(Packet_RoomListType::RemoveRoom);
	packet->WriteUInt16_LE(roomID);

	packet->Send();
}

void Packet_RoomManager::SendPacket_Room_UserLeave(TCPConnection::pointer connection, unsigned long userID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::UserLeave);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}

void Packet_RoomManager::parsePacket_Room_RequestCreate(User* user, TCPConnection::Packet::pointer packet) {
	if (user->GetUserStatus() == UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestCreate, but it's already in a room!\n", user->GetUserLogName()));
		return;
	}

	unsigned short roomID = roomManager.GetFreeRoomID();

	if (roomID != 0) {
		Room* room = new Room(roomID, user, packet);
		if (room == NULL) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (!userCharacterResult.result) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);

			delete room;
			room = NULL;
			return;
		}

		room->AddRoomUser(user);
		userManager.SendRemoveUserPacketToAll(user);

		roomManager.AddRoom(room);
		roomManager.SendAddRoomPacketToAll(room, ROOMLIST_FLAG_ALL);

		sendPacket_Room_ReplyCreateAndJoin(user->GetConnection(), room, { { user, userCharacterResult.userCharacter } });
	}
}

void Packet_RoomManager::parsePacket_Room_RequestJoin(User* user, TCPConnection::Packet::pointer packet) {
	if (user->GetUserStatus() == UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestJoin, but it's already in a room!\n", user->GetUserLogName()));
		return;
	}

	unsigned short roomID = packet->ReadUInt16_LE();
	const string& password = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestJoin - roomID: {}, password: {}\n", user->GetUserLogName(), roomID, password));

	Room* room = roomManager.GetRoomByRoomID(roomID);
	if (room == NULL) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_CLOSED");
		return;
	}

	const RoomSettings& roomSettings = room->GetRoomSettings();
	if (!roomSettings.password.empty() && password != roomSettings.password) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_INVALID_PASSWD");
		return;
	}

	if (room->GetRoomUsers().size() >= roomSettings.maxPlayers) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_FULL");
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
		return;
	}

	if (!room->AddRoomUser(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestJoin, but it's already in the room!\n", user->GetUserLogName()));
		return;
	}

	userManager.SendRemoveUserPacketToAll(user);

	const vector<User*>& users = room->GetRoomUsers();
	vector<GameUser> gameUsers;
	for (auto& u : users) {
		const UserCharacterResult& uCharacterResult = u->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (uCharacterResult.result) {
			gameUsers.push_back({ u, uCharacterResult.userCharacter });
		}

		if (u != user) {
			sendPacket_Room_UserJoin(u->GetConnection(), { user, userCharacterResult.userCharacter });
		}
	}

	sendPacket_Room_ReplyCreateAndJoin(user->GetConnection(), room, gameUsers);
}

void Packet_RoomManager::parsePacket_Room_RequestLeave(User* user, TCPConnection::Packet::pointer packet) {
	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestLeave, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	unsigned char unk = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestLeave - unk: {}\n", user->GetUserLogName(), unk));

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestLeave, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	room->RemoveRoomUser(user);
	userManager.SendAddUserPacketToAll(user);

	sendPacket_Room_ReplyLeaveRoomInGame(user->GetConnection());

	userManager.SendFullUserListPacket(user->GetConnection());
	roomManager.SendFullRoomListPacket(user->GetConnection());
}

void Packet_RoomManager::parsePacket_Room_RequestStartGame(User* user) {
	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestStartGame, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	packet_HostManager.SendPacket_Host_StartGame(user);
}

void Packet_RoomManager::parsePacket_Room_RequestUpdateRoomSettings(User* user, TCPConnection::Packet::pointer packet) {
	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestUpdateRoomSettings, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestUpdateRoomSettings, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestUpdateRoomSettings, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	RoomSettings newSettings = room->GetRoomSettings();
	unsigned long lowFlag = packet->ReadUInt32_LE();
	unsigned char highFlag = packet->ReadUInt8();

	if (lowFlag & ROOMSETTINGS_LFLAG_ROOMNAME) {
		newSettings.roomName = packet->ReadString();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK2) {
		newSettings.unk2 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK4) {
		newSettings.unk4_1 = packet->ReadUInt8();
		newSettings.unk4_2 = packet->ReadUInt8();
		newSettings.unk4_3 = packet->ReadUInt8();
		newSettings.unk4_4 = packet->ReadUInt32_LE();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_PASSWORD) {
		newSettings.password = packet->ReadString();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK10) {
		newSettings.unk10 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK20) {
		newSettings.unk20 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK40) {
		newSettings.unk40 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_MAPID) {
		newSettings.mapID = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_MAXPLAYERS) {
		newSettings.maxPlayers = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_WINLIMIT) {
		newSettings.winLimit = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK400) {
		newSettings.unk400 = packet->ReadUInt16_LE();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_TIMELIMIT) {
		newSettings.timeLimit = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_ROUNDTIME) {
		newSettings.roundTime = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK2000) {
		newSettings.unk2000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK4000) {
		newSettings.unk4000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK8000) {
		newSettings.unk8000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK10000) {
		newSettings.unk10000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK20000) {
		newSettings.unk20000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK40000) {
		newSettings.unk40000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK80000) {
		newSettings.unk80000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK100000) {
		newSettings.unk100000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK200000) {
		newSettings.unk200000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK400000) {
		newSettings.unk400000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK800000) {
		newSettings.unk800000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK1000000) {
		newSettings.unk1000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK2000000) {
		newSettings.unk2000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK4000000) {
		newSettings.unk4000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK8000000) {
		newSettings.unk8000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK10000000) {
		newSettings.unk10000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK20000000) {
		newSettings.unk20000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK40000000) {
		newSettings.unk40000000 = packet->ReadUInt8();
	}
	if (lowFlag & ROOMSETTINGS_LFLAG_UNK80000000) {
		newSettings.unk80000000_1 = packet->ReadUInt8();
		for (unsigned char i = 0; i < 2; i++) {
			newSettings.unk80000000_2[i].unk8000000_vec_1 = packet->ReadUInt32_LE();
			newSettings.unk80000000_2[i].unk8000000_vec_2 = packet->ReadUInt32_LE();
			newSettings.unk80000000_2[i].unk8000000_vec_3 = packet->ReadUInt8();
			newSettings.unk80000000_2[i].unk8000000_vec_4 = packet->ReadUInt8();
			newSettings.unk80000000_2[i].unk8000000_vec_5 = packet->ReadUInt8();
			newSettings.unk80000000_2[i].unk8000000_vec_6 = packet->ReadUInt8();
			newSettings.unk80000000_2[i].unk8000000_vec_7 = packet->ReadUInt16_LE();
			newSettings.unk80000000_2[i].unk8000000_vec_8 = packet->ReadUInt8();
			newSettings.unk80000000_2[i].unk8000000_vec_9 = packet->ReadUInt8();
		}
	}
	if (highFlag & ROOMSETTINGS_HFLAG_UNK1) {
		newSettings.unkh1_1 = packet->ReadUInt32_LE();
		newSettings.unkh1_2 = packet->ReadString();
		newSettings.unkh1_3 = packet->ReadUInt8();
		newSettings.unkh1_4 = packet->ReadUInt8();
		newSettings.unkh1_5 = packet->ReadUInt8();
	}
	if (highFlag & ROOMSETTINGS_HFLAG_UNK2) {
		newSettings.unkh2 = packet->ReadUInt8();
	}

	room->UpdateRoomSettings(newSettings);

	const vector<User*>& users = room->GetRoomUsers();
	for (auto& u : users) {
		sendPacket_Room_ReplyUpdateRoomSettings(u->GetConnection(), newSettings);
	}
}

void Packet_RoomManager::buildRoomInfo(TCPConnection::Packet::pointer packet, Room* room, unsigned short flag) {
	packet->WriteUInt16_LE(room->GetRoomID());
	packet->WriteUInt16_LE(flag);

	const RoomSettings& roomSettings = room->GetRoomSettings();

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

void Packet_RoomManager::buildRoomUserInfo(TCPConnection::Packet::pointer packet, const GameUser& gameUser) {
	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packet->WriteUInt8(0); // unk
	packet->WriteUInt8(0); // unk

	const UserNetwork& userNetwork = gameUser.user->GetUserNetwork();

	packet->WriteUInt32_LE(userNetwork.externalIP);
	packet->WriteUInt16_LE(userNetwork.externalPortType0);
	packet->WriteUInt16_LE(userNetwork.externalPortType1);
	packet->WriteUInt32_LE(userNetwork.localIP);
	packet->WriteUInt16_LE(userNetwork.localPortType0);
	packet->WriteUInt16_LE(userNetwork.localPortType1);

	packetManager.BuildUserCharacter(packet, gameUser.userCharacter);
}

void Packet_RoomManager::sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, Room* room, const vector<GameUser>& gameUsers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyCreateAndJoin);
	packet->WriteUInt32_LE(room->GetRoomHostUser()->GetUserID());
	packet->WriteUInt16_LE(room->GetRoomID());
	packet->WriteUInt8(0); // unk

	buildRoomSettings(packet, room->GetRoomSettings());

	packet->WriteUInt8((unsigned char)gameUsers.size());

	for (auto& gameUser : gameUsers) {
		buildRoomUserInfo(packet, gameUser);
	}

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_UserJoin(TCPConnection::pointer connection, const GameUser& gameUser) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::UserJoin);
	buildRoomUserInfo(packet, gameUser);

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_ReplyUpdateRoomSettings(TCPConnection::pointer connection, const RoomSettings& roomSettings) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyUpdateRoomSettings);
	buildRoomSettings(packet, roomSettings);

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_ReplyLeaveRoomInGame(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });

	packet->WriteUInt8(Packet_RoomType::ReplyLeaveRoomInGame);

	packet->Send();
}