#pragma once
#include "mysql/mysql.h"
#include <string>

using namespace std;

class DatabaseManager {
public:
	DatabaseManager();
	~DatabaseManager();

	bool Init(string server, string user, string password, string database);
	bool AddServerChannel();
	void RemoveServerChannel();
	void UpdateChannelNumPlayers(unsigned short numPlayers);
	void GetChannelsNumPlayers();
	bool Login(string userName, string password);

private:
	MYSQL* _connection;
};

extern DatabaseManager databaseManager;