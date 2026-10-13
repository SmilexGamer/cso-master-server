#include "packet_reportmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ReportManager packet_ReportManager;

void Packet_ReportManager::ParsePacket_RecvReport(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ReportManager ] Client ({}) has sent Packet_RecvReport, but it's not logged in!\n", connection->GetLogEndpoint()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_ReportManager ] Parsing Packet_RecvReport from user ({})\n", user->GetUserLogName()));

	unsigned char reportType = packet->ReadUInt8();
	unsigned long userID = packet->ReadUInt32_LE();
	const string& content = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_ReportManager ] User ({}) has sent Packet_RecvReport - reportType: {}, userID: {}, content: {}\n", connection->GetLogEndpoint(), reportType, userID, content));
}