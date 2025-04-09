#pragma once
#include "tcp_connection.h"

#define NICKNAME_MIN_SIZE 4
#define NICKNAME_MAX_SIZE 16
#define NICKNAME_MAX_CHAR_COUNT 3

class Packet_CharacterManager {
public:
	void ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet);
	void SendPacket_Character(TCPConnection::pointer connection);
};

extern Packet_CharacterManager packet_CharacterManager;