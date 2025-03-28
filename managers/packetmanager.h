#pragma once
#include "user.h"

class PacketManager {
public:
	PacketManager();
	~PacketManager();

	void Start();
	void Stop();
	void QueuePacket(TCPConnection::Packet::pointer packet);
	void SendPacket_Reply(TCPConnection::pointer connection, Packet_ReplyType type, const vector<string>& additionalText = {});
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