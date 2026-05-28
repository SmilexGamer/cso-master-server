#include "packet_transfermanager.h"
#include "packet_umsgmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include "serverconsole.h"
#include "databasemanager.h"
#include "roommanager.h"
#include "udp_server.h"
#include "libbcrypt/bcrypt.h"

Packet_TransferManager packet_TransferManager;

void Packet_TransferManager::ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_TransferManager ] User ({}) has sent Packet_TransferLogin, but it's already logged in!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_TransferManager ] Parsing Packet_TransferLogin from client ({})\n", connection->GetIPAddress()));

	const string& authToken = packet->ReadString();
	const vector<unsigned char>& hardwareID = packet->ReadArray_UInt8(HARDWARE_ID_SIZE);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {:02X}", c);
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_TransferManager ] Client ({}) has sent Packet_TransferLogin - authToken: {}, hardwareID:{}, pcBang: {}, unk: {}\n", connection->GetIPAddress(), authToken, hardwareIDStr, pcBang, unk));

	if (userManager.GetUsers().size() >= serverConfig.maxPlayers) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::EXCEED_MAX_CONNECTION);
		return;
	}

	const TransferLoginResult& transferLoginResult = databaseManager.TransferLogin(authToken);
	if (transferLoginResult.reply > Packet_ReplyType::LoginSuccess) {
		packetManager.SendPacket_Reply(connection, transferLoginResult.reply);
		return;
	}

	User* newUser = new User(connection, transferLoginResult.userID, transferLoginResult.userName, transferLoginResult.userNetwork);
	char userResult = userManager.AddUser(newUser, true);
	if (!userResult) {
		if (userResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);

			delete newUser;
			newUser = NULL;
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::Playing);

		delete newUser;
		newUser = NULL;
		return;
	}

	if (!userManager.SendLoginPackets(newUser) && connection) {
		connection->DisconnectClient();
		return;
	}
}

void Packet_TransferManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_TransferManager ] Client ({}) has sent Packet_RequestTransfer, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_TransferManager ] Parsing Packet_RequestTransfer from user ({})\n", user->GetUserLogName()));

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_TransferManager ] User ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", user->GetUserLogName(), serverID, channelID));

	if (!serverID || serverID > serverConfig.serverList.size()) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::InvalidServer);
		return;
	}

	if (!channelID || channelID > serverConfig.serverList[serverID - 1].channels.size()) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::InvalidServer);
		return;
	}

	if (serverID == serverConfig.serverID && channelID == serverConfig.channelID) {
		if (user->GetUserStatus() == UserStatus::InServerList) {
			user->SetUserStatus(UserStatus::InLobby);

			userManager.SendAddUserPacketToAll(user);
			userManager.SendFullUserListPacket(connection);
			roomManager.SendFullRoomListPacket(connection);
		}
	}
	else {
		if (!serverConfig.serverList[serverID - 1].channels[channelID - 1].isOnline) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_SERVER_DOWN");
			return;
		}

		if (serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers >= serverConfig.serverList[serverID - 1].channels[channelID - 1].maxPlayers) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_LOBBY_FULL");
			return;
		}

		const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_LEVEL);
		if (!userCharacterResult.result) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		if (serverConfig.serverList[serverID - 1].type == ServerType::Newbie && userCharacterResult.userCharacter.level >= 9) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_LEVEL_LIMIT");
			return;
		}

		char authToken[BCRYPT_HASHSIZE];
		int ret = bcrypt_gensalt(12, authToken);
		if (ret != 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
			return;
		}

		char addUserTransferResult = user->AddUserTransfer(string{ authToken }, serverID, channelID);
		if (!addUserTransferResult) {
			if (addUserTransferResult < 0) {
				packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
				return;
			}

			packetManager.SendPacket_Reply(connection, Packet_ReplyType::TRANSFER_ERR);
			return;
		}

		struct sockaddr_in addr {};

		inet_pton(AF_INET, serverConfig.serverList[serverID - 1].channels[channelID - 1].ip.c_str(), &(addr.sin_addr));

		sendPacket_Transfer(connection, (unsigned long)addr.sin_addr.S_un.S_addr, serverConfig.serverList[serverID - 1].channels[channelID - 1].port, string{ authToken });
	}
}

void Packet_TransferManager::sendPacket_Transfer(TCPConnection::pointer connection, unsigned long ip, unsigned short port, const string& authToken) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Transfer });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt32_LE(ip);
	packet->WriteUInt16_LE(port);
	packet->WriteUInt8(0); // unk
	packet->WriteString(authToken);

	packet->Send();
}