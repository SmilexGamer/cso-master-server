#include "usermanager.h"
#include "databasemanager.h"
#include "roommanager.h"
#include "packetmanager.h"
#include "packet_serverlistmanager.h"
#include "packet_cryptmanager.h"
#include "packet_clientcheckmanager.h"
#include "packet_updateinfomanager.h"
#include "packet_optionmanager.h"
#include "packet_favoritemanager.h"
#include "udp_server.h"
#include "serverconfig.h"

UserManager userManager;

UserManager::~UserManager() {
	DisconnectAllUsers();
}

char UserManager::AddUser(User* user) {
	if (user == NULL) {
		return -1;
	}

	if (_users.size() >= serverConfig.maxPlayers) {
		return Packet_ReplyType::EXCEED_MAX_CONNECTION;
	}

	char result = user->AddUserSession();
	if (result > 0) {
		_users.push_back(user);

		user->RemoveUserTransfer();
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
	if (room) {
		room->RemoveRoomUser(user);
	}

	user->RemoveUserSession();
	_users.erase(find(_users.begin(), _users.end(), user));
	delete user;

	UpdateChannelNumPlayers();
}

void UserManager::DisconnectUser(User* user) {
	if (user == NULL) {
		return;
	}

	auto& connection = user->GetConnection();
	if (connection) {
		connection->DisconnectClient();
	}
}

void UserManager::DisconnectAllUsers() {
	vector<User*> users = _users;

	for (auto& user : users) {
		DisconnectUser(user);
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

User* UserManager::GetUserByUserID(unsigned long userID) {
	if (userID == NULL) {
		return NULL;
	}

	auto it = find_if(_users.begin(), _users.end(), [&userID](User*& user) { return user->GetUserID() == userID; });
	if (it != _users.end()) {
		return _users.at(distance(_users.begin(), it));
	}

	return NULL;
}

void UserManager::DisconnectUserByConnection(TCPConnection::pointer connection, const boost::system::error_code& ec) {
	if (connection == NULL) {
		return;
	}

	User* user = GetUserByConnection(connection);
	if (user) {
		RemoveUser(user);
	}

	ec ? connection->DisconnectClient(ec) : connection->DisconnectClient();
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

	auto& connection = user->GetConnection();
	if (connection == NULL) {
		DisconnectUser(user);
		return false;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (userCharacterResult.result <= 0) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);

		DisconnectUser(user);
		return false;
	}

	if (connection->SetupEncryptCipher(serverConfig.encryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(connection, CipherType::Encrypt, connection->GetEncryptCipher());
	}

	if (connection->SetupDecryptCipher(serverConfig.decryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(connection, CipherType::Decrypt, connection->GetDecryptCipher());
	}

	if (reply != Packet_ReplyType::NoReply) {
		packetManager.SendPacket_Reply(connection, reply);
	}

	packetManager.SendPacket_UserStart({ user, userCharacterResult.userCharacter });
	packet_UpdateInfoManager.SendPacket_UpdateInfo({ user, userCharacterResult.userCharacter });

	const vector<InventoryItem>& userInventory = user->GetUserInventory();
	if (!userInventory.empty()) {
		packetManager.SendPacket_Inventory(connection, userInventory);
	}

	packet_ClientCheckManager.SendPacket_ClientCheck(connection);

	if (reply != Packet_ReplyType::NoReply) {
		user->SetUserStatus(UserStatus::InServerList);

		packet_ServerListManager.SendPacket_ServerList(connection, serverConfig.serverList);

		const vector<unsigned char>& userConfig = user->GetUserConfig();
		if (!userConfig.empty()) {
			packet_OptionManager.SendPacket_Option_UserConfig(connection, userConfig);
		}

		packet_OptionManager.SendPacket_Option_UserQuickStart(connection, user->GetUserQuickStart());

		const vector<BuyMenu>& userBuyMenus = user->GetUserBuyMenus();
		if (!userBuyMenus.empty()) {
			packet_FavoriteManager.SendPacket_Favorite_UserBuyMenu(connection, userBuyMenus);
		}

		const vector<BookMark>& userBookMarks = user->GetUserBookMarks();
		if (!userBookMarks.empty()) {
			packet_FavoriteManager.SendPacket_Favorite_UserBookMark(connection, userBookMarks);
		}
	}
	else {
		user->SetUserStatus(UserStatus::InLobby);

		SendAddUserPacketToAll(user);
		SendFullUserListPacket(connection);
		roomManager.SendFullRoomListPacket(connection);
	}

	return true;
}

void UserManager::SendFullUserListPacket(TCPConnection::pointer connection) const noexcept {
	if (connection == NULL) {
		return;
	}

	vector<User*> users = _users;

	users.erase(remove_if(users.begin(), users.end(), [](User* user) {
		return (user == NULL || user->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	vector<GameUser> gameUsers;
	UserCharacterResult userCharacterResult;

	for (auto& user : users) {
		userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
		if (userCharacterResult.result > 0) {
			gameUsers.push_back({ user, userCharacterResult.userCharacter });
		}
	}

	packet_ServerListManager.SendPacket_Lobby_FullUserList(connection, gameUsers);
}

void UserManager::SendAddUserPacketToAll(User* user) const noexcept {
	if (user == NULL) {
		return;
	}

	const UserCharacterResult& userCharacterResult = user->GetUserCharacter(USERCHARACTER_FLAG_ALL);
	if (userCharacterResult.result <= 0) {
		return;
	}

	vector<User*> users = _users;

	users.erase(remove_if(users.begin(), users.end(), [user](User* u) {
		return (u == NULL || u == user || u->GetConnection() == NULL || u->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	for (auto& u : users) {
		packet_ServerListManager.SendPacket_Lobby_AddUser(u->GetConnection(), { user, userCharacterResult.userCharacter });
	}
}

void UserManager::SendRemoveUserPacketToAll(User* user) const noexcept {
	if (user == NULL) {
		return;
	}

	vector<User*> users = _users;

	users.erase(remove_if(users.begin(), users.end(), [user](User* u) {
		return (u == NULL || u == user || u->GetConnection() == NULL || u->GetUserStatus() != UserStatus::InLobby);
	}), users.end());

	unsigned long userID = user->GetUserID();

	for (auto& u : users) {
		packet_ServerListManager.SendPacket_Lobby_RemoveUser(u->GetConnection(), userID);
	}
}

void UserManager::UpdateChannelNumPlayers() {
	serverConfig.serverList[(unsigned char)(serverConfig.serverID - 1)].channels[(unsigned char)(serverConfig.channelID - 1)].numPlayers = (unsigned short)_users.size();
	udpServer.SendNumPlayersPacketToAll();
}

void UserManager::OnMinuteTick() {
	databaseManager.RemoveOldUserTransfers();
}