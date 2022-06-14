#include "Packet_UMsgManager.h"
#include <iostream>

Packet_UMsgManager::~Packet_UMsgManager() {
	shutdown();
}

void Packet_UMsgManager::Start() {
	if (_packet_UMsgThread.joinable()) {
		std::cout << "[Packet_UMsgManager] Thread is already running!" << std::endl;
		return;
	}

	std::cout << "[Packet_UMsgManager] Starting!" << std::endl;

	_packet_UMsgQueue.clear();
	_packet_UMsgThread = std::thread(&Packet_UMsgManager::run, this);
}

void Packet_UMsgManager::Stop() {
	if (!_packet_UMsgThread.joinable()) {
		std::cout << "[Packet_UMsgManager] Thread is already shut down!" << std::endl;
		return;
	}

	shutdown();
}

void Packet_UMsgManager::QueuePacket_UMsg(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		std::cout << "[Packet_UMsgManager] Thread is shut down! Please call Start() again to begin queueing!" << std::endl;
		return;
	}

	std::cout << std::format("[Packet_UMsgManager] Queueing Packet_UMsg from {}", packet->GetConnection()->GetEndPoint()) << std::endl;

	_packet_UMsgQueue.push_back(packet);
}

void Packet_UMsgManager::run() {
	std::cout << "[Packet_UMsgManager] Thread starting!" << std::endl;

	_running = true;
	while (_running) {
		if (_packet_UMsgQueue.size() == 0) continue;

		parsePacket_UMsg(_packet_UMsgQueue.front());
		_packet_UMsgQueue.pop_front();
	}

	std::cout << "[Packet_UMsgManager] Thread shutting down!" << std::endl;
}

void Packet_UMsgManager::shutdown() {
	if (_packet_UMsgThread.joinable()) {
		std::cout << "[Packet_UMsgManager] Shutting down!" << std::endl;

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
		std::cout << std::format("[Packet_UMsgManager] Client ({}) has sent unregistered Packet_UMsg subID {}!", packet->GetConnection()->GetEndPoint(), subID & 0xFF) << std::endl;
		break;
	}
	}
}