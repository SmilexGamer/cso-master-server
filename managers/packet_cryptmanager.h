#pragma once
#include "tcp_connection.h"

enum CipherType {
	Encrypt = 0,
	Decrypt = 1
};

class Packet_CryptManager {
public:
	void ParsePacket_RecvCrypt(TCPConnection::Packet::pointer packet);
	void SendPacket_Crypt(TCPConnection::pointer connection, CipherType type, Cipher cipher);
};

extern Packet_CryptManager packet_CryptManager;