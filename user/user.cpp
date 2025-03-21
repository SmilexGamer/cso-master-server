#include "user.h"
#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, const string& userName) : _connection(connection), _userID(userID), _userName(userName) {

}

UserCharacterResult User::GetUserCharacter(unsigned short flag) {
	return databaseManager.GetUserCharacter(_userID, flag);
}

char User::IsUserCharacterExists() {
	UserCharacterResult result = GetUserCharacter(NULL);

	return result.result;
}