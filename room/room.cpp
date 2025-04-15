#include "roommanager.h"
#include "packet_roommanager.h"
#include "serverconsole.h"

Room::Room(unsigned short roomID, User* roomHostUser) : _roomID(roomID), _roomHostUser(roomHostUser) {
	_roomSettings.lowFlag = ROOMSETTINGS_LFLAG_ALL;
	_roomSettings.highFlag = ROOMSETTINGS_HFLAG_ALL;
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

	roomManager.SendUpdateRoomPacketToAll(this, ROOMLIST_FLAG_CURRENTPLAYERS);

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

	unsigned long userID = user->GetUserID();

	for (auto& u : _roomUsers) {
		roomManager.SendUpdateRoomPacket(u->GetConnection(), this, ROOMLIST_FLAG_ROOMHOST);
	}
}