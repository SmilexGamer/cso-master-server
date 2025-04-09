#include "serverconfig.h"
#include "serverconsole.h"
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

ServerConfig serverConfig;

ServerConfig::ServerConfig() {
	port = 0;
	maxPlayers = 0;
	serverID = 0;
	channelID = 0;
	sqlServer = "";
	sqlUser = "";
	sqlPassword = "";
	sqlDatabase = "";
	decryptCipherMethod = CipherMethod::CleanUp;
	encryptCipherMethod = CipherMethod::CleanUp;
}

string defaultServerConfig = R"({
	"Port": 8001,
	"MaxPlayers": 600,
	"ServerID": 1,
	"ChannelID": 1,
	"ServerList": [
		{
			"Type": 0,
			"Name": "Server 1",
			"Channels": [
				{
					"Name": "Channel 1-1"
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
	"ProhibitedNames": [],
	"DecryptCipherMethod": 1,
	"EncryptCipherMethod": 1
})";

bool ServerConfig::Load() {
	try {
		ifstream f("configs\\serverconfig.json");
		json config;

		if (f.fail()) {
			config = json::parse(defaultServerConfig);
			serverConsole.Print(PrefixType::Warn, format("[ ServerConfig ] Couldn't open serverconfig.json, using default values!\n"));
		}
		else {
			config = json::parse(f);
		}

		if (config.contains("Port")) {
			port = config.value("Port", 8001);
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
				s.status = (ServerStatus)(s.id == this->serverID);
				s.type = (ServerType)server.value("Type", 0);
				s.name = server.value("Name", format("Server {}", s.id));

				if (server.contains("Channels")) {
					json channels = server["Channels"];

					unsigned char channelID = 1;

					for (auto& channel : channels) {
						Channel ch;
						ch.id = channelID++;
						ch.name = channel.value("Name", format("Channel {}-{}", s.id, ch.id));
						ch.numPlayers = 0;

						if (s.id != this->serverID || s.id == this->serverID && ch.id != this->channelID) {
							ch.maxPlayers = channel.value("MaxPlayers", 600);
							ch.ip = channel.value("IP", "127.0.0.1");
							ch.port = channel.value("Port", 8001);
						}

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
		if (config.contains("DecryptCipherMethod")) {
			unsigned char decryptCipherMethod = config.value("DecryptCipherMethod", 1);

			if (decryptCipherMethod == 1 || decryptCipherMethod == 2) {
				this->decryptCipherMethod = (CipherMethod)(decryptCipherMethod + 1);
			}
			else {
				this->decryptCipherMethod = CipherMethod::Null;
			}
		}
		if (config.contains("EncryptCipherMethod")) {
			unsigned char encryptCipherMethod = config.value("EncryptCipherMethod", 1);

			if (encryptCipherMethod == 1 || encryptCipherMethod == 2) {
				this->encryptCipherMethod = (CipherMethod)(encryptCipherMethod + 1);
			}
			else {
				this->encryptCipherMethod = CipherMethod::Null;
			}
		}

		serverConsole.Print(PrefixType::Info, "[ ServerConfig ] Loaded server configs!\n");
	}
	catch (json::parse_error& e) {
		serverConsole.Print(PrefixType::Error, format("[ ServerConfig ] Error in parsing serverconfig.json: {}\n", e.what()));
		return false;
	}

	return true;
}