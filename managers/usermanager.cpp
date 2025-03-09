#include "usermanager.h"
#include "databasemanager.h"
#include "serverconfig.h"

UserManager userManager;

UserManager::UserManager() {
	
};

UserManager::~UserManager() {
	for (auto& user : _users) {
		user->GetConnection()->DisconnectClient();
		delete user;
		user = NULL;
	}

	_users.clear();
}

void UserManager::AddUser(User* user) {
	if (user != NULL) {
		_users.push_back(user);
	}
}

void UserManager::RemoveUser(User* user) {
	_users.erase(find(_users.begin(), _users.end(), user));
}

void UserManager::DisconnectUserByConnection(TCPConnection::pointer connection) {
	if (connection != NULL) {
		User* user = GetUserByConnection(connection);
		if (user) {
			RemoveUser(user);
			delete user;
			user = NULL;
		}
		
		connection->DisconnectClient();
	}
}

void UserManager::DisconnectUserByConnection(TCPConnection::pointer connection, boost::system::error_code ec) {
	if (connection != NULL) {
		User* user = GetUserByConnection(connection);
		if (user) {
			RemoveUser(user);
			delete user;
			user = NULL;
		}

		connection->DisconnectClient(ec);
	}
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

void UserManager::OnMinuteTick() {
	serverConfig.serverList[serverConfig.serverID - 1].channels[serverConfig.channelID - 1].numPlayers = (unsigned short)_users.size();

	databaseManager.UpdateChannelNumPlayers((unsigned short)_users.size());
	databaseManager.GetChannelsNumPlayers();
}