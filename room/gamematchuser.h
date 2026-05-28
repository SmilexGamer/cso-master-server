#pragma once
#include "user.h"

enum Team {
	None = 0,
	Terrorist = 1,
	CounterTerrorist = 2
};

class GameMatchUser {
public:
	GameMatchUser(User* user);

	User* GetUser() const noexcept {
		return _user;
	}

	void SetTeam(Team team) {
		_team = team;
	}

private:
	User* _user;
	Team _team;
};