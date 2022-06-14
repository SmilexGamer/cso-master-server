#pragma once
#include "tcp_connection.h"
#include <thread>
#include <deque>

class Packet_LoginManager {
public:
	Packet_LoginManager() = default;
	~Packet_LoginManager();

	void Start();
	void Stop();
	void QueuePacket_Login(TCPConnection::Packet::pointer);

private:
	void run();
	void shutdown();
	void parsePacket_Login(TCPConnection::Packet::pointer);

private:
	std::thread _packet_LoginThread;
	std::deque<TCPConnection::Packet::pointer> _packet_LoginQueue{};
	bool _running{ false };
};