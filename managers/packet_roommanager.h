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
#define ROOMLIST_FLAG_UNK200			0x200
#define ROOMLIST_FLAG_UNK400			0x400
#define ROOMLIST_FLAG_UNK800			0x800
#define ROOMLIST_FLAG_UNK1000			0x1000
#define ROOMLIST_FLAG_UNK2000			0x2000
#define ROOMLIST_FLAG_UNK4000			0x4000
#define ROOMLIST_FLAG_ALL				0xFFFF

enum Packet_RoomType {
	RequestCreate = 0,
	ReplyCreateAndJoin = 0,
	RequestJoin = 1,
	UserJoin = 1,
	RequestLeave = 2,
	UserLeave = 2,
	RequestStartGame = 4,
	ReplyUpdateRoomSettings = 4,
	RequestUpdateRoomSettings = 5,
	ReplyLeaveRoomInGame = 10
};

enum Packet_RoomListType {
	FullRoomList = 0,
	AddRoom = 1,
	RemoveRoom = 2
};

class Packet_RoomManager {
public:
	void ParsePacket_Room(TCPConnection::Packet::pointer packet);
	void SendPacket_RoomList_FullRoomList(TCPConnection::pointer connection, const vector<Room*>& rooms, unsigned short flag);
	void SendPacket_RoomList_AddRoom(TCPConnection::pointer connection, Room* room, unsigned short flag);
	void SendPacket_RoomList_RemoveRoom(TCPConnection::pointer connection, unsigned short roomID);
	void SendPacket_Room_UserLeave(TCPConnection::pointer connection, unsigned long userID);

private:
	void parsePacket_Room_RequestCreate(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_RequestJoin(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_RequestLeave(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_RequestStartGame(User* user);
	void parsePacket_Room_RequestUpdateRoomSettings(User* user, TCPConnection::Packet::pointer packet);
	void buildRoomInfo(TCPConnection::Packet::pointer packet, Room* room, unsigned short flag);
	void buildRoomSettings(TCPConnection::Packet::pointer packet, const RoomSettings& roomSettings);
	void buildRoomUserInfo(TCPConnection::Packet::pointer packet, const GameUser& gameUser);
	void sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, Room* room, const vector<GameUser>& gameUsers);
	void sendPacket_Room_UserJoin(TCPConnection::pointer connection, const GameUser& gameUser);
	void sendPacket_Room_ReplyLeaveRoomInGame(TCPConnection::pointer connection);
	void sendPacket_Room_ReplyUpdateRoomSettings(TCPConnection::pointer connection, const RoomSettings& roomSettings);
};

extern Packet_RoomManager packet_RoomManager;