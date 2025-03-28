#include "packet_userstartmanager.h"
#include <iostream>

Packet_UserStartManager packet_UserStartManager;

void Packet_UserStartManager::ParsePacket_UserStart(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UserStartManager] Parsing Packet_UserStart from client ({})\n", packet->GetConnection()->GetIPAddress());

	unsigned char type = packet->ReadUInt8();
	unsigned long unk1 = packet->ReadUInt32_LE();
	string unk2 = packet->ReadString();

	cout << format("[Packet_LoginManager] Client ({}) has sent Packet_UserStart - type: {}, unk1: {}, unk2: {}\n", packet->GetConnection()->GetIPAddress(), type, unk1, unk2);
}

void Packet_UserStartManager::SendPacket_UserStart(const UserFull& userFull) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, userFull.user->GetConnection(), { (unsigned char)PacketID::UserStart });

	packet->WriteUInt32_LE(userFull.user->GetUserID());
	packet->WriteString(userFull.user->GetUserName());
	packet->WriteString(userFull.userCharacter.nickName);

	packet->Send();
}