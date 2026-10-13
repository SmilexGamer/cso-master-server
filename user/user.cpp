#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, const string& userName) : _connection(connection), _userID(userID), _userName(userName) {
	if (_connection == NULL) {
		delete this;
		return;
	}

	_userStatus = UserStatus::InLogin;
	_currentRoomID = 0;
}

void User::SetUserNetwork(PortType portType, unsigned long externalIP, unsigned short externalPort, unsigned long localIP, unsigned short localPort, unsigned long long currentTime) noexcept {
	_userNetwork.externalIP = externalIP;
	_userNetwork.localIP = localIP;

	switch (portType) {
		case PortType::Guest: {
			_userNetwork.externalGuestPort = externalPort;
			_userNetwork.localGuestPort = localPort;
			_userNetwork.lastGuestHeartbeat = currentTime;
			_userNetwork.networkSet |= 0x1;
			break;
		}
		case PortType::Host: {
			_userNetwork.externalHostPort = externalPort;
			_userNetwork.localHostPort = localPort;
			_userNetwork.lastHostHeartbeat = currentTime;
			_userNetwork.networkSet |= 0x2;
			break;
		}
	}
}

void User::SetUserNetworkHeartbeat(PortType portType, unsigned long long currentTime) noexcept {
	switch (portType) {
		case PortType::Guest: {
			_userNetwork.lastGuestHeartbeat = currentTime;
			break;
		}
		case PortType::Host: {
			_userNetwork.lastHostHeartbeat = currentTime;
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

bool User::CreateUserBuymenus() const noexcept {
	return databaseManager.CreateUserBuymenus(_userID);
}

bool User::CreateUserBookmarks() const noexcept {
	return databaseManager.CreateUserBookmarks(_userID);
}

bool User::CreateUserInventory() const noexcept {
	return databaseManager.CreateUserInventory(_userID);
}

char User::AddUserSession() const noexcept {
	return databaseManager.AddUserSession(_userID);
}

void User::RemoveUserSession() const noexcept {
	databaseManager.RemoveUserSession(_userID);
}

char User::AddUserTransfer(const string& authToken, unsigned char serverID, unsigned char channelID) const noexcept {
	return databaseManager.AddUserTransfer(_userID, authToken, serverID, channelID);
}

void User::RemoveUserTransfer() const noexcept {
	databaseManager.RemoveUserTransfer(_userID);
}

char User::IsUserCharacterExists() const noexcept {
	const UserCharacterResult& userCharacterResult = GetUserCharacter(NULL);

	return userCharacterResult.result;
}

bool User::SaveUserConfig(const vector<unsigned char>& userConfig) const noexcept {
	return databaseManager.SaveUserConfig(_userID, userConfig);
}

const vector<unsigned char> User::GetUserConfig() const noexcept {
	return databaseManager.GetUserConfig(_userID);
}

bool User::SaveUserQuickStart(unsigned long userQuickStart) const noexcept {
	return databaseManager.SaveUserQuickStart(_userID, userQuickStart);
}

unsigned long User::GetUserQuickStart() const noexcept {
	return databaseManager.GetUserQuickStart(_userID);
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