#include "packet_hostmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "packet_roommanager.h"
#include "serverconsole.h"

Packet_HostManager packet_HostManager;

void Packet_HostManager::ParsePacket_Host(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] Client ({}) has sent Packet_Host, but it's not logged in!\n", connection->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] Parsing Packet_Host from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_HostType::ServerIsActivated: {
			parsePacket_Host_ServerIsActivated(user);
			break;
		}
		case Packet_HostType::SaveData: {
			parsePacket_Host_SaveData(user, packet);
			break;
		}
		case Packet_HostType::SendUserStatus: {
			parsePacket_Host_SendUserStatus(user, packet);
			break;
		}
		case Packet_HostType::PlayerKilled: {
			parsePacket_Host_PlayerKilled(user, packet);
			break;
		}
		case Packet_HostType::EndGame: {
			parsePacket_Host_EndGame(user, packet);
			break;
		}
		case Packet_HostType::RejoinFail: {
			parsePacket_Host_RejoinFail(user, packet);
			break;
		}
		case Packet_HostType::AddKill: {
			parsePacket_Host_AddKill(user, packet);
			break;
		}
		case Packet_HostType::AddDeath: {
			parsePacket_Host_AddDeath(user, packet);
			break;
		}
		case Packet_HostType::RoundResult: {
			parsePacket_Host_RoundResult(user, packet);
			break;
		}
		case Packet_HostType::SelectTeam: {
			parsePacket_Host_SelectTeam(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent unregistered Packet_Host type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_HostManager::SendPacket_Host_HostStart(User* user) {
	if (user == NULL) {
		return;
	}

	auto connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_HostType::HostStart);
	packet->WriteUInt32_LE(user->GetUserID());

	packet->Send();
}

void Packet_HostManager::SendPacket_Host_HostJoin(TCPConnection::pointer connection, unsigned long userID) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_HostType::HostJoin);
	packet->WriteUInt32_LE(userID);

	packet->Send();
}

void Packet_HostManager::SendPacket_Host_HostRestart(TCPConnection::pointer connection, bool isRoomHost, unsigned long roomHostUserID, const vector<GameMatchUser*>& gameMatchUsers, const vector<unsigned char>& saveData) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_HostType::HostRestart);
	packet->WriteUInt32_LE(roomHostUserID);

	if (isRoomHost) {
		packet->WriteUInt16_LE((unsigned short)saveData.size());

		if (!saveData.empty()) {
			packet->WriteArray_UInt8(saveData);
			packet->WriteUInt8((unsigned char)gameMatchUsers.size());

			for (auto& gameMatchUser : gameMatchUsers) {
				packet->WriteUInt32_LE(gameMatchUser->GetUser()->GetUserID());
			}
		}
	}
	else {
		packet->WriteUInt8(1); // crashFlag
	}

	packet->Send();
}

void Packet_HostManager::SendPacket_Host_ResultClose(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_HostType::ResultClose);

	packet->Send();
}

void Packet_HostManager::parsePacket_Host_ServerIsActivated(User* user) {
	if (user == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom && user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host ServerIsActivated, but it's not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host ServerIsActivated, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host ServerIsActivated, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host ServerIsActivated\n", user->GetUserLogName()));

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		GameMatch* newGameMatch = new GameMatch(room->GetRoomID());
		if (newGameMatch == NULL) {
			return;
		}

		room->SetGameMatch(newGameMatch);

		RoomSettings roomSettings;
		roomSettings.lowFlag = ROOMSETTINGS_LFLAG_ROOMSTATE;
		roomSettings.roomState = RoomState::Playing;
		room->UpdateRoomSettings(roomSettings);

		roomManager.SendUpdateRoomPacketToAll(room, ROOMLIST_FLAG_ROOMSTATE);

		vector<User*> roomUsers = room->GetRoomUsers();

		roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [](User* u) {
			return (u == NULL || u->GetConnection() == NULL);
		}), roomUsers.end());

		for (auto& u : roomUsers) {
			packet_RoomManager.SendPacket_Room_ReplyUpdateRoomSettings(u->GetConnection(), roomSettings);
			roomManager.SendUpdateRoomPacket(u->GetConnection(), room, ROOMLIST_FLAG_ROOMSTATE);
		}
	}
}

void Packet_HostManager::parsePacket_Host_SaveData(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned short saveSize = packet->ReadUInt16_LE();

	if (saveSize == 0 || saveSize > SAVEDATA_MAX_SIZE) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData, but its saveSize is invalid: {}!\n", user->GetUserLogName(), saveSize));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SaveData - saveSize: {}\n", user->GetUserLogName(), saveSize));

	const vector<unsigned char>& saveData = packet->ReadArray_UInt8(saveSize);
	gameMatch->SetSaveData(saveData);
}

void Packet_HostManager::parsePacket_Host_SendUserStatus(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom && user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but it's not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	User* targetUser = userManager.GetUserByUserID(userID);
	if (!targetUser) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but targetUser is not logged in!\n", user->GetUserLogName()));
		return;
	}

	if (targetUser->GetUserStatus() != UserStatus::InRoom && targetUser->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but targetUser is not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	if (roomManager.GetRoomByRoomID(targetUser->GetCurrentRoomID()) != room) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but targetUser is not in the same room!\n", user->GetUserLogName()));
		return;
	}

	unsigned char status = packet->ReadUInt8();

	if (status != 1) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but status is invalid: {}!\n", user->GetUserLogName(), status));
		return;
	}

	if (gameMatch->GetGameMatchUserByUser(targetUser)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus, but targetUser already exists in gameMatch!\n", user->GetUserLogName(), status));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SendUserStatus - userID: {}, status: {}\n", user->GetUserLogName(), userID, status));

	GameMatchUser* gameMatchUser = new GameMatchUser(targetUser);
	if (!gameMatch->AddGameMatchUser(gameMatchUser)) {
		return;
	}

	targetUser->SetUserStatus(UserStatus::InGame);
}

void Packet_HostManager::parsePacket_Host_PlayerKilled(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long attackerUserID = packet->ReadUInt32_LE();

	if (attackerUserID) {
		User* targetUser = userManager.GetUserByUserID(attackerUserID);
		if (!targetUser) {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but targetUser is not logged in!\n", user->GetUserLogName()));
			return;
		}

		GameMatchUser* gameMatchUser = gameMatch->GetGameMatchUserByUser(targetUser);
		if (gameMatchUser == NULL) {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled, but gameMatchUser is NULL!\n", user->GetUserLogName()));
			return;
		}
	}

	unsigned char attackerWeaponID = packet->ReadUInt8();
	signed char attackerArmor = packet->ReadUInt8();
	signed char attackerHealth = packet->ReadUInt8();
	unsigned long victimUserID = packet->ReadUInt32_LE();
	unsigned char victimWeaponID = packet->ReadUInt8();
	signed char victimArmor = packet->ReadUInt8();
	unsigned short distance = packet->ReadUInt16_LE();

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host PlayerKilled - attackerUserID: {}, attackerWeaponID: {}, attackerArmor: {}, attackerHealth: {}, victimUserID: {}, victimWeaponID: {}, victimArmor: {}, distance: {}\n", user->GetUserLogName(), attackerUserID, attackerWeaponID, attackerArmor, attackerHealth, victimUserID, victimWeaponID, victimArmor, distance));
}

void Packet_HostManager::parsePacket_Host_EndGame(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host EndGame, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host EndGame, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host EndGame, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host EndGame, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long unk = packet->ReadUInt32_LE();

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host EndGame - unk: {}\n", user->GetUserLogName(), unk));

	vector<GameMatchUser*> gameMatchUsers = gameMatch->GetGameMatchUsers();

	gameMatchUsers.erase(remove_if(gameMatchUsers.begin(), gameMatchUsers.end(), [](GameMatchUser* gameMatchUser) {
		return (gameMatchUser->GetUser() == NULL || gameMatchUser->GetUser()->GetConnection() == NULL);
	}), gameMatchUsers.end());

	for (auto& gameMatchUser : gameMatchUsers) {
		User* u = gameMatchUser->GetUser();

		u->SetUserStatus(UserStatus::InRoom);
		sendPacket_Host_ExitGame(u->GetConnection());
		packet_RoomManager.SendPacket_Room_GameResult(u->GetConnection());
	}

	delete gameMatch;
	room->SetGameMatch(NULL);

	RoomSettings roomSettings;
	roomSettings.lowFlag = ROOMSETTINGS_LFLAG_ROOMSTATE;
	roomSettings.roomState = RoomState::Waiting;
	room->UpdateRoomSettings(roomSettings);

	roomManager.SendUpdateRoomPacketToAll(room, ROOMLIST_FLAG_ROOMSTATE);

	vector<User*> roomUsers = room->GetRoomUsers();

	roomUsers.erase(remove_if(roomUsers.begin(), roomUsers.end(), [](User* u) {
		return (u == NULL || u->GetConnection() == NULL);
	}), roomUsers.end());

	for (auto& u : roomUsers) {
		packet_RoomManager.SendPacket_Room_ReplyUpdateRoomSettings(u->GetConnection(), roomSettings);
		roomManager.SendUpdateRoomPacket(u->GetConnection(), room, ROOMLIST_FLAG_ROOMSTATE);
	}
}

void Packet_HostManager::parsePacket_Host_RejoinFail(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RejoinFail, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RejoinFail, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RejoinFail, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RejoinFail, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned char rejoinFailUserCount = packet->ReadUInt8();

	string rejoinFailUserIDs;
	for (unsigned char i = 0; i < rejoinFailUserCount; i++) {
		unsigned long rejoinFailUserID = packet->ReadUInt32_LE();
		rejoinFailUserIDs += format(" {},", rejoinFailUserID);
	}
	rejoinFailUserIDs[rejoinFailUserIDs.size() - 1] = ' ';

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RejoinFail - rejoinFailUserCount: {}, rejoinFailUserIDs:{}\n", user->GetUserLogName(), rejoinFailUserCount, rejoinFailUserIDs));
}

void Packet_HostManager::parsePacket_Host_AddKill(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	if (userID) {
		User* targetUser = userManager.GetUserByUserID(userID);
		if (!targetUser) {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but targetUser is not logged in!\n", user->GetUserLogName()));
			return;
		}

		GameMatchUser* gameMatchUser = gameMatch->GetGameMatchUserByUser(targetUser);
		if (gameMatchUser == NULL) {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill, but gameMatchUser is NULL!\n", user->GetUserLogName()));
			return;
		}
	}

	signed char point = packet->ReadUInt8();
	signed short frags = packet->ReadUInt16_LE();

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddKill - userID: {}, point: {}, frags: {}\n", user->GetUserLogName(), userID, point, frags));
}

void Packet_HostManager::parsePacket_Host_AddDeath(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	User* targetUser = userManager.GetUserByUserID(userID);
	if (!targetUser) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but targetUser is not logged in!\n", user->GetUserLogName()));
		return;
	}

	GameMatchUser* gameMatchUser = gameMatch->GetGameMatchUserByUser(targetUser);
	if (gameMatchUser == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath, but gameMatchUser is NULL!\n", user->GetUserLogName()));
		return;
	}

	signed char point = packet->ReadUInt8();
	signed short deaths = packet->ReadUInt16_LE();

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host AddDeath - userID: {}, deathCount: {}, totalDeaths: {}\n", user->GetUserLogName(), userID, point, deaths));
}

void Packet_HostManager::parsePacket_Host_RoundResult(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RoundResult, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RoundResult, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RoundResult, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RoundResult, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned char winTeam = packet->ReadUInt8();
	unsigned char terWins = packet->ReadUInt8();
	unsigned char ctWins = packet->ReadUInt8();

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host RoundResult - winTeam: {}, terWins: {}, ctWins: {}\n", user->GetUserLogName(), winTeam, terWins, ctWins));
}

void Packet_HostManager::parsePacket_Host_SelectTeam(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but it's not in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	GameMatch* gameMatch = room->GetGameMatch();
	if (gameMatch == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but its room doesn't have a gameMatch!\n", user->GetUserLogName()));
		return;
	}

	if (room->GetRoomHostUser() != user) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but it's not the room host!\n", user->GetUserLogName()));
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	User* targetUser = userManager.GetUserByUserID(userID);
	if (!targetUser) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but targetUser is not logged in!\n", user->GetUserLogName()));
		return;
	}

	GameMatchUser* gameMatchUser = gameMatch->GetGameMatchUserByUser(targetUser);
	if (gameMatchUser == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but gameMatchUser is NULL!\n", user->GetUserLogName()));
		return;
	}

	unsigned char team = packet->ReadUInt8();

	if (team != Team::Terrorist && team != Team::CounterTerrorist) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam, but team is invalid: {}!\n", user->GetUserLogName(), team));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_HostManager ] User ({}) has sent Packet_Host SelectTeam - userID: {}, team: {}\n", user->GetUserLogName(), userID, team));

	gameMatchUser->SetTeam((Team)team);
}

void Packet_HostManager::sendPacket_Host_ExitGame(TCPConnection::pointer connection) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Host });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_HostType::ExitGame);

	packet->Send();
}