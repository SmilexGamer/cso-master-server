#pragma once
#include "tcp_connection.h"

enum Packet_UMsgType {
	WhisperChat = 0,
	LobbyChat = 1,
	RoomChat = 2,
	FamilyChat = 3,
	PartyChat = 5,
	InfoMessage = 5,
	WarningMessage = 6
};

class Packet_UMsgManager {
public:
	void ParsePacket_UMsg(TCPConnection::Packet::pointer packet);
	void SendPacket_UMsg_ServerMessage(TCPConnection::pointer connection, Packet_UMsgType type, const string& text, const vector<string>& additionalText = {});
};

extern Packet_UMsgManager packet_UMsgManager;