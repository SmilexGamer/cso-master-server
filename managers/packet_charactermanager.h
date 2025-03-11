#pragma once
#include "tcp_connection.h"

class Packet_CharacterManager {
public:
	void ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet);
	void SendPacket_Character(TCPConnection::pointer connection);
};

extern Packet_CharacterManager packet_CharacterManager;