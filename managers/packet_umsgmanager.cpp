#include "packet_umsgmanager.h"
#include <iostream>

Packet_UMsgManager packet_UMsgManager;

void Packet_UMsgManager::ParsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UMsgManager] Parsing Packet_UMsg from {}\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
	default: {
		cout << format("[Packet_UMsgManager] Client ({}) has sent unregistered Packet_UMsg type {}!\n", packet->GetConnection()->GetEndPoint(), type & 0xFF);
		break;
	}
	}
}