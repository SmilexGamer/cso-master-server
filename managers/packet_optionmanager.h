#pragma once
#include "tcp_connection.h"
#include "user.h"
class Packet_OptionManager {
public:
	void ParsePacket_Option(TCPConnection::Packet::pointer packet);
	void SendPacket_OptionData(const GameUser& gameUser);
};

extern Packet_OptionManager packet_OptionManager;