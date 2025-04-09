#pragma once
#include "user.h"

#define ROOMSETTINGS_LFLAG_ROOMNAME		0x1
#define ROOMSETTINGS_LFLAG_UNK2			0x2
#define ROOMSETTINGS_LFLAG_UNK4			0x4
#define ROOMSETTINGS_LFLAG_PASSWORD		0x8
#define ROOMSETTINGS_LFLAG_UNK10		0x10
#define ROOMSETTINGS_LFLAG_UNK20		0x20
#define ROOMSETTINGS_LFLAG_UNK40		0x40
#define ROOMSETTINGS_LFLAG_MAPID		0x80
#define ROOMSETTINGS_LFLAG_MAXPLAYERS	0x100
#define ROOMSETTINGS_LFLAG_WINLIMIT		0x200
#define ROOMSETTINGS_LFLAG_UNK400		0x400
#define ROOMSETTINGS_LFLAG_TIMELIMIT	0x800
#define ROOMSETTINGS_LFLAG_ROUNDTIME	0x1000
#define ROOMSETTINGS_LFLAG_UNK2000		0x2000
#define ROOMSETTINGS_LFLAG_UNK4000		0x4000
#define ROOMSETTINGS_LFLAG_UNK8000		0x8000
#define ROOMSETTINGS_LFLAG_UNK10000		0x10000
#define ROOMSETTINGS_LFLAG_UNK20000		0x20000
#define ROOMSETTINGS_LFLAG_UNK40000		0x40000
#define ROOMSETTINGS_LFLAG_UNK80000		0x80000
#define ROOMSETTINGS_LFLAG_UNK100000	0x100000
#define ROOMSETTINGS_LFLAG_UNK200000	0x200000
#define ROOMSETTINGS_LFLAG_UNK400000	0x400000
#define ROOMSETTINGS_LFLAG_UNK800000	0x800000
#define ROOMSETTINGS_LFLAG_UNK1000000	0x1000000
#define ROOMSETTINGS_LFLAG_UNK2000000	0x2000000
#define ROOMSETTINGS_LFLAG_UNK4000000	0x4000000
#define ROOMSETTINGS_LFLAG_UNK8000000	0x8000000
#define ROOMSETTINGS_LFLAG_UNK10000000	0x10000000
#define ROOMSETTINGS_LFLAG_UNK20000000	0x20000000
#define ROOMSETTINGS_LFLAG_UNK40000000	0x40000000
#define ROOMSETTINGS_LFLAG_UNK80000000	0x80000000
#define ROOMSETTINGS_LFLAG_ALL			0xFFFFFFFF

#define ROOMSETTINGS_HFLAG_UNK1			0x1
#define ROOMSETTINGS_HFLAG_UNK2			0x2
#define ROOMSETTINGS_HFLAG_ALL			0xFF

struct unk80000000_vec {
	unsigned long unk8000000_vec_1 = 0;
	unsigned long unk8000000_vec_2 = 0;
	unsigned char unk8000000_vec_3 = 0;
	unsigned char unk8000000_vec_4 = 0;
	unsigned char unk8000000_vec_5 = 0;
	unsigned char unk8000000_vec_6 = 0;
	unsigned short unk8000000_vec_7 = 0;
	unsigned char unk8000000_vec_8 = 0;
	unsigned char unk8000000_vec_9 = 0;
};

struct RoomSettings {
	unsigned long lowFlag = 0;
	unsigned char highFlag = 0;
	string roomName = "";
	unsigned char unk2 = 0;
	unsigned char unk4_1 = 0;
	unsigned char unk4_2 = 0;
	unsigned char unk4_3 = 0;
	unsigned long unk4_4 = 0;
	string password = "";
	unsigned char unk10 = 0;
	unsigned char unk20 = 0;
	unsigned char unk40 = 0;
	unsigned char mapID = 0;
	unsigned char maxPlayers = 0;
	unsigned char winLimit = 0;
	unsigned short unk400 = 0;
	unsigned char timeLimit = 0;
	unsigned char roundTime = 0;
	unsigned char unk2000 = 0;
	unsigned char unk4000 = 0;
	unsigned char unk8000 = 0;
	unsigned char unk10000 = 0;
	unsigned char unk20000 = 0;
	unsigned char unk40000 = 0;
	unsigned char unk80000 = 0;
	unsigned char unk100000 = 0;
	unsigned char unk200000 = 0;
	unsigned char unk400000 = 0;
	unsigned char unk800000 = 0;
	unsigned char unk1000000 = 0;
	unsigned char unk2000000 = 0;
	unsigned char unk4000000 = 0;
	unsigned char unk8000000 = 0;
	unsigned char unk10000000 = 0;
	unsigned char unk20000000 = 0;
	unsigned char unk40000000 = 0;
	unsigned char unk80000000_1 = 0;
	unk80000000_vec unk80000000_2[2];
	unsigned long unkh1_1 = 0;
	string unkh1_2 = "";
	unsigned char unkh1_3 = 0;
	unsigned char unkh1_4 = 0;
	unsigned char unkh1_5 = 0;
	unsigned char unkh2 = 0;
};

class Room {
public:
	Room(unsigned short roomID, User* roomHostUser, TCPConnection::Packet::pointer packet);

	unsigned short GetRoomID() const noexcept {
		return _roomID;
	}

	User* GetRoomHostUser() const noexcept {
		return _roomHostUser;
	}

	const RoomSettings& GetRoomSettings() const noexcept {
		return _roomSettings;
	}

	const vector<User*>& GetRoomUsers() const noexcept {
		return _roomUsers;
	}

	void UpdateRoomSettings(const RoomSettings& newSettings) noexcept {
		_roomSettings = newSettings;
	}

	char AddRoomUser(User* user);
	void RemoveRoomUser(User* user);
	void UpdateRoomHostUser(User* user);

private:
	unsigned short _roomID;
	User* _roomHostUser;
	RoomSettings _roomSettings;
	vector<User*> _roomUsers;
};