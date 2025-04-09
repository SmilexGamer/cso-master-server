#include "packet_serverlistmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_ServerListManager packet_ServerListManager;

void Packet_ServerListManager::ParsePacket_RequestServerList(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ServerListManager ] Client ({}) has sent Packet_RequestServerList, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	if (user->GetUserStatus() == UserStatus::InLobby) {
		user->SetUserStatus(UserStatus::InServerList);

		userManager.SendRemoveUserPacketToAll(user);
	}

	if (user->GetUserStatus() != UserStatus::InServerList) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_ServerListManager ] User ({}) has sent Packet_RequestServerList, but it's not in server list!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_ServerListManager ] User ({}) has sent Packet_RequestServerList\n", user->GetUserLogName()));

	SendPacket_ServerList(user->GetConnection(), serverConfig.serverList);
}

void Packet_ServerListManager::SendPacket_ServerList(TCPConnection::pointer connection, const vector<Server>& servers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::ServerList });

	packet->WriteUInt8((unsigned char)servers.size());

	for (auto& server : servers) {
		packet->WriteUInt8(server.id);
		packet->WriteBool(server.status);
		packet->WriteUInt8(server.type);
		packet->WriteString(server.name);

		packet->WriteUInt8((unsigned char)server.channels.size());

		for (auto& channel : server.channels) {
			packet->WriteUInt8(channel.id);
			packet->WriteString(channel.name);
			packet->WriteUInt16_LE(channel.numPlayers);
		}
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby_FullUserList(TCPConnection::pointer connection, const vector<GameUser>& gameUsers) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Lobby });

	packet->WriteUInt8(Packet_LobbyType::FullUserList);
	packet->WriteUInt16_LE((unsigned short)gameUsers.size());

	for (auto& gameUser : gameUsers) {
		buildGameUser(packet, gameUser);
	}

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby_AddUser(TCPConnection::pointer connection, const GameUser& gameUser) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Lobby });

	packet->WriteUInt8(Packet_LobbyType::AddUser);
	buildGameUser(packet, gameUser);

	packet->Send();
}

void Packet_ServerListManager::SendPacket_Lobby_RemoveUser(TCPConnection::pointer connection, unsigned long userID) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Lobby });

	packet->WriteUInt8(Packet_LobbyType::RemoveUser);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}

void Packet_ServerListManager::buildGameUser(TCPConnection::Packet::pointer packet, const GameUser& gameUser) {
	packet->WriteUInt32_LE(gameUser.user->GetUserID());
	packet->WriteString(gameUser.user->GetUserName());
	packetManager.BuildUserCharacter(packet, gameUser.userCharacter);
}