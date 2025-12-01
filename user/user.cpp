#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, const string& userName) : _connection(connection), _userID(userID), _userName(userName) {
	struct sockaddr_in addr {};

	inet_pton(AF_INET, connection->GetIPAddress().c_str(), &(addr.sin_addr));

	_userNetwork.externalIP = addr.sin_addr.S_un.S_addr;

	_userStatus = UserStatus::InLogin;
	_currentRoomID = 0;
}

void User::SetUserNetwork(PortType portType, unsigned long localIP, unsigned short localPort, unsigned short externalPort) {
	_userNetwork.localIP = localIP;

	switch (portType) {
		case PortType::Guest: {
			_userNetwork.localGuestPort = localPort;
			_userNetwork.externalGuestPort = externalPort;
			break;
		}
		case PortType::Host: {
			_userNetwork.localHostPort = localPort;
			_userNetwork.externalHostPort = externalPort;
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

char User::AddUserSession() const noexcept {
	return databaseManager.AddUserSession(_userID);
}

void User::RemoveUserSession() const noexcept {
	databaseManager.RemoveUserSession(_userID);
}

char User::AddUserTransfer(unsigned char serverID, unsigned char channelID) const noexcept {
	return databaseManager.AddUserTransfer(_userName, _connection->GetIPAddress(), serverID, channelID);
}

void User::RemoveUserTransfer() const noexcept {
	databaseManager.RemoveUserTransfer(_userName);
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