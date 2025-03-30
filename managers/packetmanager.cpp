#include "packetmanager.h"
#include "packet_versionmanager.h"
#include "packet_transfermanager.h"
#include "packet_loginmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_charactermanager.h"
#include "packet_cryptmanager.h"
#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packet_hostmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_udpmanager.h"
#include "packet_shopmanager.h"
#include "packet_userstartmanager.h"
#include "serverconsole.h"

PacketManager packetManager;

PacketManager::PacketManager() {
	_running = false;
}

PacketManager::~PacketManager() {
	shutdown();
}

void PacketManager::Start() {
	if (_packetThread.joinable()) {
		serverConsole.Print(PrintType::Warn, "[ PacketManager ] Thread is already running!\n");
		return;
	}

	serverConsole.Print(PrintType::Info, "[ PacketManager ] Starting!\n");

	_packetThread = thread(&PacketManager::run, this);
}

void PacketManager::Stop() {
	if (!_packetThread.joinable()) {
		serverConsole.Print(PrintType::Warn, "[ PacketManager ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

void PacketManager::QueuePacket(TCPConnection::Packet::pointer packet) {
	if (!_running) {
		serverConsole.Print(PrintType::Warn, "[ PacketManager ] Thread is shut down! Please call Start() again to begin queueing!\n");
		return;
	}

	serverConsole.Print(PrintType::Info, format("[ PacketManager ] Queueing packet from client ({})\n", packet->GetConnection()->GetIPAddress()));

	_packetQueue.push_back(packet);
}

void PacketManager::SendPacket_Reply(TCPConnection::pointer connection, Packet_ReplyType type, const vector<string>& additionalText) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Reply });

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

	if (userCharacter.flag & USERCHARACTER_FLAG_UNK1) {
		packet->WriteUInt8(userCharacter.unk1);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_NICKNAME) {
		packet->WriteString(userCharacter.nickName);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK4) {
		packet->WriteString(userCharacter.unk4_1);
		packet->WriteUInt8(userCharacter.unk4_2);
		packet->WriteUInt8(userCharacter.unk4_3);
		packet->WriteUInt8(userCharacter.unk4_4);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_LEVEL) {
		packet->WriteUInt8(userCharacter.level);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK10) {
		packet->WriteUInt8(userCharacter.unk10);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_EXP) {
		packet->WriteUInt64_LE(userCharacter.exp);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_CASH) {
		packet->WriteUInt64_LE(userCharacter.cash);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_POINTS) {
		packet->WriteUInt64_LE(userCharacter.points);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_BATTLESTATS) {
		packet->WriteUInt32_LE(userCharacter.battles);
		packet->WriteUInt32_LE(userCharacter.wins);
		packet->WriteUInt32_LE(userCharacter.frags);
		packet->WriteUInt32_LE(userCharacter.deaths);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_LOCATION) {
		packet->WriteString(""); // pcBang
		packet->WriteUInt16_LE(userCharacter.city); // province/city (도시) index
		packet->WriteUInt16_LE(userCharacter.county); // district/county (구군) index
		packet->WriteUInt16_LE(userCharacter.neighborhood); // neighborhood (동) index
		packet->WriteString(userCharacter.unk200_5);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK400) {
		packet->WriteUInt32_LE(userCharacter.unk400);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK800) {
		packet->WriteUInt8(userCharacter.unk800);
	}
	if (userCharacter.flag & USERCHARACTER_FLAG_UNK1000) {
		packet->WriteUInt32_LE(userCharacter.unk1000_1);
		packet->WriteUInt32_LE(userCharacter.unk1000_2);
		packet->WriteString(userCharacter.unk1000_3);
		packet->WriteUInt8(userCharacter.unk1000_4);
		packet->WriteUInt8(userCharacter.unk1000_5);
		packet->WriteUInt8(userCharacter.unk1000_6);
	}
}

int PacketManager::run() {
	serverConsole.Print(PrintType::Info, "[ PacketManager ] Thread starting!\n");

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
		serverConsole.Print(PrintType::Error, format("[ PacketManager ] Error on run: {}\n", e.what()));
		return -1;
	}

	serverConsole.Print(PrintType::Info, "[ PacketManager ] Thread shutting down!\n");
	return 0;
}

int PacketManager::shutdown() {
	try {
		if (_packetThread.joinable()) {
			serverConsole.Print(PrintType::Info, "[ PacketManager ] Shutting down!\n");

			_running = false;
			_packetQueue.clear();
			_packetThread.detach();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrintType::Error, format("[ PacketManager ] Error on shutdown: {}\n", e.what()));
		return -1;
	}

	return 0;
}

void PacketManager::parsePacket(TCPConnection::Packet::pointer packet) {
	unsigned char packetID = packet->ReadUInt8();

	switch ((PacketID)packetID) {
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
		case PacketID::RecvCrypt: {
			packet_CryptManager.ParsePacket_RecvCrypt(packet);
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
			serverConsole.Print(PrintType::Warn, format("[ PacketManager ] Client ({}) has sent unregistered packet ID {}!\n", packet->GetConnection()->GetIPAddress(), packetID));
			break;
		}
	}
}