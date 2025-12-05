#include "packet_clientcheckmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ClientCheckManager packet_ClientCheckManager;

void Packet_ClientCheckManager::ParsePacket_ClientCheck(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ClientCheckManager ] Client ({}) has sent Packet_ClientCheck, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	unsigned short cpuMHz = packet->ReadUInt16_LE(); // HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0\~MHz
	unsigned short memoryMB = packet->ReadUInt16_LE(); // GlobalMemoryStatus(), dwTotalPhys >> 20
	unsigned long upload = packet->ReadUInt32_LE(); // HKEY_CURRENT_USER\Software\Nexon\CStrike-Online\Settings\Upload. If it doesn't exist, it sends 4200
	unsigned char unk4 = packet->ReadUInt8(); // this[11] = 0;
	const string& cpuName = packet->ReadString(); // HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0\ProcessorNameString
	const string& gpuName = packet->ReadString(); // glGetString(GL_RENDERER)

	serverConsole.Print(PrefixType::Info, format("[ Packet_ClientCheckManager ] User ({}) has sent Packet_ClientCheck - cpuMHz: {}, memoryMB: {}, upload: {}, unk4: {}, cpuName: {}, gpuName: {}\n", user->GetUserLogName(), cpuMHz, memoryMB, upload, unk4, cpuName, gpuName));
}

void Packet_ClientCheckManager::SendPacket_ClientCheck(TCPConnection::pointer connection) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::ClientCheck });

	packet->Send();
}