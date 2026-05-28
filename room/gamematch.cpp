#include "gamematch.h"
#include "usermanager.h"

GameMatch::GameMatch(unsigned short roomID) : _roomID(roomID) {
	if (roomID == NULL) {
		delete this;
		return;
	}
}

GameMatch::~GameMatch() {
	for (auto& gameMatchUser : _gameMatchUsers) {
		delete gameMatchUser;
		gameMatchUser = NULL;
	}

	_gameMatchUsers.clear();
}

bool GameMatch::AddGameMatchUser(GameMatchUser* gameMatchUser) {
	if (gameMatchUser == NULL) {
		return false;
	}

	_gameMatchUsers.push_back(gameMatchUser);
	return true;
}

void GameMatch::RemoveGameMatchUser(GameMatchUser* gameMatchUser) {
	if (gameMatchUser == NULL) {
		return;
	}

	_gameMatchUsers.erase(find(_gameMatchUsers.begin(), _gameMatchUsers.end(), gameMatchUser));
	delete gameMatchUser;
	gameMatchUser = NULL;
}

GameMatchUser* GameMatch::GetGameMatchUserByUser(User* user) {
	if (user == NULL) {
		return NULL;
	}

	auto it = find_if(_gameMatchUsers.begin(), _gameMatchUsers.end(), [&user](GameMatchUser*& gameMatchUser) { return gameMatchUser->GetUser() == user; });
	if (it != _gameMatchUsers.end()) {
		return _gameMatchUsers.at(distance(_gameMatchUsers.begin(), it));
	}

	return NULL;
}