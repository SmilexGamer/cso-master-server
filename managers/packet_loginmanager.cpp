#include "packet_loginmanager.h"
#include "packetmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_userstartmanager.h"
#include <iostream>

Packet_LoginManager packet_LoginManager;

void Packet_LoginManager::ParsePacket_Login(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_LoginManager] Parsing Packet_Login from client ({})\n", packet->GetConnection()->GetEndPoint());

	string username = packet->ReadString();
	string password = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();

	string hardwareIDStr;
	for (auto c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_LoginManager] Client ({}) has sent Packet_Login - username: {}, password: {}, hardwareID:{}, pcBang: {}\n", packet->GetConnection()->GetEndPoint(), username.c_str(), password.c_str(), hardwareIDStr.c_str(), pcBang);

	packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::LoginSuccess);
	packet_UserStartManager.SendPacket_UserStart(packet->GetConnection());
	packet_UpdateInfoManager.SendPacket_UpdateInfo(packet->GetConnection(), UserInfoFlag::All);
	packet_ServerListManager.SendPacket_ServerList(packet->GetConnection());
}