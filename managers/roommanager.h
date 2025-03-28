#pragma once
#include "room.h"

class RoomManager {
public:
	~RoomManager();

	const vector<Room*>& GetRooms() const noexcept {
		return _rooms;
	}

	bool AddRoom(Room* room);
	void RemoveRoom(Room* room);
	Room* GetRoomByRoomID(unsigned short roomID);
	void RemoveRoomByRoomID(unsigned short roomID);
	unsigned short GetFreeRoomID();

private:
	vector<Room*> _rooms;
};

extern RoomManager roomManager;