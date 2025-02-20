#include "roomsettings.h"
#include <iostream>

RoomSettings::RoomSettings(TCPConnection::Packet::pointer packet) {
	Init();

	lowFlag = 0xFFFFFFFF;
	highFlag = 0xFF;
	roomName = packet->ReadString();
	unk2 = packet->ReadUInt8();
	maxPlayers = packet->ReadUInt8();
	gameModeID = packet->ReadUInt8();
	mapID = packet->ReadUInt8();
	winLimit = packet->ReadUInt8();
	killLimit = packet->ReadUInt16_LE();
	timeLimit = packet->ReadUInt8();
	roundTime = packet->ReadUInt8();
	password = packet->ReadString();
	unk11 = packet->ReadUInt8();
	unk12 = packet->ReadUInt8();
	quickStart = packet->ReadUInt8();
	unk14 = packet->ReadUInt8();
	unk15 = packet->ReadUInt8();
	unk16 = packet->ReadUInt32_LE();
}

void RoomSettings::Init() {
	lowFlag = 0;
	highFlag = 0;
	roomName = "";
	unk2 = 0;
	maxPlayers = 0;
	gameModeID = 0;
	mapID = 0;
	winLimit = 0;
	killLimit = 0;
	timeLimit = 0;
	roundTime = 0;
	password = "";
	unk11 = 0;
	unk12 = 0;
	quickStart = 0;
	unk14 = 0;
	unk15 = 0;
	unk16 = 0;
}