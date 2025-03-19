#include "packet_updateinfomanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo, but it's not logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_UpdateInfoManager] Parsing Packet_UpdateInfo from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			string unk1 = packet->ReadString();

			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", packet->GetConnection()->GetEndPoint(), type, unk1.c_str());
			break;
		}
		case 1: {
			// nothing to read
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
		default: {
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent unregistered Packet_UpdateInfo type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}

void Packet_UpdateInfoManager::SendPacket_UpdateInfo(TCPConnection::pointer connection, unsigned long userID, UserCharacter& userCharacter) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::UpdateInfo });

	packet->WriteUInt32_LE(userID); // userID
	packetManager.BuildUserInfo(packet, userCharacter);

	packet->Send();
}