#include "room.h"

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

	AddRoomUser(roomHostUser);
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

	return 1;
}

void Room::RemoveRoomUser(User* user) {
	if (user == NULL) {
		return;
	}

	_roomUsers.erase(find(_roomUsers.begin(), _roomUsers.end(), user));
}