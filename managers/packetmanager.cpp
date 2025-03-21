#include "packetmanager.h"
#include "packet_versionmanager.h"
#include "packet_charactermanager.h"
#include "packet_loginmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_udpmanager.h"
#include "packet_shopmanager.h"
#include "packet_userstartmanager.h"
#include "packet_transfermanager.h"
#include "serverconfig.h"
#include <iostream>

PacketManager packetManager;

PacketManager::PacketManager() {
	_running = false;
}

PacketManager::~PacketManager() {
	shutdown();
}

void PacketManager::Start() {
	if (_packetThread.joinable()) {
		cout << "[PacketManager] Thread is already running!\n";
		return;
	}

	cout << "[PacketManager] Starting!\n";

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

void PacketManager::BuildUserCharacter(TCPConnection::Packet::pointer packet, const UserCharacter& userCharacter) {
	packet->WriteUInt16_LE(userCharacter.flag);

	if (userCharacter.flag & UserCharacterFlag::Unk1) {
		packet->WriteUInt8(userCharacter.unk1);
	}
	if (userCharacter.flag & UserCharacterFlag::NickName) {
		packet->WriteString(userCharacter.nickName);
	}
	if (userCharacter.flag & UserCharacterFlag::Unk4) {
		packet->WriteString(userCharacter.unk4_1);
		packet->WriteUInt8(userCharacter.unk4_2);
		packet->WriteUInt8(userCharacter.unk4_3);
		packet->WriteUInt8(userCharacter.unk4_4);
	}
	if (userCharacter.flag & UserCharacterFlag::Level) {
		packet->WriteUInt8(userCharacter.level);
	}
	if (userCharacter.flag & UserCharacterFlag::Unk10) {
		packet->WriteUInt8(userCharacter.unk10);
	}
	if (userCharacter.flag & UserCharacterFlag::Exp) {
		packet->WriteUInt64_LE(userCharacter.exp);
	}
	if (userCharacter.flag & UserCharacterFlag::Cash) {
		packet->WriteUInt64_LE(userCharacter.cash);
	}
	if (userCharacter.flag & UserCharacterFlag::Points) {
		packet->WriteUInt64_LE(userCharacter.points);
	}
	if (userCharacter.flag & UserCharacterFlag::BattleStats) {
		packet->WriteUInt32_LE(userCharacter.battles);
		packet->WriteUInt32_LE(userCharacter.wins);
		packet->WriteUInt32_LE(userCharacter.frags);
		packet->WriteUInt32_LE(userCharacter.deaths);
	}
	if (userCharacter.flag & UserCharacterFlag::Location) {
		packet->WriteString(""); // pcBang
		packet->WriteUInt16_LE(userCharacter.city); // province/city (도시) index
		packet->WriteUInt16_LE(userCharacter.county); // district/county (구군) index
		packet->WriteUInt16_LE(userCharacter.neighborhood); // neighborhood (동) index
		packet->WriteString(userCharacter.unk200_5);
	}
	if (userCharacter.flag & UserCharacterFlag::Unk400) {
		packet->WriteUInt32_LE(userCharacter.unk400);
	}
	if (userCharacter.flag & UserCharacterFlag::Unk800) {
		packet->WriteUInt8(userCharacter.unk800);
	}
	if (userCharacter.flag & UserCharacterFlag::Unk1000) {
		packet->WriteUInt32_LE(userCharacter.unk1000_1);
		packet->WriteUInt32_LE(userCharacter.unk1000_2);
		packet->WriteString(userCharacter.unk1000_3);
		packet->WriteUInt8(userCharacter.unk1000_4);
		packet->WriteUInt8(userCharacter.unk1000_5);
		packet->WriteUInt8(userCharacter.unk1000_6);
	}
}

int PacketManager::run() {
	cout << "[PacketManager] Thread starting!\n";

	try {
		_running = true;
		while (_running) {
			if (_packetQueue.empty()) {
				this_thread::yield();
				continue;
			}

			parsePacket(_packetQueue.front());
			_packetQueue.pop_front();
		}
	}
	catch (exception& e) {
		cerr << format("[PacketManager] Error on run: {}\n", e.what());
		return -1;
	}

	cout << "[PacketManager] Thread shutting down!\n";
	return 0;
}

int PacketManager::shutdown() {
	try {
		if (_packetThread.joinable()) {
			cout << "[PacketManager] Shutting down!\n";

			_running = false;
			_packetQueue.clear();
			_packetThread.detach();
		}
	}
	catch (exception& e) {
		cerr << format("[PacketManager] Error on shutdown: {}\n", e.what());
		return -1;
	}

	return 0;
}

void PacketManager::parsePacket(TCPConnection::Packet::pointer packet) {
	unsigned char ID = packet->ReadUInt8();

	switch (ID) {
		case PacketID::Version: {
			packet_VersionManager.ParsePacket_Version(packet);
			break;
		}
		case PacketID::RecvCharacter: {
			packet_CharacterManager.ParsePacket_RecvCharacter(packet);
			break;
		}
		case PacketID::Login: {
			packet_LoginManager.ParsePacket_Login(packet);
			break;
		}
		case PacketID::TransferLogin: {
			packet_TransferManager.ParsePacket_TransferLogin(packet);
			break;
		}
		case PacketID::RequestTransfer: {
			packet_TransferManager.ParsePacket_RequestTransfer(packet);
			break;
		}
		case PacketID::RequestServerList: {
			packet_ServerListManager.ParsePacket_RequestServerList(packet);
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
		case PacketID::Udp: {
			packet_UdpManager.ParsePacket_Udp(packet);
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
			cout << format("[PacketManager] Client ({}) has sent unregistered packet ID {}!\n", packet->GetConnection()->GetEndPoint(), ID);
			break;
		}
	}
}