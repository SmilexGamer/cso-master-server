#include "packet_userstartmanager.h"
#include "serverconsole.h"

Packet_UserStartManager packet_UserStartManager;

void Packet_UserStartManager::ParsePacket_UserStart(TCPConnection::Packet::pointer packet) {
	serverConsole.Print(PrintType::Info, format("[ Packet_UserStartManager ] Parsing Packet_UserStart from client ({})\n", packet->GetConnection()->GetIPAddress()));

	unsigned char type = packet->ReadUInt8();
	unsigned long unk1 = packet->ReadUInt32_LE();
	string unk2 = packet->ReadString();

	serverConsole.Print(PrintType::Info, format("[ Packet_UserStartManager ] Client ({}) has sent Packet_UserStart - type: {}, unk1: {}, unk2: {}\n", packet->GetConnection()->GetIPAddress(), type, unk1, unk2));
}

void Packet_UserStartManager::SendPacket_UserStart(const GameUser& gameUser) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, gameUser.user->GetConnection(), { (unsigned char)PacketID::UserStart });

	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packet->WriteString(gameUser.user->GetUserName());
	packet->WriteString(gameUser.userCharacter.nickName);

	packet->Send();
}