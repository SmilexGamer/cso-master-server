#include "packet_umsgmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_UMsgManager packet_UMsgManager;

void Packet_UMsgManager::ParsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		serverConsole.Print(PrintType::Warn, format("[ Packet_UMsgManager ] Client ({}) has sent Packet_UMsg, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrintType::Info, format("[ Packet_UMsgManager ] Parsing Packet_UMsg from client ({})\n", user->GetUserIPAddress()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		default: {
			serverConsole.Print(PrintType::Warn, format("[ Packet_UMsgManager ] Client ({}) has sent unregistered Packet_UMsg type {}!\n", user->GetUserIPAddress(), type));
			break;
		}
	}
}

void Packet_UMsgManager::SendPacket_UMsg_ServerMessage(TCPConnection::pointer connection, Packet_UMsgType type, const string& text, const vector<string>& additionalText) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::UMsg });

	packet->WriteUInt8(type);
	packet->WriteString(text);
	packet->WriteUInt8((unsigned char)additionalText.size());
	for (auto& text : additionalText) {
		packet->WriteString(text);
	}

	packet->Send();
}