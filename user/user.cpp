#include "user.h"
#include "databasemanager.h"

User::User(TCPConnection::pointer connection, unsigned long userID, string userName) : _connection(connection), _userID(userID), _userName(userName) {

}

int User::GetCharacter(UserCharacter& userCharacter) {
	return databaseManager.GetUserCharacter(_userID, userCharacter);
}

int User::IsCharacterExists() {
	UserCharacter userCharacter;
	userCharacter.flag = UserInfoFlag::Unk1;

	return GetCharacter(userCharacter);
}