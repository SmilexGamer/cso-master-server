#include "Packet_VersionManager.h"
#include <iostream>

Packet_VersionManager::~Packet_VersionManager() {
	shutdown();
}

void Packet_VersionManager::Start() {
	if (_packet_VersionThread.joinable()) {
		cout << "[Packet_VersionManager] Thread is already running!\n";
		return;
	}

	cout << "[Packet_VersionManager] Starting!\n";

	_packet_VersionQueue.clear();
	_packet_VersionThread = thread(&Packet_VersionManager::run, this);
}

void Packet_VersionManager::Stop() {
	if (!_packet_VersionThread.joinable()) {
		cout << "[Packet_VersionManager] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

void Packet_VersionManager::QueuePacket_Version(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		cout << "[Packet_VersionManager] Thread is shut down! Please call Start() again to begin queueing!\n";
		return;
	}

	cout << format("[Packet_VersionManager] Queueing Packet_Version from {}\n", packet->GetConnection()->GetEndPoint());

	_packet_VersionQueue.push_back(packet);
}

void Packet_VersionManager::run() {
	cout << "[Packet_VersionManager] Thread starting!\n";

	_running = true;
	while (_running) {
		if (_packet_VersionQueue.size() == 0) {
			Sleep(1);
			continue;
		}

		parsePacket_Version(_packet_VersionQueue.front());
		_packet_VersionQueue.pop_front();
	}

	cout << "[Packet_VersionManager] Thread shutting down!\n";
}

void Packet_VersionManager::shutdown() {
	if (_packet_VersionThread.joinable()) {
		cout << "[Packet_VersionManager] Shutting down!\n";

		_running = false;
		_packet_VersionThread.detach();
	}
}

void Packet_VersionManager::parsePacket_Version(TCPConnection::Packet::pointer packet) {
	unsigned char launcherVersion = packet->ReadUInt8();
	unsigned short clientVersion = packet->ReadUInt16_LE();
	unsigned long clientCRC = packet->ReadUInt32_LE();
	unsigned long clientNARCRC = packet->ReadUInt32_LE();

	cout << format("[Packet_VersionManager] Client ({}) has sent Packet_Version - launcherVersion: {}, clientVersion: {}, clientCRC: {}, clientNARCRC: {}\n", packet->GetConnection()->GetEndPoint(), launcherVersion & 0xFF, clientVersion, clientCRC, clientNARCRC);

	sendPacket_Version(packet->GetConnection());
}

void Packet_VersionManager::sendPacket_Version(TCPConnection::pointer connection) {
	if (!_running) {
		cout << "[Packet_VersionManager] Thread is shut down! Please call Start() again to send Packet_Version!\n";
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { 0 });
	packet->Send();
}