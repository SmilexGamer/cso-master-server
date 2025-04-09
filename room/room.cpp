#include "roommanager.h"
#include "serverconsole.h"

Room::Room(unsigned short roomID, User* roomHostUser, TCPConnection::Packet::pointer packet) : _roomID(roomID), _roomHostUser(roomHostUser) {
	_roomSettings.lowFlag = ROOMSETTINGS_LFLAG_ALL;
	_roomSettings.highFlag = ROOMSETTINGS_HFLAG_ALL;

	_roomSettings.roomName = packet->ReadString();
	unsigned char unk2 = packet->ReadUInt8();
	_roomSettings.maxPlayers = packet->ReadUInt8();
	unsigned char gameModeID = packet->ReadUInt8();
	_roomSettings.mapID = packet->ReadUInt8();
	_roomSettings.winLimit = packet->ReadUInt8();
	unsigned short killLimit = packet->ReadUInt16_LE();
	_roomSettings.timeLimit = packet->ReadUInt8();
	_roomSettings.roundTime = packet->ReadUInt8();
	_roomSettings.password = packet->ReadString();
	unsigned char unk11 = packet->ReadUInt8();
	unsigned char unk12 = packet->ReadUInt8();
	unsigned char fastStart = packet->ReadUInt8();
	unsigned char unk14 = packet->ReadUInt8();
	unsigned char unk15 = packet->ReadUInt8();
	unsigned char unk16 = packet->ReadUInt32_LE();

	serverConsole.Print(PrefixType::Info, format("[ Packet_RoomManager ] User ({}) has sent Packet_Room RequestCreate - roomName: {}, unk2: {}, maxPlayers: {}, gameModeID: {}, mapID: {}, winLimit: {}, killLimit: {}, timeLimit: {}, roundTime: {}, password: {}, unk11: {}, unk12: {}, quickStart: {}, unk14: {}, unk15: {}, unk16: {}\n", roomHostUser->GetUserLogName(), _roomSettings.roomName, unk2, _roomSettings.maxPlayers, gameModeID, _roomSettings.mapID, _roomSettings.winLimit, killLimit, _roomSettings.timeLimit, _roomSettings.roundTime, _roomSettings.password, unk11, unk12, fastStart, unk14, unk15, unk16));
}

char Room::AddRoomUser(User* user) {
	if (user == NULL) {
		return -1;
	}

	auto it = find_if(_roomUsers.begin(), _roomUsers.end(), [&user](User*& roomUser) { return roomUser == user; });
	if (it != _roomUsers.end()) {
		return 0;
	}

	_roomUsers.push_back(user);

	user->SetUserStatus(UserStatus::InRoom);
	user->SetCurrentRoomID(_roomID);

	return 1;
}

void Room::RemoveRoomUser(User* user) {
	if (user == NULL) {
		return;
	}

	_roomUsers.erase(find(_roomUsers.begin(), _roomUsers.end(), user));

	user->SetUserStatus(UserStatus::InLobby);
	user->SetCurrentRoomID(NULL);

	if (_roomUsers.empty()) {
		roomManager.RemoveRoom(this);
	}
	else {
		if (_roomHostUser == user) {
			UpdateRoomHostUser(_roomUsers[0]);
		}

		unsigned long userID = user->GetUserID();

		for (auto& u : _roomUsers) {
			roomManager.SendRoomUserLeavePacket(u->GetConnection(), userID);
		}
	}
}

void Room::UpdateRoomHostUser(User* user) {
	if (user == NULL) {
		return;
	}

	_roomHostUser = user;
}