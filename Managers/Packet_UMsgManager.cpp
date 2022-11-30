#include "Packet_UMsgManager.h"
#include <iostream>

Packet_UMsgManager::~Packet_UMsgManager() {
	shutdown();
}

void Packet_UMsgManager::Start() {
	if (_packet_UMsgThread.joinable()) {
		cout << "[Packet_UMsgManager] Thread is already running!\n";
		return;
	}

	cout << "[Packet_UMsgManager] Starting!\n";

	_packet_UMsgQueue.clear();
	_packet_UMsgThread = thread(&Packet_UMsgManager::run, this);
}

void Packet_UMsgManager::Stop() {
	if (!_packet_UMsgThread.joinable()) {
		cout << "[Packet_UMsgManager] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

void Packet_UMsgManager::QueuePacket_UMsg(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		cout << "[Packet_UMsgManager] Thread is shut down! Please call Start() again to begin queueing!\n";
		return;
	}

	cout << format("[Packet_UMsgManager] Queueing Packet_UMsg from {}\n", packet->GetConnection()->GetEndPoint());

	_packet_UMsgQueue.push_back(packet);
}

void Packet_UMsgManager::run() {
	cout << "[Packet_UMsgManager] Thread starting!\n";

	_running = true;
	while (_running) {
		if (_packet_UMsgQueue.size() == 0) {
			Sleep(1);
			continue;
		}

		parsePacket_UMsg(_packet_UMsgQueue.front());
		_packet_UMsgQueue.pop_front();
	}

	cout << "[Packet_UMsgManager] Thread shutting down!\n";
}

void Packet_UMsgManager::shutdown() {
	if (_packet_UMsgThread.joinable()) {
		cout << "[Packet_UMsgManager] Shutting down!\n";

		_running = false;
		_packet_UMsgThread.detach();
	}
}

void Packet_UMsgManager::parsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	unsigned char subID = packet->ReadUInt8();

	switch (subID)
	{
	default:
	{
		cout << format("[Packet_UMsgManager] Client ({}) has sent unregistered Packet_UMsg subID {}!\n", packet->GetConnection()->GetEndPoint(), subID & 0xFF);
		break;
	}
	}
}