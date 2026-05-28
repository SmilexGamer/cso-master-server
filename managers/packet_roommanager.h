#pragma once
#include "room.h"

#define ROOMLIST_FLAG_ROOMNAME			0x1
#define ROOMLIST_FLAG_UNK2				0x2
#define ROOMLIST_FLAG_ISPASSWORDED		0x4
#define ROOMLIST_FLAG_LEVELLIMIT		0x8
#define ROOMLIST_FLAG_GAMEMODEID		0x10
#define ROOMLIST_FLAG_MAPID				0x20
#define ROOMLIST_FLAG_CURRENTPLAYERS	0x40
#define ROOMLIST_FLAG_MAXPLAYERS		0x80
#define ROOMLIST_FLAG_WEAPONLIMIT		0x100
#define ROOMLIST_FLAG_ROOMHOST			0x200
#define ROOMLIST_FLAG_UNK400			0x400
#define ROOMLIST_FLAG_UNK800			0x800
#define ROOMLIST_FLAG_UNK1000			0x1000
#define ROOMLIST_FLAG_UNK2000			0x2000
#define ROOMLIST_FLAG_ROOMSTATE			0x4000
#define ROOMLIST_FLAG_ALL				0xFFFF

enum Packet_RoomType {
	CreateRoom = 0,
	ReplyCreateAndJoin = 0,
	JoinRoom = 1,
	UserJoin = 1,
	LeaveRoom = 2,
	UserLeave = 2,
	UserReady = 3,
	StartGame = 4,
	ReplyUpdateRoomSettings = 4,
	UpdateRoomSettings = 5,
	ResultConfirm = 6,
	GameResult = 6,
	LeaveRoomInGame = 10,
	ReturnToRoom = 11
};

enum Packet_RoomListType {
	FullRoomList = 0,
	AddRoom = 1,
	RemoveRoom = 2,
	UpdateRoom = 3
};

enum GameMode {
	Original = 0,
	DeathMatch = 1,
	TeamDeathMatch = 2,
	Bot = 3,
	BotDM = 4,
	BotTDM = 5,
	Official = 6,
	Official_TieBreak = 7
};

class Packet_RoomManager {
public:
	void ParsePacket_Room(TCPConnection::Packet::pointer packet);
	void SendPacket_RoomList_FullRoomList(TCPConnection::pointer connection, vector<Room*> rooms, unsigned short flag);
	void SendPacket_RoomList_AddRoom(TCPConnection::pointer connection, Room* room, unsigned short flag);
	void SendPacket_RoomList_RemoveRoom(TCPConnection::pointer connection, unsigned short roomID);
	void SendPacket_RoomList_UpdateRoom(TCPConnection::pointer connection, Room* room, unsigned short flag);
	void SendPacket_Room_UserLeave(TCPConnection::pointer connection, unsigned long userID);
	void SendPacket_Room_ReplyUpdateRoomSettings(TCPConnection::pointer connection, const RoomSettings& roomSettings);
	void SendPacket_Room_GameResult(TCPConnection::pointer connection);

private:
	void parsePacket_Room_CreateRoom(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_JoinRoom(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_LeaveRoom(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_StartGame(User* user);
	void parsePacket_Room_UpdateRoomSettings(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_ResultConfirm(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_ReturnToRoom(User* user);
	void buildRoomInfo(TCPConnection::Packet::pointer packet, Room* room, unsigned short flag);
	void buildRoomSettings(TCPConnection::Packet::pointer packet, const RoomSettings& roomSettings);
	void buildRoomUserInfo(TCPConnection::Packet::pointer packet, const GameUser& gameUser);
	void sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, Room* room, const vector<GameUser>& gameUsers);
	void sendPacket_Room_UserJoin(TCPConnection::pointer connection, const GameUser& gameUser);
	void sendPacket_Room_UserReady(TCPConnection::pointer connection, unsigned long userID, unsigned char ready);
	void sendPacket_Room_LeaveRoomInGame(TCPConnection::pointer connection);
};

extern Packet_RoomManager packet_RoomManager;