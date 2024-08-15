#include "Packet_UMsgManager.h"
#include "definitions.h"
#include <iostream>

void Packet_UMsgManager::ParsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_UMsgManager] Parsing Packet_UMsg from {}\n", packet->GetConnection()->GetEndPoint());

	unsigned char subID = packet->ReadUInt8();

	switch (subID) {
	default: {
		cout << format("[Packet_UMsgManager] Client ({}) has sent unregistered Packet_UMsg subID {}!\n", packet->GetConnection()->GetEndPoint(), subID & 0xFF);
		break;
	}
	}
}