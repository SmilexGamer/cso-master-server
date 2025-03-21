#include "serverconfig.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ServerConfig serverConfig;

ServerConfig::ServerConfig() {
	tcpPort = 0;
	udpPort = 0;
	maxPlayers = 0;
	serverID = 0;
	channelID = 0;
	sqlServer = "";
	sqlUser = "";
	sqlPassword = "";
	sqlDatabase = "";
}

string defaultServerConfig = R"({
	"TCPPort": 30002,
	"UDPPort": 30002,
	"MaxPlayers": 600,
	"ServerID": 1,
	"ChannelID": 1,
	"ServerList": [
		{
			"Type": 0,
			"Name": "Server 1",
			"Channels": [
				{
					"Name": "Channel 1-1",
					"IP": "127.0.0.1",
					"Port": 30002
				}
			]
		}
	],
	"SQL": {
		"Server": "localhost",
		"User": "root",
		"Password": "",
		"Database": "csodatabase"
	},
	"ProhibitedNames": []
})";

bool ServerConfig::Load() {
	try {
		ifstream f("configs\\serverconfig.json");
		json config;

		if (f.fail()) {
			config = json::parse(defaultServerConfig);
			cout << format("[ServerConfig] Couldn't open serverconfig.json, using default values!\n");
		}
		else {
			config = json::parse(f);
		}

		if (config.contains("TCPPort")) {
			tcpPort = config.value("TCPPort", 30002);
		}
		if (config.contains("UDPPort")) {
			udpPort = config.value("UDPPort", 30002);
		}
		if (config.contains("MaxPlayers")) {
			maxPlayers = config.value("MaxPlayers", 600);
		}
		if (config.contains("ServerID")) {
			serverID = config.value("ServerID", 1);
		}
		if (config.contains("ChannelID")) {
			channelID = config.value("ChannelID", 1);
		}
		if (config.contains("ServerList")) {
			json serverList = config["ServerList"];

			unsigned char serverID = 1;

			for (auto& server : serverList) {
				Server s;
				s.id = serverID++;
				s.status = s.id == this->serverID;
				s.type = server.value("Type", 0);
				s.name = server.value("Name", format("Server {}", s.id));

				if (server.contains("Channels")) {
					json channels = server["Channels"];

					unsigned char channelID = 1;

					for (auto& channel : channels) {
						Channel ch;
						ch.id = channelID++;
						ch.name = channel.value("Name", format("Channel {}-{}", s.id, ch.id));
						ch.numPlayers = 0;
						ch.ip = channel.value("IP", "127.0.0.1");
						ch.port = channel.value("Port", 30002);

						s.channels.push_back(ch);
					}
				}

				this->serverList.push_back(s);
			}
		}
		if (config.contains("SQL")) {
			json sql = config["SQL"];

			sqlServer = sql.value("Server", "localhost");
			sqlUser = sql.value("User", "root");
			sqlPassword = sql.value("Password", "");
			sqlDatabase = sql.value("Database", "csodatabase");
		}
		if (config.contains("ProhibitedNames")) {
			prohibitedNames = config["ProhibitedNames"].get<vector<string>>();
		}

		cout << "[ServerConfig] Loaded server configs!\n";
	}
	catch (json::parse_error& e) {
		cerr << format("[ServerConfig] Error in parsing serverconfig.json: {}\n", e.what());
		return false;
	}

	return true;
}