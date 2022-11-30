#include "Packet_LoginManager.h"
#include <iostream>

Packet_LoginManager::~Packet_LoginManager() {
	shutdown();
}

void Packet_LoginManager::Start() {
	if (_packet_LoginThread.joinable()) {
		cout << "[Packet_LoginManager] Thread is already running!\n";
		return;
	}

	cout << "[Packet_LoginManager] Starting!\n";

	_packet_LoginQueue.clear();
	_packet_LoginThread = thread(&Packet_LoginManager::run, this);
}

void Packet_LoginManager::Stop() {
	if (!_packet_LoginThread.joinable()) {
		cout << "[Packet_LoginManager] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

void Packet_LoginManager::QueuePacket_Login(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		cout << "[Packet_LoginManager] Thread is shut down! Please call Start() again to begin queueing!\n";
		return;
	}

	cout << format("[Packet_LoginManager] Queueing Packet_Login from {}\n", packet->GetConnection()->GetEndPoint());

	_packet_LoginQueue.push_back(packet);
}

void Packet_LoginManager::run() {
	cout << "[Packet_LoginManager] Thread starting!\n";

	_running = true;
	while (_running) {
		if (_packet_LoginQueue.size() == 0) {
			Sleep(1);
			continue;
		}

		parsePacket_Login(_packet_LoginQueue.front());
		_packet_LoginQueue.pop_front();
	}

	cout << "[Packet_LoginManager] Thread shutting down!\n";
}

void Packet_LoginManager::shutdown() {
	if (_packet_LoginThread.joinable()) {
		cout << "[Packet_LoginManager] Shutting down!\n";

		_running = false;
		_packet_LoginThread.detach();
	}
}

void Packet_LoginManager::parsePacket_Login(TCPConnection::Packet::pointer packet) {
	string username = packet->ReadString();
	unsigned short passwordSize = packet->ReadUInt16_LE();
	string password = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned long ipAddress = packet->ReadUInt32_BE();

	cout << format("[Packet_LoginManager] Client ({}) has sent Packet_Login - username: {}, passwordSize: {}, password: {}, hardwareID:", packet->GetConnection()->GetEndPoint(), username.c_str(), passwordSize, password.c_str());

	for (auto c : hardwareID)
	{
		cout << format(" {}", c & 0xFF);
	}

	cout << format(", pcBang: {}, ipAddress: {}.{}.{}.{}\n", pcBang, (ipAddress >> 24) & 0xFF, (ipAddress >> 16) & 0xFF, (ipAddress >> 8) & 0xFF, ipAddress & 0xFF);
}