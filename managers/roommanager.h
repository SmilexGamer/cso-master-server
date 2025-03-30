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
	void RemoveAllRooms();
	Room* GetRoomByRoomID(unsigned short roomID);
	void RemoveRoomByRoomID(unsigned short roomID);
	unsigned short GetFreeRoomID();
	void SendFullRoomListPacket(TCPConnection::pointer connection);
	void SendAddRoomPacketToAll(Room* room, unsigned short flag);
	void SendRemoveRoomPacketToAll(unsigned short roomID);
	void SendRoomUserLeavePacket(TCPConnection::pointer connection, unsigned long userID);

private:
	vector<Room*> _rooms;
};

extern RoomManager roomManager;