#include "usermanager.h"
#include "databasemanager.h"
#include "serverconfig.h"
#include "packetmanager.h"
#include "packet_userstartmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_serverlistmanager.h"

UserManager userManager;

UserManager::~UserManager() {
	for (auto& user : _users) {
		delete user;
		user = NULL;
	}

	_users.clear();
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
	databaseManager.RemoveUserSession(user->GetUserID());
	_users.erase(find(_users.begin(), _users.end(), user));
	delete user;
	user = NULL;

	UpdateChannelNumPlayers();
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

void UserManager::SendLoginPackets(User* user, Packet_ReplyType reply) {
	if (user == NULL) {
		return;
	}

	UserCharacterResult result = user->GetUserCharacter(UserCharacterFlag::All);
	if (!result.result) {
		packetManager.SendPacket_Reply(user->GetConnection(), Packet_ReplyType::SysError);
		return;
	}

	packetManager.SendPacket_Reply(user->GetConnection(), reply);
	packet_UserStartManager.SendPacket_UserStart(user, result.userCharacter);
	packet_UpdateInfoManager.SendPacket_UpdateInfo({ user, result.userCharacter });
	packet_ServerListManager.SendPacket_ServerList(user->GetConnection(), serverConfig.serverList);
}

void UserManager::UpdateChannelNumPlayers() {
	serverConfig.serverList[serverConfig.serverID - 1].channels[serverConfig.channelID - 1].numPlayers = (unsigned short)_users.size();

	databaseManager.UpdateChannelNumPlayers((unsigned short)_users.size());
}

void UserManager::OnMinuteTick() {
	databaseManager.GetChannelsNumPlayers();
}