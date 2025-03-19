#include "packet_umsgmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_UMsgManager packet_UMsgManager;

void Packet_UMsgManager::ParsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_UMsgManager] Client ({}) has sent Packet_UMsg, but it's not logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_UMsgManager] Parsing Packet_UMsg from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		default: {
			cout << format("[Packet_UMsgManager] Client ({}) has sent unregistered Packet_UMsg type {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}