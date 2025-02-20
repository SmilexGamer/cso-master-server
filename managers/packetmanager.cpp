#include "packetmanager.h"
#include "packet_versionmanager.h"
#include "packet_loginmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_shopmanager.h"
#include "packet_userstartmanager.h"
#include <iostream>

PacketManager packetManager;

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

	cout << format("[PacketManager] Queueing packet from client ({})\n", packet->GetConnection()->GetEndPoint());

	_packetQueue.push_back(packet);
}

void PacketManager::SendPacket_Reply(TCPConnection::pointer connection, unsigned char type, vector<string> additionalText) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Reply });

	packet->WriteUInt8(type);
	packet->WriteString("");
	packet->WriteUInt8((unsigned char)additionalText.size());
	for (auto& text : additionalText) {
		packet->WriteString(text);
	}

	packet->Send();
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
		case PacketID::ServerList: {
			packet_ServerListManager.ParsePacket_ServerList(packet);
			break;
		}
		case PacketID::RequestTransfer: {
			packet_ServerListManager.ParsePacket_RequestTransfer(packet);
			break;
		}
		case PacketID::RequestServerList: {
			packet_ServerListManager.SendPacket_ServerList(packet->GetConnection());
			break;
		}
		case PacketID::Room: {
			packet_RoomManager.ParsePacket_Room(packet);
			break;
		}
		case PacketID::UMsg: {
			packet_UMsgManager.ParsePacket_UMsg(packet);
			break;
		}
		case PacketID::Host: {
			packet_HostManager.ParsePacket_Host(packet);
			break;
		}
		case PacketID::UpdateInfo: {
			packet_UpdateInfoManager.ParsePacket_UpdateInfo(packet);
			break;
		}
		case PacketID::Shop: {
			packet_ShopManager.ParsePacket_Shop(packet);
			break;
		}
		case PacketID::UserStart: {
			packet_UserStartManager.ParsePacket_UserStart(packet);
			break;
		}
		default: {
			cout << format("[PacketManager] Client ({}) has sent unregistered packet ID {}!\n", packet->GetConnection()->GetEndPoint(), ID & 0xFF);
			break;
		}
	}
}