#include "packet_umsgmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "packetmanager.h"
#include "serverconsole.h"

Packet_UMsgManager packet_UMsgManager;

void Packet_UMsgManager::ParsePacket_UMsg(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UMsgManager ] Client ({}) has sent Packet_UMsg, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UMsgManager ] Parsing Packet_UMsg from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_UMsgType::WhisperChat: {
			parsePacket_UMsg_WhisperChat(user, packet);
			break;
		}
		case Packet_UMsgType::ChannelChat: {
			parsePacket_UMsg_ChannelChat(user, packet);
			break;
		}
		case Packet_UMsgType::RoomChat: {
			parsePacket_UMsg_RoomChat(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_UMsgManager ] User ({}) has sent unregistered Packet_UMsg type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_UMsgManager::SendPacket_UMsg_NoticeMessage(TCPConnection::pointer connection, const string& text) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::UMsg });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_UMsgType::NoticeMessage);
	packet->WriteString(text);

	packet->Send();
}

void Packet_UMsgManager::SendPacket_UMsg_ServerMessage(TCPConnection::pointer connection, Packet_UMsgType type, const string& text, const vector<string>& additionalText) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::UMsg });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(type);
	packet->WriteString(text);
	packet->WriteUInt8((unsigned char)additionalText.size());
	for (auto& text : additionalText) {
		packet->WriteString(text);
	}

	packet->Send();
}

void Packet_UMsgManager::parsePacket_UMsg_WhisperChat(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() == UserStatus::InServerList) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UMsgManager ] User ({}) has sent Packet_UMsg WhisperChat, but it's in the server list!\n", user->GetUserLogName()));
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_NICKNAME);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	const string& receiverNickName = packet->ReadString();
	const string& text = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_UMsgManager ] User ({}) has sent Packet_UMsg WhisperChat - receiverNickName: {}, text: {}\n", user->GetUserLogName(), receiverNickName, text));

	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* u) {
		return (u == NULL || u->GetConnection() == NULL);
	}), users.end());

	UserCharacterResult uCharacterResult;
	bool foundNickName = false;

	for (auto& u : users) {
		uCharacterResult = u->GetUserCharacter(USERCHARACTER_FLAG_NICKNAME);
		if (uCharacterResult.result && uCharacterResult.userCharacter.nickName == receiverNickName) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::InfoMessage, "MSG_TELL_OK", { receiverNickName });
			packet_UMsgManager.sendPacket_UMsg_Chat(u->GetConnection(), Packet_UMsgType::WhisperChat, userCharacterResult.userCharacter.nickName, text);

			foundNickName = true;
			break;
		}
	}

	if (!foundNickName) {
		packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::InfoMessage, "MSG_TELL_USER_NOT_FOUND", { receiverNickName });
	}
}

void Packet_UMsgManager::parsePacket_UMsg_ChannelChat(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InLobby) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UMsgManager ] User ({}) has sent Packet_UMsg ChannelChat, but it's not in the lobby!\n", user->GetUserLogName()));
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_NICKNAME);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	const string& text = packet->ReadString();

	serverConsole.Print(PrefixType::Info, format("[ Packet_UMsgManager ] User ({}) has sent Packet_UMsg ChannelChat - text: {}\n", user->GetUserLogName(), text));

	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* u) {
		return (u == NULL || u->GetConnection() == NULL || u->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	for (auto& u : users) {
		packet_UMsgManager.sendPacket_UMsg_Chat(u->GetConnection(), Packet_UMsgType::ChannelChat, userCharacterResult.userCharacter.nickName, text);
	}
}

void Packet_UMsgManager::parsePacket_UMsg_RoomChat(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UMsgManager ] User ({}) has sent Packet_UMsg RoomChat, but it's not in a room!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_RoomManager ] User ({}) has sent Packet_UMsg RoomChat, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_NICKNAME);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);
		return;
	}

	const string& text = packet->ReadString();

	vector<User*> roomUsers = room->GetRoomUsers();

	roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [](User* u) {
		return (u == NULL || u->GetConnection() == NULL || u->GetUserStatus() != UserStatus::InRoom);
	}), roomUsers.end());

	for (auto& u : roomUsers) {
		packet_UMsgManager.sendPacket_UMsg_Chat(u->GetConnection(), Packet_UMsgType::RoomChat, userCharacterResult.userCharacter.nickName, text);
	}
}

void Packet_UMsgManager::sendPacket_UMsg_Chat(TCPConnection::pointer connection, Packet_UMsgType type, const string& senderNickName, const string& text) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::UMsg });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(type);
	packet->WriteString(senderNickName);
	packet->WriteString(text);

	packet->Send();
}