#pragma once
#include "tcp_connection.h"

class RoomSettings {
public:
	RoomSettings(TCPConnection::Packet::pointer packet);
	
	void Init();

	unsigned long lowFlag;
	unsigned char highFlag;
	string roomName;
	unsigned char unk2;
	unsigned char maxPlayers;
	unsigned char gameModeID;
	unsigned char mapID;
	unsigned char winLimit;
	unsigned short killLimit;
	unsigned char timeLimit;
	unsigned char roundTime;
	string password;
	unsigned char unk11;
	unsigned char unk12;
	unsigned char quickStart;
	unsigned char unk14;
	unsigned char unk15;
	unsigned long unk16;
};