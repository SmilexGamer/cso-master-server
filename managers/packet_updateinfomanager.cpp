#include "packet_updateinfomanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_UpdateInfoManager packet_UpdateInfoManager;

void Packet_UpdateInfoManager::ParsePacket_UpdateInfo(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo, but it's not logged in\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_UpdateInfoManager] Parsing Packet_UpdateInfo from client ({})\n", user->GetUserIPAddress());

	unsigned char type = packet->ReadUInt8();
	
	switch (type) {
		case 0: {
			string unk1 = packet->ReadString();

			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}, unk1: {}\n", user->GetUserIPAddress(), type, unk1);
			break;
		}
		case 1: {
			// nothing to read
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent Packet_UpdateInfo - type: {}\n", user->GetUserIPAddress(), type);
			break;
		}
		default: {
			cout << format("[Packet_UpdateInfoManager] Client ({}) has sent unregistered Packet_UpdateInfo type {}!\n", user->GetUserIPAddress(), type);
			break;
		}
	}
}

void Packet_UpdateInfoManager::SendPacket_UpdateInfo(const UserFull& userFull) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, userFull.user->GetConnection(), { PacketID::UpdateInfo });

	packet->WriteUInt32_LE(userFull.user->GetUserID());
	packetManager.BuildUserCharacter(packet, userFull.userCharacter);

	packet->Send();
}