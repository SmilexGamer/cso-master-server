#include "PacketManager.h"
#include <iostream>

PacketManager::~PacketManager() {
	shutdown();
}

void PacketManager::Start() {
	if (_packetThread.joinable()) {
		std::cout << "[PacketManager] Thread is already running!" << std::endl;
		return;
	}

	std::cout << "[PacketManager] Starting!" << std::endl;

	_packetQueue.clear();
	_packetThread = std::thread(&PacketManager::run, this);
}

void PacketManager::Stop() {
	if (!_packetThread.joinable()) {
		std::cout << "[PacketManager] Thread is already shut down!" << std::endl;
		return;
	}

	shutdown();
}

void PacketManager::QueuePacket(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		std::cout << "[PacketManager] Thread is shut down! Please call Start() again to begin queueing!" << std::endl;
		return;
	}

	std::cout << std::format("[PacketManager] Queueing packet from {}", packet->GetConnection()->GetEndPoint()) << std::endl;

	_packetQueue.push_back(packet);
}

void PacketManager::run() {
	std::cout << "[PacketManager] Thread starting!" << std::endl;

	_running = true;
	while (_running) {
		if (_packetQueue.size() == 0) continue;

		parsePacket(_packetQueue.front());
		_packetQueue.pop_front();
	}

	std::cout << "[PacketManager] Thread shutting down!" << std::endl;
}

void PacketManager::shutdown() {
	if (_packetThread.joinable()) {
		std::cout << "[PacketManager] Shutting down!" << std::endl;

		_running = false;
		_packetThread.detach();
	}
}

void PacketManager::parsePacket(TCPConnection::Packet::pointer packet) {
	unsigned char ID = packet->ReadUInt8();

	switch (ID)
	{
	case PacketID::Version:
	{
		packet_VersionManager.QueuePacket_Version(packet);
		break;
	}
	case PacketID::Login:
	{
		packet_LoginManager.QueuePacket_Login(packet);
		break;
	}
	case PacketID::UMsg:
	{
		packet_UMsgManager.QueuePacket_UMsg(packet);
		break;
	}
	default:
	{
		std::cout << std::format("[PacketManager] Client ({}) has sent unregistered packet ID {}!", packet->GetConnection()->GetEndPoint(), ID & 0xFF) << std::endl;
		break;
	}
	}
}