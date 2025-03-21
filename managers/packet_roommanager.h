#pragma once
#include "tcp_connection.h"
#include "roomsettings.h"
#include "user.h"

class Packet_RoomManager {
public:
	void ParsePacket_Room(TCPConnection::Packet::pointer packet);

private:
	void parsePacket_Room_RequestCreate(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Room_RequestStartGame(User* user);
	void parsePacket_Room_RequestLeave(User* user);
	void sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, const RoomSettings& roomSettings);
	void sendPacket_Room_ReplyLeaveRoom(User* user);
	void sendPacket_Room_ReplyLeaveRoomInGame(TCPConnection::pointer connection);
};

extern Packet_RoomManager packet_RoomManager;