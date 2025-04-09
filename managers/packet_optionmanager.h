#pragma once
#include "tcp_connection.h"

#define OPTION_MAX_SIZE 5120

enum Packet_OptionType {
	UserOption = 0,
	Unk1 = 1
};

class Packet_OptionManager {
public:
	void ParsePacket_Option(TCPConnection::Packet::pointer packet);
	void SendPacket_Option_UserOption(TCPConnection::pointer connection, const vector<unsigned char>& userOption);
};

extern Packet_OptionManager packet_OptionManager;