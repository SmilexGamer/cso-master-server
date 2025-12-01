#pragma once
#include "tcp_connection.h"

#define BOOKMARK_NAME_MAX_SIZE 12

enum Packet_FavoriteType {
	UserBuyMenu = 0,
	UserBookMark = 1
};

class Packet_FavoriteManager {
public:
	void ParsePacket_Favorite(TCPConnection::Packet::pointer packet);
	void SendPacket_Favorite_UserBuyMenu(TCPConnection::pointer connection, const vector<BuyMenu>& userBuyMenus);
	void SendPacket_Favorite_UserBookMark(TCPConnection::pointer connection, const vector<BookMark>& userBookMarks);
};

extern Packet_FavoriteManager packet_FavoriteManager;