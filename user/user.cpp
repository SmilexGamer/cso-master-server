#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, const string& userName) : _connection(connection), _userID(userID), _userName(userName) {
	struct sockaddr_in addr {};

	inet_pton(AF_INET, connection->GetIPAddress().c_str(), &(addr.sin_addr));

	_userNetwork.externalIP = addr.sin_addr.S_un.S_addr;
}

void User::SetUserNetwork(unsigned char portType, unsigned long localIP, unsigned short localPort, unsigned short externalPort) {
	_userNetwork.localIP = localIP;

	switch (portType) {
		case 0: {
			_userNetwork.localPortType0 = localPort;
			_userNetwork.externalPortType0 = externalPort;
			break;
		}
		case 1: {
			_userNetwork.localPortType1 = localPort;
			_userNetwork.externalPortType1 = externalPort;
			break;
		}
	}
}

char User::CreateCharacter(const string& nickName) {
	return databaseManager.CreateCharacter(_userID, nickName);
}

UserCharacterResult User::GetUserCharacter(unsigned short flag) {
	return databaseManager.GetUserCharacter(_userID, flag);
}

char User::IsUserCharacterExists() {
	UserCharacterResult result = GetUserCharacter(NULL);

	return result.result;
}