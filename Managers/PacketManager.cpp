#include "PacketManager.h"
#include "packet_versionmanager.h"
#include "packet_loginmanager.h"
#include "packet_umsgmanager.h"
#include <iostream>

Packet_VersionManager packet_VersionManager;
Packet_LoginManager packet_LoginManager;
Packet_UMsgManager packet_UMsgManager;

PacketManager::~PacketManager() {
	shutdown();
}

void PacketManager::Start() {
	if (_packetThread.joinable()) {
		cout << "[PacketManager] Thread is already running!\n";
		return;
	}

	cout << "[PacketManager] Starting!\n";

	_packetQueue.clear();
	_packetThread = thread(&PacketManager::run, this);
}

void PacketManager::Stop() {
	if (!_packetThread.joinable()) {
		cout << "[PacketManager] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

void PacketManager::QueuePacket(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		cout << "[PacketManager] Thread is shut down! Please call Start() again to begin queueing!\n";
		return;
	}

	cout << format("[PacketManager] Queueing packet from {}\n", packet->GetConnection()->GetEndPoint());

	_packetQueue.push_back(packet);
}

void PacketManager::run() {
	cout << "[PacketManager] Thread starting!\n";

	_running = true;
	while (_running) {
		if (_packetQueue.size() == 0) {
			Sleep(1);
			continue;
		}

		parsePacket(_packetQueue.front());
		_packetQueue.pop_front();
	}

	cout << "[PacketManager] Thread shutting down!\n";
}

void PacketManager::shutdown() {
	if (_packetThread.joinable()) {
		cout << "[PacketManager] Shutting down!\n";

		_running = false;
		_packetThread.detach();
	}
}

void PacketManager::parsePacket(TCPConnection::Packet::pointer packet) {
	unsigned char ID = packet->ReadUInt8();

	switch (ID) {
		case PacketID::Version: {
			packet_VersionManager.ParsePacket_Version(packet);
			break;
		}
		case PacketID::Login: {
			packet_LoginManager.ParsePacket_Login(packet);
			break;
		}
		case PacketID::UMsg: {
			packet_UMsgManager.ParsePacket_UMsg(packet);
			break;
		}
		default: {
			cout << format("[PacketManager] Client ({}) has sent unregistered packet ID {}!\n", packet->GetConnection()->GetEndPoint(), ID & 0xFF);
			break;
		}
	}
}