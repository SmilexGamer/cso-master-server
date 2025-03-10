#include "packet_transfermanager.h"
#include "packetmanager.h"
#include <iostream>

Packet_TransferManager packet_transferManager;

void Packet_TransferManager::ParsePacket_Transfer(TCPConnection::Packet::pointer packet) {
	string name = packet->ReadString();
	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_Transfer - character name: {}\n", packet->GetConnection()->GetEndPoint(), name);
}