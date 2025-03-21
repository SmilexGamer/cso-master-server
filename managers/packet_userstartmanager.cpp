#include "packet_userstartmanager.h"
#include <iostream>

Packet_UserStartManager packet_UserStartManager;

void Packet_UserStartManager::ParsePacket_UserStart(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UserStartManager] Parsing Packet_UserStart from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();
	unsigned long unk1 = packet->ReadUInt32_LE();
	string unk2 = packet->ReadString();

	cout << format("[Packet_LoginManager] Client ({}) has sent Packet_UserStart - type: {}, unk1: {}, unk2: {}\n", packet->GetConnection()->GetEndPoint(), type, unk1, unk2.c_str());
}

void Packet_UserStartManager::SendPacket_UserStart(User* user, const UserCharacter& userCharacter) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, user->GetConnection(), { PacketID::UserStart });

	packet->WriteUInt32_LE(user->GetUserID());
	packet->WriteString(user->GetUserName());
	packet->WriteString(userCharacter.nickName);

	packet->Send();
}