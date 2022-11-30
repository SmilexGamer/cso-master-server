#pragma once
#include "tcp_connection.h"
#include <thread>
#include <deque>

enum Packet_UMsgID {
	WhisperChat = 0,
	LobbyChat = 1,
	FamilyChat = 3,
	PartyChat = 5,
};

class Packet_UMsgManager {
public:
	Packet_UMsgManager() = default;
	~Packet_UMsgManager();

	void Start();
	void Stop();
	void QueuePacket_UMsg(TCPConnection::Packet::pointer);

private:
	void run();
	void shutdown();
	void parsePacket_UMsg(TCPConnection::Packet::pointer);

private:
	thread _packet_UMsgThread;
	deque<TCPConnection::Packet::pointer> _packet_UMsgQueue{};
	bool _running{ false };
};