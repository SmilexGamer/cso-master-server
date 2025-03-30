#include "usermanager.h"
#include "databasemanager.h"
#include "roommanager.h"
#include "packetmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_userstartmanager.h"

UserManager userManager;

UserManager::~UserManager() {
	RemoveAllUsers();
}

char UserManager::AddUser(User* user) {
	if (user == NULL) {
		return -1;
	}

	char result = databaseManager.AddUserSession(user->GetUserID());
	if (result) {
		_users.push_back(user);

		UpdateChannelNumPlayers();
	}

	return result;
}

void UserManager::RemoveUser(User* user) {
	if (user == NULL) {
		return;
	}

	if (user->GetUserStatus() == UserStatus::InLobby) {
		SendRemoveUserPacketToAll(user);
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room != NULL) {
		room->RemoveRoomUser(user);
	}

	databaseManager.RemoveUserSession(user->GetUserID());
	_users.erase(find(_users.begin(), _users.end(), user));
	delete user;
	user = NULL;

	UpdateChannelNumPlayers();
}

void UserManager::RemoveAllUsers() {
	for (auto& user : _users) {
		delete user;
		user = NULL;
	}

	_users.clear();
}

User* UserManager::GetUserByConnection(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return NULL;
	}

	auto it = find_if(_users.begin(), _users.end(), [&connection](User*& user) { return user->GetConnection() == connection; });
	if (it != _users.end()) {
		return _users.at(distance(_users.begin(), it));
	}

	return NULL;
}

User* UserManager::GetUserByUserID(unsigned long userID) {
	if (!userID) {
		return NULL;
	}

	auto it = find_if(_users.begin(), _users.end(), [&userID](User*& user) { return user->GetUserID() == userID; });
	if (it != _users.end()) {
		return _users.at(distance(_users.begin(), it));
	}

	return NULL;
}

void UserManager::RemoveUserByConnection(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	User* user = GetUserByConnection(connection);
	if (user == NULL) {
		return;
	}

	RemoveUser(user);
}

bool UserManager::IsUserLoggedIn(User* user) {
	if (user == NULL) {
		return false;
	}

	if (user->GetUserStatus() == UserStatus::InLogin) {
		return false;
	}

	return true;
}

bool UserManager::SendLoginPackets(User* user, Packet_ReplyType reply) {
	if (user == NULL) {
		return false;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);

		RemoveUser(user);
		return false;
	}

	if (reply != Packet_ReplyType::NoReply) {
		user->SetUserStatus(UserStatus::InServerList);

		packetManager.SendPacket_Reply(user->GetConnection(), reply);
		packet_ServerListManager.SendPacket_ServerList(user->GetConnection(), serverConfig.serverList);
	}
	else {
		user->SetUserStatus(UserStatus::InLobby);

		SendAddUserPacketToAll(user);
		SendFullUserListPacket(user->GetConnection());
		roomManager.SendFullRoomListPacket(user->GetConnection());
	}

	packet_UserStartManager.SendPacket_UserStart({ user, userCharacterResult.userCharacter });
	packet_UpdateInfoManager.SendPacket_UpdateInfo({ user, userCharacterResult.userCharacter });

	return true;
}

void UserManager::SendFullUserListPacket(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	vector<GameUser> gameUsers;
	for (auto& user : _users) {
		if (user->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (userCharacterResult.result) {
			gameUsers.push_back({ user, userCharacterResult.userCharacter });
		}
	}

	packet_ServerListManager.SendPacket_Lobby_FullUserList(connection, gameUsers);
}

void UserManager::SendAddUserPacketToAll(User* user) {
	if (user == NULL) {
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (!userCharacterResult.result) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
		return;
	}

	for (auto& u : _users) {
		if (u == user || u->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		packet_ServerListManager.SendPacket_Lobby_AddUser(u->GetConnection(), { user, userCharacterResult.userCharacter });
	}
}

void UserManager::SendRemoveUserPacketToAll(User* user) {
	if (user == NULL) {
		return;
	}

	unsigned long userID = user->GetUserID();

	for (auto& u : _users) {
		if (u == user || u->GetUserStatus() != UserStatus::InLobby) {
			continue;
		}

		packet_ServerListManager.SendPacket_Lobby_RemoveUser(u->GetConnection(), userID);
	}
}

void UserManager::UpdateChannelNumPlayers() {
	serverConfig.serverList[serverConfig.serverID - 1].channels[serverConfig.channelID - 1].numPlayers = (unsigned short)_users.size();

	databaseManager.UpdateChannelNumPlayers((unsigned short)_users.size());
}

void UserManager::OnMinuteTick() {
	databaseManager.GetAllChannelsNumPlayers();
	databaseManager.RemoveOldUserTransfers();
}