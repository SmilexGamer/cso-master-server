#pragma once
#include "user.h"

enum Packet_UMsgType {
	WhisperChat = 0,
	ChannelChat = 1,
	RoomChat = 2,
	ClanChat = 3,
	NoticeMessage = 4,
	InfoMessage = 5,
	WarningMessage = 6
};

class Packet_UMsgManager {
public:
	void ParsePacket_UMsg(TCPConnection::Packet::pointer packet);
	void SendPacket_UMsg_NoticeMessage(TCPConnection::pointer connection, const string& text);
	void SendPacket_UMsg_ServerMessage(TCPConnection::pointer connection, Packet_UMsgType type, const string& text, const vector<string>& additionalText = {});

private:
	void parsePacket_UMsg_WhisperChat(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_UMsg_ChannelChat(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_UMsg_RoomChat(User* user, TCPConnection::Packet::pointer packet);
	void sendPacket_UMsg_Chat(TCPConnection::pointer connection, Packet_UMsgType type, const string& senderNickName, const string& text);
};

extern Packet_UMsgManager packet_UMsgManager;