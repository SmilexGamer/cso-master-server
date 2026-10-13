#include "packet_searchroommanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "packetmanager.h"
#include "serverconsole.h"

Packet_SearchRoomManager packet_SearchRoomManager;

void Packet_SearchRoomManager::ParsePacket_SearchRoom(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_SearchRoomManager ] Client ({}) has sent Packet_SearchRoom, but it's not logged in!\n", connection->GetLogEndpoint()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_SearchRoomManager ] Parsing Packet_SearchRoom from user ({})\n", user->GetUserLogName()));

	unsigned char gameModeID = packet->ReadUInt8();
	unsigned char mapID = packet->ReadUInt8();
	unsigned char levelLimit = packet->ReadUInt8();
	unsigned char clanBattle = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_SearchRoomManager ] User ({}) has sent Packet_SearchRoom - gameModeID: {}, mapID: {}, levelLimit: {}, clanBattle: {}\n", connection->GetLogEndpoint(), gameModeID, mapID, levelLimit, clanBattle));

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_LEVEL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	SearchRoomResult searchRoomResult = roomManager.SearchRoom(userCharacterResult.userCharacter.level, gameModeID, mapID, levelLimit, clanBattle);
	sendPacket_SearchRoom_SearchRoomReply(connection, searchRoomResult.serverID, searchRoomResult.channelID, searchRoomResult.roomID);
}

void Packet_SearchRoomManager::sendPacket_SearchRoom_SearchRoomReply(TCPConnection::pointer connection, unsigned char serverID, unsigned char channelID, unsigned short roomID) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::SearchRoom });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_SearchRoomType::SearchRoomReply);
	packet->WriteUInt8(serverID);
	packet->WriteUInt8(channelID);
	packet->WriteUInt16_LE(roomID);

	packet->Send();
}