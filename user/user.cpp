#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, const string& userName, const UserNetwork& userNetwork) : _connection(connection), _userID(userID), _userName(userName), _userNetwork(userNetwork) {
	if (_connection == NULL) {
		delete this;
		return;
	}

	struct sockaddr_in addr {};

	inet_pton(AF_INET, connection->GetIPAddress().c_str(), &(addr.sin_addr));

	_userNetwork.externalIP = addr.sin_addr.S_un.S_addr;

	_userStatus = UserStatus::InLogin;
	_currentRoomID = 0;
}

void User::SetUserNetwork(PortType portType, unsigned long externalIP, unsigned short externalPort, unsigned long localIP, unsigned short localPort) {
	_userNetwork.externalIP = externalIP;
	_userNetwork.localIP = localIP;

	switch (portType) {
		case PortType::Guest: {
			_userNetwork.externalGuestPort = externalPort;
			_userNetwork.localGuestPort = localPort;
			break;
		}
		case PortType::Host: {
			_userNetwork.externalHostPort = externalPort;
			_userNetwork.localHostPort = localPort;
			break;
		}
	}
}

char User::CreateUserCharacter(const string& nickName) const noexcept {
	return databaseManager.CreateUserCharacter(_userID, nickName);
}

UserCharacterResult User::GetUserCharacter(unsigned short flag) const noexcept {
	return databaseManager.GetUserCharacter(_userID, flag);
}

char User::CreateUserBuymenus() const noexcept {
	return databaseManager.CreateUserBuymenus(_userID);
}

char User::CreateUserBookmarks() const noexcept {
	return databaseManager.CreateUserBookmarks(_userID);
}

char User::CreateUserInventory() const noexcept {
	return databaseManager.CreateUserInventory(_userID);
}

char User::AddUserSession() const noexcept {
	return databaseManager.AddUserSession(_userID);
}

void User::RemoveUserSession() const noexcept {
	databaseManager.RemoveUserSession(_userID);
}

char User::AddUserTransfer(const string& authToken, unsigned char serverID, unsigned char channelID) const noexcept {
	return databaseManager.AddUserTransfer(_userID, authToken, serverID, channelID, _userNetwork);
}

void User::RemoveUserTransfer() const noexcept {
	databaseManager.RemoveUserTransfer(_userID);
}

char User::IsUserCharacterExists() const noexcept {
	const UserCharacterResult& userCharacterResult = GetUserCharacter(NULL);

	return userCharacterResult.result;
}

bool User::SaveUserOption(const vector<unsigned char>& userOption) const noexcept {
	return databaseManager.SaveUserOption(_userID, userOption);
}

const vector<unsigned char> User::GetUserOption() const noexcept {
	return databaseManager.GetUserOption(_userID);
}

bool User::SaveUserBuyMenu(unsigned char categoryID, unsigned char slotID, unsigned char itemID) const noexcept {
	return databaseManager.SaveUserBuyMenu(_userID, categoryID, slotID, itemID);
}

const vector<BuyMenu> User::GetUserBuyMenus() const noexcept {
	return databaseManager.GetUserBuyMenus(_userID);
}

bool User::SaveUserBookMark(const BookMark& userBookMark) const noexcept {
	return databaseManager.SaveUserBookMark(_userID, userBookMark);
}

const vector<BookMark> User::GetUserBookMarks() const noexcept {
	return databaseManager.GetUserBookMarks(_userID);
}

const vector<InventoryItem> User::GetUserInventory() const noexcept {
	return databaseManager.GetUserInventory(_userID);
}