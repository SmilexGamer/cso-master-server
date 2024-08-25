#pragma once
//#define NO_SSL

#define SERVERCONNECTED "~SERVERCONNECTED\n\0"

#define PACKET_SIGNATURE 'U'

#define PACKET_HEADER_SIZE 4

#define PACKET_MAX_SIZE 0x10000

#define LAUNCHER_VERSION 67

#define CLIENT_VERSION 11

#define CLIENT_BUILD_TIMESTAMP "11.06.08"

#define CLIENT_NAR_CHECKSUM 1860010506

enum PacketID {
	Version = 0,
	Reply = 1,
	Transfer = 2,
	Login = 3, // or Auth, whatever you want to name it
	ServerList = 5,
	Character = 6,
	Crypt = 7,
	ServerChannel = 10,
	RecvCrypt = 12,
	Room = 65,
	ClientCheck = 66,
	UMsg = 67,
	Host = 68,
	UpdateInfo = 69,
	Udp = 70,
	Clan = 71,
	Shop = 72,
	Rank = 73,
	Option = 76,
	Favorite = 77,
	Item = 78,
	GameGuard = 79,
	SearchRoom = 80,
	Report = 83,
	UserStart = 150,
	RoomList = 151,
	Lobby = 153,
	Inventory = 154,
	ClanStock = 155,
	CafeItems = 156
};

enum Packet_ReplyType {
	LoginSuccess = 0,
	CreateCharacter = 1,
	InvalidName = 3,
	Playing = 4,
	NotExist = 5,
	WrongPassword = 7,
	InvalidPassword = 8,
	ServerDown = 9,
	SysError = 10,
	InvalidServer = 11,
	InvalidSlot = 12,
	ID_NOT_ALLOWD = 14,
	NEXONCOMERROR = 15,
	NEXONCOM_PASSPORTERROR = 16,
	NEXONCOM_INVALID_SESSION = 17,
	INVALID_USERINFO = 18,
	TRANSFER_ERR = 19,
	ID_TOO_SHORT = 20,
	ID_TOO_LONG = 21,
	INVALID_CHAR = 22,
	ID_DIGIT_BEFORE_CHAR = 23,
	ID_EXCEED_CHAR_COUNT = 24,
	ALREADY_EXIST = 25,
	ID_PROHIBITED = 26,
	EXCEED_MAX_CONNECTION = 27,
	NEXONCOM_UNDER_AGE = 28,
	INVALID_CLIENT_VERSION = 38
};

enum Packet_UMsgType {
	WhisperChat = 0,
	LobbyChat = 1,
	FamilyChat = 3,
	PartyChat = 5
};

enum UserInfoFlag {
	Unk1 = 0x1,
	NickName = 0x2,
	Unk4 = 0x4,
	Level = 0x8,
	Unk10 = 0x10,
	Exp = 0x20,
	Cash = 0x40,
	Point = 0x80,
	BattleStats = 0x100,
	Location = 0x200,
	Unk400 = 0x400,
	Unk800 = 0x800,
	Unk1000 = 0x1000,
	All = 0xFFFF
};