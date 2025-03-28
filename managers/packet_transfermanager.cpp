#include "packet_transfermanager.h"
#include "packet_serverlistmanager.h"
#include "packet_cryptmanager.h"
#include "packet_roommanager.h"
#include "packet_umsgmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include "databasemanager.h"
#include "roommanager.h"
#include <iostream>

Packet_TransferManager packet_TransferManager;

void Packet_TransferManager::ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user != NULL) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_TransferLogin, but it's already logged in!\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_TransferManager] Parsing Packet_TransferLogin from client ({})\n", packet->GetConnection()->GetIPAddress());

	string userName = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_TransferLogin - userName: {}, hardwareID:{}, pcBang: {}, unk: {}\n", packet->GetConnection()->GetIPAddress(), userName, hardwareIDStr, pcBang, unk);

	if (userManager.GetUsers().size() >= serverConfig.maxPlayers) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::EXCEED_MAX_CONNECTION);
		return;
	}

	LoginResult transferLoginResult = databaseManager.TransferLogin(userName, packet->GetConnection()->GetIPAddress());
	if (transferLoginResult.reply > Packet_ReplyType::LoginSuccess) {
		packetManager.SendPacket_Reply(packet->GetConnection(), transferLoginResult.reply);
		return;
	}

	databaseManager.RemoveUserTransfer(userName);

	User* newUser = new User(packet->GetConnection(), transferLoginResult.userID, userName);
	char userResult = userManager.AddUser(newUser);
	if (!userResult) {
		if (userResult < 0) {
			packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::Playing);
		return;
	}

#ifdef NO_SSL
	if (newUser->GetConnection()->SetupEncryptCipher(serverConfig.encryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(newUser->GetConnection(), CipherType::Encrypt, newUser->GetConnection()->GetEncryptCipher());
	}

	if (newUser->GetConnection()->SetupDecryptCipher(serverConfig.decryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(newUser->GetConnection(), CipherType::Decrypt, newUser->GetConnection()->GetDecryptCipher());
	}
#endif

	vector<User*> users = userManager.GetUsers();
	vector<UserFull> fullUsers;
	UserCharacterResult userCharacterResult;
	for (auto& user : users) {
		if (user->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (userCharacterResult.result) {
			fullUsers.push_back({ user, userCharacterResult.userCharacter });
		}
	}

	userManager.SendLoginPackets(newUser);
	packet_ServerListManager.SendPacket_Lobby_FullUserList(newUser->GetConnection(), fullUsers);
	packet_RoomManager.SendPacket_RoomList_FullRoomList(newUser->GetConnection(), roomManager.GetRooms(), ROOMLIST_FLAG_ALL);
}

void Packet_TransferManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer, but it's not logged in!\n", packet->GetConnection()->GetIPAddress());
		return;
	}

	cout << format("[Packet_TransferManager] Parsing Packet_RequestTransfer from client ({})\n", user->GetUserIPAddress());

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", user->GetUserIPAddress(), serverID, channelID);

	if (!serverID || serverID > serverConfig.serverList.size()) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::InvalidServer);
		return;
	}

	if (!channelID || channelID > serverConfig.serverList[serverID - 1].channels.size()) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::InvalidServer);
		return;
	}

	if (serverID == serverConfig.serverID && channelID == serverConfig.channelID) {
		vector<User*> users = userManager.GetUsers();
		vector<UserFull> fullUsers;
		UserCharacterResult result;
		for (auto& user : users) {
			if (user->GetUserStatus() != UserStatus::InLobby) {
				continue;
			}

			result = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
			if (result.result) {
				fullUsers.push_back({ user, result.userCharacter });
			}
		}

		packet_ServerListManager.SendPacket_Lobby_FullUserList(user->GetConnection(), fullUsers);
		packet_RoomManager.SendPacket_RoomList_FullRoomList(user->GetConnection(), roomManager.GetRooms(), ROOMLIST_FLAG_ALL);
	}
	else {
		char getChannelNumPlayersResult = databaseManager.GetChannelNumPlayers(serverID, channelID);
		if (!getChannelNumPlayersResult) {
			if (getChannelNumPlayersResult < 0) {
				packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
				return;
			}

			packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_SERVER_DOWN");
			return;
		}

		if (serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers >= serverConfig.serverList[serverID - 1].channels[channelID - 1].maxPlayers) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_LOBBY_FULL");
			return;
		}

		UserCharacterResult userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_LEVEL);
		if (!userCharacterResult.result) {
			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
			return;
		}

		if (serverConfig.serverList[serverID - 1].type == ServerType::Newbie && userCharacterResult.userCharacter.level >= 9) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "SERVER_SELECT_FAIL_LEVEL_LIMIT");
			return;
		}

		char addUserTransferResult = databaseManager.AddUserTransfer(user->GetUserName(), user->GetUserIPAddress(), serverID, channelID);
		if (!addUserTransferResult) {
			if (addUserTransferResult < 0) {
				packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
				return;
			}

			packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::TRANSFER_ERR);
			return;
		}

		struct sockaddr_in addr {};

		inet_pton(AF_INET, serverConfig.serverList[serverID - 1].channels[channelID - 1].ip.c_str(), &(addr.sin_addr));

		sendPacket_Transfer(user, (unsigned long)addr.sin_addr.S_un.S_addr, serverConfig.serverList[serverID - 1].channels[channelID - 1].port);
	}
}

void Packet_TransferManager::sendPacket_Transfer(User* user, unsigned long ip, unsigned short port) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, user->GetConnection(), { (unsigned char)PacketID::Transfer });

	packet->WriteUInt32_LE(ip);
	packet->WriteUInt16_LE(port);
	packet->WriteUInt8(0); // unk
	packet->WriteString(user->GetUserName());

	packet->Send();
}