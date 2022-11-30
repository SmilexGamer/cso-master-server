#pragma once
#include "tcp_connection.h"
#include "Packet_VersionManager.h"
#include "Packet_LoginManager.h"
#include "Packet_UMsgManager.h"
#include <thread>
#include <deque>

enum PacketID {
	Version = 0,
	Reply = 1,
	Transfer = 2,
	Login = 3,
	ServerList = 5,
	Character = 6,
	Crypt = 7,
	Room = 65,
	ClientCheck = 66,
	UMsg = 67,
	Host = 68,
	UpdateInfo = 69,
	Udp = 70,
	Clan = 71,
	Shop = 72,
	Rank = 73,
	Ban = 74,
	Option = 76,
	Favorite = 77,
	Item = 78,
	Unk79 = 79,
	SearchRoom = 80,
	HostServer = 81, // UpdateInfo = 81,
	HackShield = 82,
	Report = 83,
	Title = 84,
	Buff = 85,
	QuickStart = 86,
	UserSurvey = 87,
	Quest = 88,
	MiniGame = 89,
	Hack = 90,
	Metadata = 91,
	SNS = 92,
	Comrade = 94,
	RankRelatedUnk = 95,
	Gift_Item = 96,
	_2nd_Password = 97,
	Request2nd_Password = 98,
	Match = 99,
	ZBEnhance = 100,
	CleanSystem = 101,
	UserStart = 150,
	RoomList = 151,
	DefaultItems = 152,
	Lobby = 153,
	Inventory = 154,
	ClanStock = 155,
	CafeItems = 156,
	UserUpdateInfo = 157,
	FabItems = 158,
	Event = 159,
	CostumeInven = 160
};

class PacketManager {
public:
	PacketManager() = default;
	~PacketManager();

	void Start();
	void Stop();
	void QueuePacket(TCPConnection::Packet::pointer);

private:
	void run();
	void shutdown();
	void parsePacket(TCPConnection::Packet::pointer);

public:
	Packet_VersionManager packet_VersionManager;
	Packet_LoginManager packet_LoginManager;
	Packet_UMsgManager packet_UMsgManager;

private:
	thread _packetThread;
	deque<TCPConnection::Packet::pointer> _packetQueue{};
	bool _running{ false };
};