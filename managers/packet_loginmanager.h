#pragma once
#include "tcp_connection.h"

class Packet_LoginManager {
public:
	void ParsePacket_Login(TCPConnection::Packet::pointer packet);
	void sendPacket_Character_Reply_CreateNicknameDlg(TCPConnection::pointer connection);
};

extern Packet_LoginManager packet_LoginManager;