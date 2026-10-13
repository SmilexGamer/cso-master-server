#include "roommanager.h"
#include "usermanager.h"
#include "databasemanager.h"
#include "serverconfig.h"
#include "packet_roommanager.h"

RoomManager roomManager;

RoomManager::~RoomManager() {
	RemoveAllRooms();
}

char RoomManager::AddRoom(Room* room) {
	if (room == NULL) {
		return false;
	}

	const RoomSettings& roomSettings = room->GetRoomSettings();

	char result = databaseManager.AddRoom(room->GetRoomID(), false, !roomSettings.password.empty(), roomSettings.levelLimit, roomSettings.gameModeID, roomSettings.mapID, (unsigned char)room->GetRoomUsers().size(), roomSettings.maxPlayers);
	if (result) {
		_rooms.push_back(room);
	}

	return result;
}

void RoomManager::UpdateRoom(Room* room) {
	const RoomSettings& roomSettings = room->GetRoomSettings();
	databaseManager.UpdateRoom(room->GetRoomID(), false, !roomSettings.password.empty(), roomSettings.levelLimit, roomSettings.gameModeID, roomSettings.mapID, (unsigned char)room->GetRoomUsers().size(), roomSettings.maxPlayers);
}

SearchRoomResult RoomManager::SearchRoom(unsigned char level, unsigned char gameModeID, unsigned char mapID, unsigned char levelLimit, unsigned char clanBattle) {
	auto it = find_if(_rooms.begin(), _rooms.end(), [gameModeID, mapID, levelLimit, clanBattle](Room*& room) {
		const RoomSettings& roomSettings = room->GetRoomSettings();
		if (!roomSettings.password.empty() || room->GetRoomUsers().size() >= roomSettings.maxPlayers || roomSettings.gameModeID != 99 && roomSettings.gameModeID != gameModeID || mapID != 99 && roomSettings.mapID != mapID || roomSettings.levelLimit != levelLimit) {
			return false;
		}

		return true;
	});
	if (it != _rooms.end()) {
		return { serverConfig.serverID, serverConfig.channelID, _rooms.at(distance(_rooms.begin(), it))->GetRoomID() };
	}

	return databaseManager.SearchRoom(level, gameModeID, mapID, levelLimit, clanBattle);
}

void RoomManager::RemoveRoom(Room* room) {
	if (room == NULL) {
		return;
	}

	unsigned short roomID = room->GetRoomID();

	roomManager.SendRemoveRoomPacketToAll(roomID);
	databaseManager.RemoveRoom(roomID);

	_rooms.erase(find(_rooms.begin(), _rooms.end(), room));

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch) {
		delete gameMatch;
		room->SetGameMatch(NULL);
	}

	delete room;
	room = NULL;
}

void RoomManager::RemoveAllRooms() {
	for (auto& room : _rooms) {
		GameMatch* gameMatch = room->GetGameMatch();
		if (gameMatch) {
			delete gameMatch;
			room->SetGameMatch(NULL);
		}

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

unsigned short RoomManager::GetFreeRoomID() {
	for (unsigned short roomID = 1; roomID < UINT16_MAX; roomID++) {
		if (GetRoomByRoomID(roomID)) {
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
	if (room == NULL) {
		return;
	}

	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* user) {
		return (user == NULL || user->GetConnection() == NULL || user->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	for (auto& user : users) {
		packet_RoomManager.SendPacket_RoomList_AddRoom(user->GetConnection(), room, flag);
	}
}

void RoomManager::SendRemoveRoomPacketToAll(unsigned short roomID) {
	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* user) {
		return (user == NULL || user->GetConnection() == NULL || user->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	for (auto& user : users) {
		packet_RoomManager.SendPacket_RoomList_RemoveRoom(user->GetConnection(), roomID);
	}
}

void RoomManager::SendUpdateRoomPacket(TCPConnection::pointer connection, Room* room, unsigned short flag) {
	if (connection == NULL || room == NULL) {
		return;
	}

	packet_RoomManager.SendPacket_RoomList_UpdateRoom(connection, room, flag);
}

void RoomManager::SendUpdateRoomPacketToAll(Room* room, unsigned short flag) {
	if (room == NULL) {
		return;
	}

	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* user) {
		return (user == NULL || user->GetConnection() == NULL || user->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	for (auto& user : users) {
		packet_RoomManager.SendPacket_RoomList_UpdateRoom(user->GetConnection(), room, flag);
	}
}

void RoomManager::SendRoomUserLeavePacket(TCPConnection::pointer connection, unsigned long userID) {
	if (connection == NULL) {
		return;
	}

	packet_RoomManager.SendPacket_Room_UserLeave(connection, userID);
}