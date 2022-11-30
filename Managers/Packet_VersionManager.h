#pragma once
#include "tcp_connection.h"
#include <thread>
#include <deque>

class Packet_VersionManager {
public:
	Packet_VersionManager() = default;
	~Packet_VersionManager();

	void Start();
	void Stop();
	void QueuePacket_Version(TCPConnection::Packet::pointer);

private:
	void run();
	void shutdown();
	void parsePacket_Version(TCPConnection::Packet::pointer);
	void sendPacket_Version(TCPConnection::pointer);

private:
	thread _packet_VersionThread;
	deque<TCPConnection::Packet::pointer> _packet_VersionQueue{};
	bool _running{ false };
};