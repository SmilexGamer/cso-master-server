#include "Packet_LoginManager.h"
#include <iostream>

Packet_LoginManager::~Packet_LoginManager() {
	shutdown();
}

void Packet_LoginManager::Start() {
	if (_packet_LoginThread.joinable()) {
		std::cout << "[Packet_LoginManager] Thread is already running!" << std::endl;
		return;
	}

	std::cout << "[Packet_LoginManager] Starting!" << std::endl;

	_packet_LoginQueue.clear();
	_packet_LoginThread = std::thread(&Packet_LoginManager::run, this);
}

void Packet_LoginManager::Stop() {
	if (!_packet_LoginThread.joinable()) {
		std::cout << "[Packet_LoginManager] Thread is already shut down!" << std::endl;
		return;
	}

	shutdown();
}

void Packet_LoginManager::QueuePacket_Login(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		std::cout << "[Packet_LoginManager] Thread is shut down! Please call Start() again to begin queueing!" << std::endl;
		return;
	}

	std::cout << std::format("[Packet_LoginManager] Queueing Packet_Login from {}", packet->GetConnection()->GetEndPoint()) << std::endl;

	_packet_LoginQueue.push_back(packet);
}

void Packet_LoginManager::run() {
	std::cout << "[Packet_LoginManager] Thread starting!" << std::endl;

	_running = true;
	while (_running) {
		if (_packet_LoginQueue.size() == 0) continue;

		parsePacket_Login(_packet_LoginQueue.front());
		_packet_LoginQueue.pop_front();
	}

	std::cout << "[Packet_LoginManager] Thread shutting down!" << std::endl;
}

void Packet_LoginManager::shutdown() {
	if (_packet_LoginThread.joinable()) {
		std::cout << "[Packet_LoginManager] Shutting down!" << std::endl;

		_running = false;
		_packet_LoginThread.detach();
	}
}

void Packet_LoginManager::parsePacket_Login(TCPConnection::Packet::pointer packet) {
	std::string username = packet->ReadString();
	unsigned short passwordSize = packet->ReadUInt16_LE();
	std::string password = packet->ReadString();
	std::vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned long ipAddress = packet->ReadUInt32_BE();

	std::cout << std::format("[Packet_LoginManager] Client ({}) has sent Packet_Login - username: {}, passwordSize: {}, password: {}, hardwareID:", packet->GetConnection()->GetEndPoint(), username.c_str(), passwordSize, password.c_str());

	for (auto c : hardwareID)
	{
		std::cout << std::format(" {}", c & 0xFF);
	}

	std::cout << std::format(", pcBang: {}, ipAddress: {}.{}.{}.{}", pcBang, (ipAddress >> 24) & 0xFF, (ipAddress >> 16) & 0xFF, (ipAddress >> 8) & 0xFF, ipAddress & 0xFF) << std::endl;
}