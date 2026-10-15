#pragma once
#include "user.h"
#include "gamematch.h"

constexpr auto ROOMSETTINGS_LFLAG_ROOMNAME =		0x1;
constexpr auto ROOMSETTINGS_LFLAG_UNK2 =			0x2;
constexpr auto ROOMSETTINGS_LFLAG_UNK4 =			0x4;
constexpr auto ROOMSETTINGS_LFLAG_PASSWORD =		0x8;
constexpr auto ROOMSETTINGS_LFLAG_LEVELLIMIT =		0x10;
constexpr auto ROOMSETTINGS_LFLAG_UNK20 =			0x20;
constexpr auto ROOMSETTINGS_LFLAG_GAMEMODEID =		0x40;
constexpr auto ROOMSETTINGS_LFLAG_MAPID =			0x80;
constexpr auto ROOMSETTINGS_LFLAG_MAXPLAYERS =		0x100;
constexpr auto ROOMSETTINGS_LFLAG_WINLIMIT =		0x200;
constexpr auto ROOMSETTINGS_LFLAG_KILLLIMIT =		0x400;
constexpr auto ROOMSETTINGS_LFLAG_TIMELIMIT =		0x800;
constexpr auto ROOMSETTINGS_LFLAG_ROUNDTIME =		0x1000;
constexpr auto ROOMSETTINGS_LFLAG_WEAPONLIMIT =		0x2000;
constexpr auto ROOMSETTINGS_LFLAG_HOSTAGEPENALTY =	0x4000;
constexpr auto ROOMSETTINGS_LFLAG_FREEZETIME =		0x8000;
constexpr auto ROOMSETTINGS_LFLAG_BUYTIME =			0x10000;
constexpr auto ROOMSETTINGS_LFLAG_NICKNAMEDISPLAY = 0x20000;
constexpr auto ROOMSETTINGS_LFLAG_UNK40000 =		0x40000;
constexpr auto ROOMSETTINGS_LFLAG_UNK80000 =		0x80000;
constexpr auto ROOMSETTINGS_LFLAG_FRIENDLYFIRE =	0x100000;
constexpr auto ROOMSETTINGS_LFLAG_FLASHLIGHT =		0x200000;
constexpr auto ROOMSETTINGS_LFLAG_FOOTSTEPS =		0x400000;
constexpr auto ROOMSETTINGS_LFLAG_UNK800000 =		0x800000;
constexpr auto ROOMSETTINGS_LFLAG_UNK1000000 =		0x1000000;
constexpr auto ROOMSETTINGS_LFLAG_UNK2000000 =		0x2000000;
constexpr auto ROOMSETTINGS_LFLAG_UNK4000000 =		0x4000000;
constexpr auto ROOMSETTINGS_LFLAG_UNK8000000 =		0x8000000;
constexpr auto ROOMSETTINGS_LFLAG_DEATHCAMERATYPE = 0x10000000;
constexpr auto ROOMSETTINGS_LFLAG_VOICECHAT =		0x20000000;
constexpr auto ROOMSETTINGS_LFLAG_ROOMSTATE =		0x40000000;
constexpr auto ROOMSETTINGS_LFLAG_UNK80000000 =		0x80000000;
constexpr auto ROOMSETTINGS_LFLAG_ALL =				0xFFFFFFFF;

constexpr auto ROOMSETTINGS_HFLAG_UNK1 =			0x1;
constexpr auto ROOMSETTINGS_HFLAG_UNK2 =			0x2;
constexpr auto ROOMSETTINGS_HFLAG_ALL =				0xFF;

enum class RoomState {
	Waiting = 0,
	Playing = 2
};

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
	unsigned char levelLimit = 0;
	unsigned char unk20 = 0;
	unsigned char gameModeID = 0;
	unsigned char mapID = 0;
	unsigned char maxPlayers = 0;
	unsigned char winLimit = 0;
	unsigned short killLimit = 0;
	unsigned char timeLimit = 0;
	unsigned char roundTime = 0;
	unsigned char weaponLimit = 0;
	unsigned char hostagePenalty = 0;
	unsigned char freezeTime = 0;
	unsigned char buyTime = 0;
	unsigned char nickNameDisplay = 0;
	unsigned char unk40000 = 0;
	unsigned char unk80000 = 0;
	bool friendlyFire = false;
	bool flashLight = false;
	bool footSteps = false;
	unsigned char unk800000 = 0;
	unsigned char unk1000000 = 0;
	unsigned char unk2000000 = 0;
	unsigned char unk4000000 = 0;
	unsigned char unk8000000 = 0;
	unsigned char deathCameraType = 0;
	bool voiceChat = false;
	RoomState roomState = RoomState::Waiting;
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
	Room(unsigned short roomID, User* roomHostUser);
	~Room();

	unsigned short GetRoomID() const noexcept {
		return _roomID;
	}

	User* GetRoomHostUser() const noexcept {
		return _roomHostUser;
	}

	const RoomSettings& GetRoomSettings() const noexcept {
		return _roomSettings;
	}

	void SetRoomSettings(const RoomSettings& roomSettings) noexcept {
		_roomSettings = roomSettings;
	}

	const vector<User*>& GetRoomUsers() const noexcept {
		return _roomUsers;
	}

	GameMatch* GetGameMatch() const noexcept {
		return _gameMatch;
	}

	void SetGameMatch(GameMatch* gameMatch) noexcept {
		_gameMatch = gameMatch;
	}

	char AddRoomUser(User* user);
	void RemoveRoomUser(User* user);
	void UpdateRoomHostUser(User* user);
	void UpdateRoomSettings(const RoomSettings& roomSettings);

private:
	unsigned short _roomID;
	User* _roomHostUser;
	RoomSettings _roomSettings;
	vector<User*> _roomUsers;
	GameMatch* _gameMatch;
};