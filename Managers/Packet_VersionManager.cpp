#include "Packet_VersionManager.h"
#include <iostream>

Packet_VersionManager::~Packet_VersionManager() {
	shutdown();
}

void Packet_VersionManager::Start() {
	if (_packet_VersionThread.joinable()) {
		std::cout << "[Packet_VersionManager] Thread is already running!" << std::endl;
		return;
	}

	std::cout << "[Packet_VersionManager] Starting!" << std::endl;

	_packet_VersionQueue.clear();
	_packet_VersionThread = std::thread(&Packet_VersionManager::run, this);
}

void Packet_VersionManager::Stop() {
	if (!_packet_VersionThread.joinable()) {
		std::cout << "[Packet_VersionManager] Thread is already shut down!" << std::endl;
		return;
	}

	shutdown();
}

void Packet_VersionManager::QueuePacket_Version(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		std::cout << "[Packet_VersionManager] Thread is shut down! Please call Start() again to begin queueing!" << std::endl;
		return;
	}

	std::cout << std::format("[Packet_VersionManager] Queueing Packet_Version from {}", packet->GetConnection()->GetEndPoint()) << std::endl;

	_packet_VersionQueue.push_back(packet);
}

void Packet_VersionManager::run() {
	std::cout << "[Packet_VersionManager] Thread starting!" << std::endl;

	_running = true;
	while (_running) {
		if (_packet_VersionQueue.size() == 0) continue;

		parsePacket_Version(_packet_VersionQueue.front());
		_packet_VersionQueue.pop_front();
	}

	std::cout << "[Packet_VersionManager] Thread shutting down!" << std::endl;
}

void Packet_VersionManager::shutdown() {
	if (_packet_VersionThread.joinable()) {
		std::cout << "[Packet_VersionManager] Shutting down!" << std::endl;

		_running = false;
		_packet_VersionThread.detach();
	}
}

void Packet_VersionManager::parsePacket_Version(TCPConnection::Packet::pointer packet) {
	unsigned char launcherVersion = packet->ReadUInt8();
	unsigned short clientVersion = packet->ReadUInt16_LE();
	unsigned long clientCRC = packet->ReadUInt32_LE();
	unsigned long clientNARCRC = packet->ReadUInt32_LE();

	std::cout << std::format("[Packet_VersionManager] Client ({}) has sent Packet_Version - launcherVersion: {}, clientVersion: {}, clientCRC: {}, clientNARCRC: {}", packet->GetConnection()->GetEndPoint(), launcherVersion & 0xFF, clientVersion, clientCRC, clientNARCRC) << std::endl;

	sendPacket_Version(packet->GetConnection());
}

void Packet_VersionManager::sendPacket_Version(TCPConnection::pointer connection) {
	if (!_running) {
		std::cout << "[Packet_VersionManager] Thread is shut down! Please call Start() again to send Packet_Version!" << std::endl;
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { 0 });
	packet->Send();
}