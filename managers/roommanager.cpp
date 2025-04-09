#include "roommanager.h"
#include "usermanager.h"
#include "packet_roommanager.h"

RoomManager roomManager;

RoomManager::~RoomManager() {
	RemoveAllRooms();
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

	roomManager.SendRemoveRoomPacketToAll(room->GetRoomID());
	_rooms.erase(find(_rooms.begin(), _rooms.end(), room));
	delete room;
	room = NULL;
}

void RoomManager::RemoveAllRooms() {
	for (auto& room : _rooms) {
		delete room;
		room = NULL;
	}

	_rooms.clear();
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
	for (unsigned short roomID = 1; roomID < UINT16_MAX; roomID++) {
		if (GetRoomByRoomID(roomID) != NULL) {
			continue;
		}

		return roomID;
	}

	return 0;
}

void RoomManager::SendFullRoomListPacket(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	packet_RoomManager.SendPacket_RoomList_FullRoomList(connection, _rooms, ROOMLIST_FLAG_ALL);
}

void RoomManager::SendAddRoomPacketToAll(Room* room, unsigned short flag) {
	const vector<User*>& users = userManager.GetUsers();

	for (auto& user : users) {
		if (user->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		packet_RoomManager.SendPacket_RoomList_AddRoom(user->GetConnection(), room, flag);
	}
}

void RoomManager::SendRemoveRoomPacketToAll(unsigned short roomID) {
	const vector<User*>& users = userManager.GetUsers();

	for (auto& user : users) {
		if (user->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		packet_RoomManager.SendPacket_RoomList_RemoveRoom(user->GetConnection(), roomID);
	}
}

void RoomManager::SendUpdateRoomPacket(TCPConnection::pointer connection, Room* room, unsigned short flag) {
	if (connection == NULL) {
		return;
	}

	packet_RoomManager.SendPacket_RoomList_UpdateRoom(connection, room, flag);
}

void RoomManager::SendUpdateRoomPacketToAll(Room* room, unsigned short flag) {
	const vector<User*>& users = userManager.GetUsers();

	for (auto& user : users) {
		if (user->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		packet_RoomManager.SendPacket_RoomList_UpdateRoom(user->GetConnection(), room, flag);
	}
}

void RoomManager::SendRoomUserLeavePacket(TCPConnection::pointer connection, unsigned long userID) {
	if (connection == NULL) {
		return;
	}

	packet_RoomManager.SendPacket_Room_UserLeave(connection, userID);
}