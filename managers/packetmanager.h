#pragma once
#include "tcp_connection.h"
#include <thread>
#include <deque>

class PacketManager {
public:
	~PacketManager();

	void Start();
	void Stop();
	void QueuePacket(TCPConnection::Packet::pointer packet);
	void SendPacket_Reply(TCPConnection::pointer connection, unsigned char type, vector<string> additionalText = {});

private:
	void run();
	void shutdown();
	void parsePacket(TCPConnection::Packet::pointer packet);

private:
	thread _packetThread;
	deque<TCPConnection::Packet::pointer> _packetQueue{};
	bool _running{ false };
};

extern PacketManager packetManager;