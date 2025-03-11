#include "packet_charactermanager.h"
#include <iostream>

Packet_CharacterManager packet_CharacterManager;

void Packet_CharacterManager::ParsePacket_RecvCharacter(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_CharacterManager] Parsing Packet_Character from client ({})\n", packet->GetConnection()->GetEndPoint());

	string nickName = packet->ReadString();

	cout << format("[Packet_CharacterManager] Client ({}) has sent Packet_RecvCharacter - nickName: {}\n", packet->GetConnection()->GetEndPoint(), nickName);
}

void Packet_CharacterManager::SendPacket_Character(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { PacketID::Character });

	packet->Send();
}