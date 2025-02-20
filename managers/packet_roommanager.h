#pragma once
#include "tcp_connection.h"
#include "roomsettings.h"

class Packet_RoomManager {
public:
	void ParsePacket_Room(TCPConnection::Packet::pointer packet);

private:
	void parsePacket_Room_RequestCreate(TCPConnection::Packet::pointer packet);
	void parsePacket_Room_RequestStartGame(TCPConnection::Packet::pointer packet);
	void sendPacket_Room_ReplyCreateAndJoin(TCPConnection::pointer connection, RoomSettings roomSettings);
};

extern Packet_RoomManager packet_RoomManager;