#pragma once
#include "gamematchuser.h"
#include "user.h"
#include <vector>

using namespace std;

class GameMatch {
public:
	GameMatch(unsigned short roomID);
	~GameMatch();

	const vector<unsigned char>& GetSaveData() {
		return _saveData;
	}

	void SetSaveData(const vector<unsigned char> saveData) {
		_saveData = saveData;
	}

	const vector<GameMatchUser*>& GetGameMatchUsers() {
		return _gameMatchUsers;
	}

	bool AddGameMatchUser(GameMatchUser* gameMatchUser);
	void RemoveGameMatchUser(GameMatchUser* gameMatchUser);
	GameMatchUser* GetGameMatchUserByUser(User* user);

private:
	unsigned short _roomID;
	vector<unsigned char> _saveData;
	vector<GameMatchUser*> _gameMatchUsers;
};