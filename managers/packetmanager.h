#pragma once
#include "tcp_connection.h"
#include "user.h"
#include <thread>
#include <deque>

class PacketManager {
public:
	PacketManager();
	~PacketManager();

	void Start();
	void Stop();
	void QueuePacket(TCPConnection::Packet::pointer packet);
	void SendPacket_Reply(TCPConnection::pointer connection, unsigned char type, const vector<string> additionalText = {});
	void BuildUserCharacter(TCPConnection::Packet::pointer packet, const UserCharacter& userCharacter);

private:
	int run();
	int shutdown();
	void parsePacket(TCPConnection::Packet::pointer packet);

private:
	thread _packetThread;
	deque<TCPConnection::Packet::pointer> _packetQueue {};
	bool _running;
};

extern PacketManager packetManager;