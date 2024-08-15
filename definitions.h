#pragma once
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

enum Packet_UMsgID {
	WhisperChat = 0,
	LobbyChat = 1,
	FamilyChat = 3,
	PartyChat = 5,
};