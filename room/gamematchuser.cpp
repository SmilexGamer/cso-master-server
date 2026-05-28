#include "gamematchuser.h"
#include <cstddef>

GameMatchUser::GameMatchUser(User* user) : _user(user) {
	if (user == NULL) {
		delete this;
		return;
	}

	SetTeam(Team::None);
}