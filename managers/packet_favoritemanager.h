#pragma once
#include "user.h"

constexpr auto BOOKMARK_NAME_MAX_SIZE = 12;
constexpr auto BOOKMARK_FLASHBANG_MAX = 2;
constexpr auto BOOKMARK_KEVLAR_MAX = 2;

enum Packet_FavoriteType {
	UserBuyMenu = 0,
	UserBookMark = 1
};

class Packet_FavoriteManager {
public:
	void ParsePacket_Favorite(TCPConnection::Packet::pointer packet);
	void SendPacket_Favorite_UserBuyMenu(TCPConnection::pointer connection, const vector<BuyMenu>& userBuyMenus);
	void SendPacket_Favorite_UserBookMark(TCPConnection::pointer connection, const vector<BookMark>& userBookMarks);

private:
	void parsePacket_Favorite_UserBuyMenu(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Favorite_UserBookMark(User* user, TCPConnection::Packet::pointer packet);
};

extern Packet_FavoriteManager packet_FavoriteManager;