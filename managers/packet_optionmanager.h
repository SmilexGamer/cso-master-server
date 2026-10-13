#pragma once
#include "user.h"

constexpr auto USERCONFIG_MAX_SIZE = 5120;

enum Packet_OptionType {
	UserConfig = 0,
	UserQuickStart = 1
};

class Packet_OptionManager {
public:
	void ParsePacket_Option(TCPConnection::Packet::pointer packet);
	void SendPacket_Option_UserConfig(TCPConnection::pointer connection, const vector<unsigned char>& userConfig);
	void SendPacket_Option_UserQuickStart(TCPConnection::pointer connection, unsigned long userQuickStart);

private:
	void parsePacket_Option_UserConfig(User* user, TCPConnection::Packet::pointer packet);
	void parsePacket_Option_UserQuickStart(User* user, TCPConnection::Packet::pointer packet);
};

extern Packet_OptionManager packet_OptionManager;