#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "serverconsole.h"

Packet_RoomManager packet_RoomManager;

void Packet_RoomManager::ParsePacket_Room(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] Client ({}) has sent Packet_Room, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] Parsing Packet_Room from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_RoomType::CreateRoom: {
			parsePacket_Room_CreateRoom(user, packet);
			break;
		}
		case Packet_RoomType::JoinRoom: {
			parsePacket_Room_JoinRoom(user, packet);
			break;
		}
		case Packet_RoomType::LeaveRoom: {
			parsePacket_Room_LeaveRoom(user, packet);
			break;
		}
		case Packet_RoomType::StartGame: {
			parsePacket_Room_StartGame(user);
			break;
		}
		case Packet_RoomType::UpdateRoomSettings: {
			parsePacket_Room_UpdateRoomSettings(user, packet);
			break;
		}
		case Packet_RoomType::ResultConfirm: {
			parsePacket_Room_ResultConfirm(user, packet);
			break;
		}
		case Packet_RoomType::ReturnToRoom: {
			parsePacket_Room_ReturnToRoom(user);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent unregistered Packet_Room type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_RoomManager::SendPacket_RoomList_FullRoomList(TCPConnection::pointer connection, vector<Room*> rooms, unsigned short flag) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });
	if (packet == NULL) {
		return;
	}

	rooms.erase(remove_if(rooms.begin(), rooms.end(), [](Room* room) {
		return (room == NULL);
	}), rooms.end());

	packet->WriteUInt8(Packet_RoomListType::FullRoomList);
	packet->WriteUInt16_LE((unsigned short)rooms.size());

	for (auto& room : rooms) {
		buildRoomInfo(packet, room, flag);
	}

	packet->Send();
}

void Packet_RoomManager::SendPacket_RoomList_AddRoom(TCPConnection::pointer connection, Room* room, unsigned short flag) {
	if (connection == NULL || room == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomListType::AddRoom);
	buildRoomInfo(packet, room, flag);

	packet->Send();
}

void Packet_RoomManager::SendPacket_RoomList_RemoveRoom(TCPConnection::pointer connection, unsigned short roomID) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomListType::RemoveRoom);
	packet->WriteUInt16_LE(roomID);

	packet->Send();
}

void Packet_RoomManager::SendPacket_RoomList_UpdateRoom(TCPConnection::pointer connection, Room* room, unsigned short flag) {
	if (connection == NULL || room == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::RoomList });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomListType::UpdateRoom);
	buildRoomInfo(packet, room, flag);

	packet->Send();
}

void Packet_RoomManager::SendPacket_Room_UserLeave(TCPConnection::pointer connection, unsigned long userID) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::UserLeave);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}

void Packet_RoomManager::SendPacket_Room_ReplyUpdateRoomSettings(TCPConnection::pointer connection, const RoomSettings& roomSettings) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::ReplyUpdateRoomSettings);
	buildRoomSettings(packet, roomSettings);

	packet->Send();
}

void Packet_RoomManager::SendPacket_Room_GameResult(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::GameResult);
	packet->WriteUInt8(0); // 0 - normal match, 1 - clan battle

	if (true) { // normal match
		packet->WriteUInt8(0); // win team; 1 - tr, 2 - ct, any other value - no team
		packet->WriteUInt8(0); // num of players

		for (unsigned char unk3 = 0; unk3 < 0; unk3++) {
			packet->WriteUInt32_LE(0); // userID
			packet->WriteUInt8(0); // team; 1 - tr, 2 - ct
			packet->WriteUInt16_LE(0); // kills
			packet->WriteUInt16_LE(0); // deaths
			packet->WriteUInt8(0); // wins
			packet->WriteUInt8(0); // losses
			packet->WriteUInt16_LE(0); // exp
			packet->WriteUInt16_LE(0); // bonus exp?
			packet->WriteUInt16_LE(0); // point
			packet->WriteUInt16_LE(0); // bonus point?
			packet->WriteUInt8(0); // rank; 1 - 1st, 2 - 2nd, 3 - 3rd, 4 - 4th, 5 and beyond - visual bugs
			packet->WriteBool(false); // level up
		}
	}
	else { // clan battle
		packet->WriteUInt8(0); // unk2
		packet->WriteUInt8(0); // unk3
		packet->WriteUInt8(0); // unk4
		packet->WriteUInt8(0); // unk5
		packet->WriteUInt8(0); // unk6

		for (unsigned char i = 0; i < 2; i++) {
			packet->WriteUInt32_LE(0); // unk7
			packet->WriteUInt32_LE(0); // unk8
			packet->WriteString(""); // unk9
			packet->WriteString(""); // unk10
			packet->WriteUInt8(0); // unk11
			packet->WriteUInt8(0); // unk12
			packet->WriteUInt8(0); // unk13
			packet->WriteString(""); // unk14
			packet->WriteUInt16_LE(0); // unk15
			packet->WriteUInt16_LE(0); // unk16
			packet->WriteUInt16_LE(0); // unk17
		}

		packet->WriteUInt8(0); // unk18

		for (unsigned char unk18 = 0; unk18 < 0; unk18++) {
			packet->WriteUInt32_LE(0); // unk19
			packet->WriteString(""); // unk20
			packet->WriteUInt8(0); // unk21
			packet->WriteUInt8(0); // unk22
			packet->WriteUInt8(0); // unk23
			packet->WriteUInt16_LE(0); // unk24
			packet->WriteUInt16_LE(0); // unk25
		}

		for (unsigned char unk5 = 0; unk5 < 0; unk5++) {
			packet->WriteUInt8(0); // unk26
			packet->WriteUInt8(0); // unk27

			for (unsigned char i = 0; i < 2; i++) {
				packet->WriteUInt8(0); // unk28
				packet->WriteUInt8(0); // unk29
				packet->WriteUInt8(0); // unk30

				for (unsigned char unk30 = 0; unk30 < 0; unk30++) {
					packet->WriteUInt32_LE(0); // unk31
					packet->WriteUInt8(0); // unk32
					packet->WriteUInt16_LE(0); // unk33
					packet->WriteUInt16_LE(0); // unk34
					packet->WriteUInt8(0); // unk35
					packet->WriteUInt8(0); // unk36
					packet->WriteUInt16_LE(0); // unk37
					packet->WriteUInt16_LE(0); // unk38
					packet->WriteUInt16_LE(0); // unk39
					packet->WriteUInt16_LE(0); // unk40
					packet->WriteUInt8(0); // unk41
					packet->WriteUInt8(0); // unk42
					packet->WriteUInt8(0); // unk43
					packet->WriteUInt8(0); // unk44
				}
			}
		}

		if (false) { // if (unk2)
			for (unsigned char i = 0; i < 0; i++) {
				packet->WriteUInt16_LE(0); // unk45
				packet->WriteUInt8(0); // unk46
				packet->WriteUInt8(0); // unk47
				packet->WriteUInt8(0); // unk48
			}
		}
	}

	packet->Send();
}

void Packet_RoomManager::parsePacket_Room_CreateRoom(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() == UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room CreateRoom, but it's already in a room!\n", user->GetUserLogName()));
		return;
	}

	unsigned short roomID = roomManager.GetFreeRoomID();
	if (!roomID) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room CreateRoom, but there's no available roomID!\n", user->GetUserLogName()));
		return;
	}

	Room* newRoom = new Room(roomID, user);
	if (newRoom == NULL) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);

		delete newRoom;
		newRoom = NULL;
		return;
	}

	RoomSettings roomSettings = newRoom->GetRoomSettings();

	roomSettings.roomName = packet->ReadString();
	unsigned char unk2 = packet->ReadUInt8();
	roomSettings.maxPlayers = packet->ReadUInt8();
	roomSettings.gameModeID = packet->ReadUInt8();
	roomSettings.mapID = packet->ReadUInt8();
	roomSettings.winLimit = packet->ReadUInt8();
	roomSettings.killLimit = packet->ReadUInt16_LE();
	roomSettings.timeLimit = packet->ReadUInt8();
	roomSettings.roundTime = packet->ReadUInt8();
	roomSettings.password = packet->ReadString();
	unsigned char unk11 = packet->ReadUInt8();
	unsigned char unk12 = packet->ReadUInt8();
	bool fastStart = packet->ReadBool();
	unsigned char unk14 = packet->ReadUInt8();
	unsigned char unk15 = packet->ReadUInt8();
	unsigned char unk16 = packet->ReadUInt32_LE();

	roomSettings.hostagePenalty = 3;

	switch (roomSettings.gameModeID) {
		case GameMode::Original:
		case GameMode::Bot:
		case GameMode::Official:
		case GameMode::Official_TieBreak: {
			roomSettings.freezeTime = 6;
			break;
		}
	}

	roomSettings.buyTime = 90;
	roomSettings.nickNameDisplay = 1;

	if (roomSettings.gameModeID == GameMode::DeathMatch || roomSettings.gameModeID == GameMode::BotDM) {
		roomSettings.friendlyFire = true;
	}

	roomSettings.flashLight = true;
	roomSettings.footSteps = true;
	roomSettings.voiceChat = true;

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room CreateRoom - roomName: {}, unk2: {}, maxPlayers: {}, gameModeID: {}, mapID: {}, winLimit: {}, killLimit: {}, timeLimit: {}, roundTime: {}, password: {}, unk11: {}, unk12: {}, fastStart: {}, unk14: {}, unk15: {}, unk16: {}\n", user->GetUserLogName(), roomSettings.roomName, unk2, roomSettings.maxPlayers, roomSettings.gameModeID, roomSettings.mapID, roomSettings.winLimit, roomSettings.killLimit, roomSettings.timeLimit, roomSettings.roundTime, roomSettings.password, unk11, unk12, fastStart, unk14, unk15, unk16));

	newRoom->SetRoomSettings(roomSettings);

	newRoom->AddRoomUser(user);
	userManager.SendRemoveUserPacketToAll(user);

	roomManager.AddRoom(newRoom);
	roomManager.SendAddRoomPacketToAll(newRoom, ROOMLIST_FLAG_ALL);

	sendPacket_Room_ReplyCreateAndJoin(connection, newRoom, { { user, userCharacterResult.userCharacter } });

	if (fastStart) {
		packet_HostManager.SendPacket_Host_HostStart(user);
	}
}

void Packet_RoomManager::parsePacket_Room_JoinRoom(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() == UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room JoinRoom, but it's already in a room!\n", user->GetUserLogName()));
		return;
	}

	unsigned short roomID = packet->ReadUInt16_LE();
	const string& password = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room JoinRoom - roomID: {}, password: {}\n", user->GetUserLogName(), roomID, password));

	Room* room = roomManager.GetRoomByRoomID(roomID);
	if (room == NULL) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_CLOSED");
		return;
	}

	const RoomSettings& roomSettings = room->GetRoomSettings();
	if (!roomSettings.password.empty() && password != roomSettings.password) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_INVALID_PASSWD");
		return;
	}

	if (room->GetRoomUsers().size() >= roomSettings.maxPlayers) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "ROOM_JOIN_FAILED_FULL");
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	if (!room->AddRoomUser(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room JoinRoom, but it's already in the room!\n", user->GetUserLogName()));
		return;
	}

	userManager.SendRemoveUserPacketToAll(user);
	roomManager.SendUpdateRoomPacketToAll(room, ROOMLIST_FLAG_CURRENTPLAYERS);

	vector<User*> roomUsers = room->GetRoomUsers();

	roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [user](User* u) {
		return (u == NULL || u == user || u->GetConnection() == NULL);
	}), roomUsers.end());

	vector<GameUser> gameUsers;
	UserCharacterResult uCharacterResult;
	unsigned long userID = user->GetUserID();

	for (auto& u : roomUsers) {
		uCharacterResult = u->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (uCharacterResult.result) {
			gameUsers.push_back({ u, uCharacterResult.userCharacter });
		}

		sendPacket_Room_UserJoin(u->GetConnection(), { user, userCharacterResult.userCharacter });
		sendPacket_Room_UserReady(u->GetConnection(), userID, 0);
	}

	gameUsers.push_back({ user, userCharacterResult.userCharacter });
	sendPacket_Room_ReplyCreateAndJoin(connection, room, gameUsers);
	sendPacket_Room_UserReady(connection, userID, 0);
}

void Packet_RoomManager::parsePacket_Room_LeaveRoom(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom && user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room LeaveRoom, but it's not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room LeaveRoom, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	unsigned char unk = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room LeaveRoom - unk: {}\n", user->GetUserLogName(), unk));

	room->RemoveRoomUser(user);
	userManager.SendAddUserPacketToAll(user);

	sendPacket_Room_LeaveRoomInGame(connection);

	userManager.SendFullUserListPacket(connection);
	roomManager.SendFullRoomListPacket(connection);
}

void Packet_RoomManager::parsePacket_Room_StartGame(User* user) {
	if (user == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room StartGame, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room StartGame, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	User* roomHostUser = room->GetRoomHostUser();
	if (roomHostUser == NULL) {
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room StartGame\n", user->GetUserLogName()));

	if (roomHostUser == user) {
		packet_HostManager.SendPacket_Host_HostStart(user);
	}
	else {
		packet_HostManager.SendPacket_Host_HostJoin(connection, roomHostUser->GetUserID());
	}
}

void Packet_RoomManager::parsePacket_Room_UpdateRoomSettings(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room UpdateRoomSettings, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room UpdateRoomSettings, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room UpdateRoomSettings, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room UpdateRoomSettings\n", user->GetUserLogName()));

	RoomSettings roomSettings;
	roomSettings.lowFlag = packet->ReadUInt32_LE();
	roomSettings.highFlag = packet->ReadUInt8();

	unsigned char roomListFlag = 0;

	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROOMNAME) {
		roomSettings.roomName = packet->ReadString();
		roomListFlag |= ROOMLIST_FLAG_ROOMNAME;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK2) {
		roomSettings.unk2 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK4) {
		roomSettings.unk4_1 = packet->ReadUInt8();
		roomSettings.unk4_2 = packet->ReadUInt8();
		roomSettings.unk4_3 = packet->ReadUInt8();
		roomSettings.unk4_4 = packet->ReadUInt32_LE();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_PASSWORD) {
		roomSettings.password = packet->ReadString();
		roomListFlag |= ROOMLIST_FLAG_ISPASSWORDED;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_LEVELLIMIT) {
		roomSettings.levelLimit = packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_LEVELLIMIT;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK20) {
		roomSettings.unk20 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_GAMEMODEID) {
		roomSettings.gameModeID = packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_GAMEMODEID;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_MAPID) {
		roomSettings.mapID = packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_MAPID;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_MAXPLAYERS) {
		roomSettings.maxPlayers = packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_MAXPLAYERS;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_WINLIMIT) {
		roomSettings.winLimit = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_KILLLIMIT) {
		roomSettings.killLimit = packet->ReadUInt16_LE();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_TIMELIMIT) {
		roomSettings.timeLimit = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROUNDTIME) {
		roomSettings.roundTime = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_WEAPONLIMIT) {
		roomSettings.weaponLimit = packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_WEAPONLIMIT;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_HOSTAGEPENALTY) {
		roomSettings.hostagePenalty = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FREEZETIME) {
		roomSettings.freezeTime = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_BUYTIME) {
		roomSettings.buyTime = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_NICKNAMEDISPLAY) {
		roomSettings.nickNameDisplay = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK40000) {
		roomSettings.unk40000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK80000) {
		roomSettings.unk80000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FRIENDLYFIRE) {
		roomSettings.friendlyFire = packet->ReadBool();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FLASHLIGHT) {
		roomSettings.flashLight = packet->ReadBool();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FOOTSTEPS) {
		roomSettings.footSteps = packet->ReadBool();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK800000) {
		roomSettings.unk800000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK1000000) {
		roomSettings.unk1000000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK2000000) {
		roomSettings.unk2000000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK4000000) {
		roomSettings.unk4000000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK8000000) {
		roomSettings.unk8000000 = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_DEATHCAMERATYPE) {
		roomSettings.deathCameraType = packet->ReadUInt8();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_VOICECHAT) {
		roomSettings.voiceChat = packet->ReadBool();
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROOMSTATE) {
		roomSettings.roomState = (RoomState)packet->ReadUInt8();
		roomListFlag |= ROOMLIST_FLAG_ROOMSTATE;
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK80000000) {
		roomSettings.unk80000000_1 = packet->ReadUInt8();
		for (unsigned char i = 0; i < 2; i++) {
			roomSettings.unk80000000_2[i].unk8000000_vec_1 = packet->ReadUInt32_LE();
			roomSettings.unk80000000_2[i].unk8000000_vec_2 = packet->ReadUInt32_LE();
			roomSettings.unk80000000_2[i].unk8000000_vec_3 = packet->ReadUInt8();
			roomSettings.unk80000000_2[i].unk8000000_vec_4 = packet->ReadUInt8();
			roomSettings.unk80000000_2[i].unk8000000_vec_5 = packet->ReadUInt8();
			roomSettings.unk80000000_2[i].unk8000000_vec_6 = packet->ReadUInt8();
			roomSettings.unk80000000_2[i].unk8000000_vec_7 = packet->ReadUInt16_LE();
			roomSettings.unk80000000_2[i].unk8000000_vec_8 = packet->ReadUInt8();
			roomSettings.unk80000000_2[i].unk8000000_vec_9 = packet->ReadUInt8();
		}
	}
	if (roomSettings.highFlag & ROOMSETTINGS_HFLAG_UNK1) {
		roomSettings.unkh1_1 = packet->ReadUInt32_LE();
		roomSettings.unkh1_2 = packet->ReadString();
		roomSettings.unkh1_3 = packet->ReadUInt8();
		roomSettings.unkh1_4 = packet->ReadUInt8();
		roomSettings.unkh1_5 = packet->ReadUInt8();
	}
	if (roomSettings.highFlag & ROOMSETTINGS_HFLAG_UNK2) {
		roomSettings.unkh2 = packet->ReadUInt8();
	}

	room->UpdateRoomSettings(roomSettings);

	roomManager.SendUpdateRoomPacketToAll(room, roomListFlag);

	vector<User*> roomUsers = room->GetRoomUsers();

	roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [](User* u) {
		return (u == NULL || u->GetConnection() == NULL);
	}), roomUsers.end());

	for (auto& u : roomUsers) {
		SendPacket_Room_ReplyUpdateRoomSettings(u->GetConnection(), roomSettings);
	}
}

void Packet_RoomManager::parsePacket_Room_ResultConfirm(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ResultConfirm, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ResultConfirm, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	unsigned char unk = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ResultConfirm - unk: {}\n", user->GetUserLogName(), unk));

	packet_HostManager.SendPacket_Host_ResultClose(connection);
}

void Packet_RoomManager::parsePacket_Room_ReturnToRoom(User* user) {
	if (user == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom && user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ReturnToRoom, but it's not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ReturnToRoom, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ReturnToRoom, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room ReturnToRoom\n", user->GetUserLogName()));

	GameMatchUser* gameMatchUser = gameMatch->GetGameMatchUserByUser(user);
	if (gameMatchUser) {
		gameMatch->RemoveGameMatchUser(gameMatchUser);
	}

	if (user == room->GetRoomHostUser()) {
		vector<GameMatchUser*> gameMatchUsers = gameMatch->GetGameMatchUsers();

		gameMatchUsers.erase(remove_if(gameMatchUsers.begin(), gameMatchUsers.end(), [](GameMatchUser* gameMatchUser) {
			return (gameMatchUser->GetUser() == NULL || gameMatchUser->GetUser()->GetConnection() == NULL);
		}), gameMatchUsers.end());

		if (gameMatchUsers.empty()) {
			delete gameMatch;
			room->SetGameMatch(NULL);

			RoomSettings roomSettings;
			roomSettings.lowFlag = ROOMSETTINGS_LFLAG_ROOMSTATE;
			roomSettings.roomState = RoomState::Waiting;
			room->UpdateRoomSettings(roomSettings);

			roomManager.SendUpdateRoomPacketToAll(room, ROOMLIST_FLAG_ROOMSTATE);

			vector<User*> roomUsers = room->GetRoomUsers();

			roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [](User* u) {
				return (u == NULL || u->GetConnection() == NULL);
			}), roomUsers.end());

			for (auto& u : roomUsers) {
				packet_RoomManager.SendPacket_Room_ReplyUpdateRoomSettings(u->GetConnection(), roomSettings);
				roomManager.SendUpdateRoomPacket(u->GetConnection(), room, ROOMLIST_FLAG_ROOMSTATE);
			}
		}
		else {
			User* newRoomHostUser = gameMatchUsers[0]->GetUser();
			room->UpdateRoomHostUser(newRoomHostUser);

			gameMatchUsers.erase(remove_if(gameMatchUsers.begin(), gameMatchUsers.end(), [newRoomHostUser](GameMatchUser* gameMatchUser) {
				return (gameMatchUser->GetUser() == newRoomHostUser);
			}), gameMatchUsers.end());

			unsigned long newRoomHostUserID = newRoomHostUser->GetUserID();
			const vector<unsigned char>& saveData = gameMatch->GetSaveData();

			packet_HostManager.SendPacket_Host_HostRestart(newRoomHostUser->GetConnection(), true, newRoomHostUserID, gameMatchUsers, saveData);

			for (auto& gameMatchUser : gameMatchUsers) {
				packet_HostManager.SendPacket_Host_HostRestart(gameMatchUser->GetUser()->GetConnection(), false, newRoomHostUserID);
				packet_HostManager.SendPacket_Host_HostJoin(gameMatchUser->GetUser()->GetConnection(), newRoomHostUserID);
			}

			if (saveData.empty()) {
				delete gameMatch;
				room->SetGameMatch(NULL);
			}
		}
	}

	user->SetUserStatus(UserStatus::InRoom);
}

void Packet_RoomManager::buildRoomInfo(TCPConnection::Packet::pointer packet, Room* room, unsigned short flag) {
	if (packet == NULL || room == NULL) {
		return;
	}

	packet->WriteUInt16_LE(room->GetRoomID());
	packet->WriteUInt16_LE(flag);

	const RoomSettings& roomSettings = room->GetRoomSettings();
	const UserNetwork& userNetwork = room->GetRoomHostUser()->GetUserNetwork();

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
		packet->WriteUInt8(roomSettings.levelLimit);
	}
	if (flag & ROOMLIST_FLAG_GAMEMODEID) {
		packet->WriteUInt8(roomSettings.gameModeID);
	}
	if (flag & ROOMLIST_FLAG_MAPID) {
		packet->WriteUInt8(roomSettings.mapID);
	}
	if (flag & ROOMLIST_FLAG_CURRENTPLAYERS) {
		packet->WriteUInt8((unsigned char)room->GetRoomUsers().size());
	}
	if (flag & ROOMLIST_FLAG_MAXPLAYERS) {
		packet->WriteUInt8(roomSettings.maxPlayers);
	}
	if (flag & ROOMLIST_FLAG_WEAPONLIMIT) {
		packet->WriteUInt8(roomSettings.weaponLimit);
	}
	if (flag & ROOMLIST_FLAG_ROOMHOST) {
		packet->WriteUInt32_LE(room->GetRoomHostUser()->GetUserID());
		packet->WriteUInt8(0); // unk
	}
	if (flag & ROOMLIST_FLAG_UNK400) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & ROOMLIST_FLAG_UNK800) {
		packet->WriteUInt32_LE(userNetwork.externalIP); // unk
		packet->WriteUInt16_LE(userNetwork.externalHostPort); // unk
		packet->WriteUInt32_LE(userNetwork.localIP); // unk
		packet->WriteUInt16_LE(userNetwork.localHostPort); // unk
		packet->WriteUInt8(0); // unk
	}
	if (flag & ROOMLIST_FLAG_UNK1000) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & ROOMLIST_FLAG_UNK2000) {
		packet->WriteUInt8(0); // unk
	}
	if (flag & ROOMLIST_FLAG_ROOMSTATE) {
		packet->WriteUInt8((unsigned char)roomSettings.roomState);
	}
}

void Packet_RoomManager::buildRoomSettings(TCPConnection::Packet::pointer packet, const RoomSettings& roomSettings) {
	if (packet == NULL) {
		return;
	}

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
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_LEVELLIMIT) {
		packet->WriteUInt8(roomSettings.levelLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK20) {
		packet->WriteUInt8(roomSettings.unk20); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_GAMEMODEID) {
		packet->WriteUInt8(roomSettings.gameModeID);
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
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_KILLLIMIT) {
		packet->WriteUInt16_LE(roomSettings.killLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_TIMELIMIT) {
		packet->WriteUInt8(roomSettings.timeLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROUNDTIME) {
		packet->WriteUInt8(roomSettings.roundTime);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_WEAPONLIMIT) {
		packet->WriteUInt8(roomSettings.weaponLimit);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_HOSTAGEPENALTY) {
		packet->WriteUInt8(roomSettings.hostagePenalty);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FREEZETIME) {
		packet->WriteUInt8(roomSettings.freezeTime);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_BUYTIME) {
		packet->WriteUInt8(roomSettings.buyTime);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_NICKNAMEDISPLAY) {
		packet->WriteUInt8(roomSettings.nickNameDisplay);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK40000) {
		packet->WriteUInt8(roomSettings.unk40000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_UNK80000) {
		packet->WriteUInt8(roomSettings.unk80000); // unk
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FRIENDLYFIRE) {
		packet->WriteUInt8(roomSettings.friendlyFire);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FLASHLIGHT) {
		packet->WriteUInt8(roomSettings.flashLight);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_FOOTSTEPS) {
		packet->WriteUInt8(roomSettings.footSteps);
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
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_DEATHCAMERATYPE) {
		packet->WriteUInt8(roomSettings.deathCameraType);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_VOICECHAT) {
		packet->WriteUInt8(roomSettings.voiceChat);
	}
	if (roomSettings.lowFlag & ROOMSETTINGS_LFLAG_ROOMSTATE) {
		packet->WriteUInt8((unsigned char)roomSettings.roomState);
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
	if (packet == NULL || gameUser.user == NULL) {
		return;
	}

	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packet->WriteUInt8(0); // unk
	packet->WriteUInt8(0); // unk

	const UserNetwork& userNetwork = gameUser.user->GetUserNetwork();

	packet->WriteUInt32_LE(userNetwork.externalIP);
	packet->WriteUInt16_LE(userNetwork.externalHostPort);
	packet->WriteUInt16_LE(userNetwork.externalGuestPort);
	packet->WriteUInt32_LE(userNetwork.localIP);
	packet->WriteUInt16_LE(userNetwork.localHostPort);
	packet->WriteUInt16_LE(userNetwork.localGuestPort);

	packetManager.BuildUserCharacter(packet, gameUser.userCharacter);
}

void Packet_RoomManager::sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, Room* room, const vector<GameUser>& gameUsers) {
	if (connection == NULL || room == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	User* roomHostUser = room->GetRoomHostUser();
	if (roomHostUser == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::ReplyCreateAndJoin);
	packet->WriteUInt32_LE(roomHostUser->GetUserID());
	packet->WriteUInt16_LE(room->GetRoomID());
	packet->WriteUInt8(0); // ping, 0~1 - Worst, 2 - Bad, 3 - Normal, >=4 - Good

	buildRoomSettings(packet, room->GetRoomSettings());

	packet->WriteUInt8((unsigned char)gameUsers.size());

	for (auto& gameUser : gameUsers) {
		buildRoomUserInfo(packet, gameUser);
	}

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_UserJoin(TCPConnection::pointer connection, const GameUser& gameUser) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::UserJoin);
	buildRoomUserInfo(packet, gameUser);

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_UserReady(TCPConnection::pointer connection, unsigned long userID, unsigned char ready) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::UserReady);
	packet->WriteUInt32_LE(userID);
	packet->WriteUInt8(ready);

	packet->Send();
}

void Packet_RoomManager::sendPacket_Room_LeaveRoomInGame(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Room });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_RoomType::LeaveRoomInGame);

	packet->Send();
}