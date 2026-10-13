#pragma once
#include "user.h"

enum Packet_SearchRoomType {
	SearchRoomReply = 0,
	GameInvite = 1,
	ClanMemberSummon = 2
};

class Packet_SearchRoomManager {
public:
	void ParsePacket_SearchRoom(TCPConnection::Packet::pointer packet);

private:
	void sendPacket_SearchRoom_SearchRoomReply(TCPConnection::pointer connection, unsigned char serverID, unsigned char channelID, unsigned short roomID);
};

extern Packet_SearchRoomManager packet_SearchRoomManager;