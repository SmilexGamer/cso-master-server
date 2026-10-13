#pragma once
#include "room.h"

class RoomManager {
public:
	~RoomManager();

	const vector<Room*>& GetRooms() const noexcept {
		return _rooms;
	}

	char AddRoom(Room* room);
	void UpdateRoom(Room* room);
	SearchRoomResult SearchRoom(unsigned char level, unsigned char gameModeID, unsigned char mapID, unsigned char levelLimit, unsigned char clanBattle);
	void RemoveRoom(Room* room);
	void RemoveAllRooms();
	Room* GetRoomByRoomID(unsigned short roomID);
	unsigned short GetFreeRoomID();
	void SendFullRoomListPacket(TCPConnection::pointer connection);
	void SendAddRoomPacketToAll(Room* room, unsigned short flag);
	void SendRemoveRoomPacketToAll(unsigned short roomID);
	void SendUpdateRoomPacket(TCPConnection::pointer connection, Room* room, unsigned short flag);
	void SendUpdateRoomPacketToAll(Room* room, unsigned short flag);
	void SendRoomUserLeavePacket(TCPConnection::pointer connection, unsigned long userID);

private:
	vector<Room*> _rooms;
};

extern RoomManager roomManager;