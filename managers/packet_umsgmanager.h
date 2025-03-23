#pragma once
#include "tcp_connection.h"

class Packet_UMsgManager {
public:
	void ParsePacket_UMsg(TCPConnection::Packet::pointer packet);
	void SendPacket_UMsg_ServerMessage(TCPConnection::pointer connection, Packet_UMsgType type, const string& text, vector<string> additionalText = {});
};

extern Packet_UMsgManager packet_UMsgManager;