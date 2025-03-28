#include "roommanager.h"

RoomManager roomManager;

RoomManager::~RoomManager() {
	for (auto& room : _rooms) {
		delete room;
		room = NULL;
	}

	_rooms.clear();
}

bool RoomManager::AddRoom(Room* room) {
	if (room == NULL) {
		return false;
	}

	_rooms.push_back(room);

	return true;
}

void RoomManager::RemoveRoom(Room* room) {
	if (room == NULL) {
		return;
	}

	_rooms.erase(find(_rooms.begin(), _rooms.end(), room));
	delete room;
	room = NULL;
}

Room* RoomManager::GetRoomByRoomID(unsigned short roomID) {
	if (!roomID) {
		return NULL;
	}

	auto it = find_if(_rooms.begin(), _rooms.end(), [&roomID](Room*& room) { return room->GetRoomID() == roomID; });
	if (it != _rooms.end()) {
		return _rooms.at(distance(_rooms.begin(), it));
	}

	return NULL;
}

void RoomManager::RemoveRoomByRoomID(unsigned short roomID) {
	if (!roomID) {
		return;
	}

	Room* room = GetRoomByRoomID(roomID);
	if (room == NULL) {
		return;
	}

	RemoveRoom(room);
}

unsigned short RoomManager::GetFreeRoomID() {
	for (unsigned short roomID = 1; roomID < 0xFFFF; roomID++) {
		if (GetRoomByRoomID(roomID) != NULL) {
			continue;
		}

		return roomID;
	}

	return 0;
}