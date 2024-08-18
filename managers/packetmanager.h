#pragma once
#include "tcp_connection.h"
#include <thread>
#include <deque>

class PacketManager {
public:
	~PacketManager();

	void Start();
	void Stop();
	void QueuePacket(TCPConnection::Packet::pointer);
	void SendPacket_Reply(TCPConnection::pointer, unsigned char type, vector<string> additionalText = {});

private:
	void run();
	void shutdown();
	void parsePacket(TCPConnection::Packet::pointer);

private:
	thread _packetThread;
	deque<TCPConnection::Packet::pointer> _packetQueue{};
	bool _running{ false };
};

extern PacketManager packetManager;