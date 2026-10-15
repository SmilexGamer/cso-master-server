#include "serverconsole.h"
#include "tcp_server.h"
#include "usermanager.h"
#include "roommanager.h"
#include "databasemanager.h"
#include "udp_server.h"
#include <iostream>
#include <mutex>

ServerConsole serverConsole;
static mutex consoleMutex;

ServerConsole::ServerConsole() {
	_running = false;
	_currentTime = 0;
	_currentTimeStr = "";
	_secondCount = 0;
	_consoleOutput = INVALID_HANDLE_VALUE;
	_logFile = "";
}

ServerConsole::~ServerConsole() {
	shutdown();
}

bool ServerConsole::Init() {
	time(&_currentTime);

	char time_buf[28];
	struct tm ts;
	ts = *localtime(&_currentTime);

	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &ts);
	_currentTimeStr = time_buf;

	strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &ts);
	_logFile = format("logs/serverlog_{}.log", time_buf);

	_consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (_consoleOutput == INVALID_HANDLE_VALUE) {
		Print(PrefixType::Error, "[ ServerConsole ] Failed to get console output handle!\n");
		return false;
	}

	return true;
}

void ServerConsole::Start() {
	if (_serverConsoleThread.joinable()) {
		Print(PrefixType::Warn, "[ ServerConsole ] Thread is already running!\n");
		return;
	}

	Print(PrefixType::Info, "[ ServerConsole ] Starting!\n");

	_serverConsoleThread = thread(&ServerConsole::run, this);
}

void ServerConsole::Stop() {
	if (!_serverConsoleThread.joinable()) {
		Print(PrefixType::Warn, "[ ServerConsole ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

void ServerConsole::Log(PrefixType prefixType, const string& text) {
	string prefix;

	switch (prefixType) {
		case Info: {
			prefix = INFO_PREFIX;
			break;
		}
		case Warn: {
			prefix = WARN_PREFIX;
			break;
		}
		case Error: {
			prefix = ERROR_PREFIX;
			break;
		}
		case Fatal: {
			prefix = FATAL_PREFIX;
			break;
		}
		case Debug: {
			prefix = DEBUG_PREFIX;
			break;
		}
		default: {
			break;
		}
	}

	log(format("[ {} ]{}{}", _currentTimeStr, prefix, text));
}

void ServerConsole::Print(PrefixType prefixType, const string& text, bool logToFile) {
	string output;
	{
		lock_guard<mutex> lock(consoleMutex);

		string prefix;

		switch (prefixType) {
			case Info: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
				prefix = INFO_PREFIX;
				break;
			}
			case Warn: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				prefix = WARN_PREFIX;
				break;
			}
			case Error: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED | FOREGROUND_INTENSITY);
				prefix = ERROR_PREFIX;
				break;
			}
			case Fatal: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED);
				prefix = FATAL_PREFIX;
				break;
			}
			case Debug: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_INTENSITY);
				prefix = DEBUG_PREFIX;
				break;
			}
			default: {
				SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
				break;
			}
		}

		output = format("[ {} ]{}{}", _currentTimeStr, prefix, text);

		cout << output;

		SetConsoleTextAttribute(_consoleOutput, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	}

	if (logToFile) {
		log(output);
	}
}

void ServerConsole::StartRead() {
	string command;
	while (getline(cin >> ws, command)) {
		if (command == "stop") {
			break;
		}
		else if (command == "status") {
			size_t connectionsCount = tcpServer.GetConnections().size();
			size_t usersCount = userManager.GetUsers().size();

			Print(PrefixType::Info, format("[ ServerConsole ] Clients: {} ({} logged-in, {} not logged-in), rooms: {}\n", connectionsCount, usersCount, connectionsCount - usersCount, roomManager.GetRooms().size()));
			continue;
		}
		else if (command == "clients") {
			auto connections = tcpServer.GetConnections();
			size_t usersCount = userManager.GetUsers().size();

			Print(PrefixType::Info, format("[ ServerConsole ] Clients: {} ({} logged-in, {} not logged-in)\n", connections.size(), usersCount, connections.size() - usersCount));

			if (connections.size()) {
				Print(PrefixType::Info, "[ ServerConsole ] UserID, UserName, IP, Status\n");
			}

			for (auto& connection : connections) {
				User* user = userManager.GetUserByConnection(connection);
				Print(PrefixType::Info, user ? format("[ ServerConsole ] {}, {}, {}, {}\n", user->GetUserID(), user->GetUserName(), user->GetUserIPAddress(), (unsigned char)user->GetUserStatus()) : format("[ ServerConsole ] 0, Not logged-in, {}, -1\n", connection->GetIPAddress()));
			}
			continue;
		}
		else if (command == "disconnectallclients") {
			auto connections = tcpServer.GetConnections();
			if (connections.size()) {
				if (roomManager.GetRooms().size()) {
					roomManager.RemoveAllRooms();
					databaseManager.RemoveAllRooms();
				}

				for (auto& connection : connections) {
					if (userManager.GetUserByConnection(connection) == NULL) {
						connection->DisconnectClient(true);
					}
				}

				userManager.DisconnectAllUsers();
			}

			Print(PrefixType::Info, connections.size() ? format("[ ServerConsole ] Disconnected {} client{}\n", connections.size(), connections.size() > 1 ? "s" : "") : "[ ServerConsole ] There are no clients to disconnect\n");
			continue;
		}
		else if (command == "removeallrooms") {
			size_t roomsCount = roomManager.GetRooms().size();
			if (roomsCount) {
				roomManager.RemoveAllRooms();
				databaseManager.RemoveAllRooms();
			}

			Print(PrefixType::Info, roomsCount ? format("[ ServerConsole ] Removed {} room{}\n", roomsCount, roomsCount > 1 ? "s" : "") : "[ ServerConsole ] There are no rooms to remove\n");
			continue;
		}
		else {
			Print(PrefixType::Info, "[ ServerConsole ] Available commands: stop, status, clients, disconectallclients, removeallrooms\n");
			continue;
		}
	}
}

int ServerConsole::run() {
	Print(PrefixType::Info, "[ ServerConsole ] Thread starting!\n");

	try {
		_running = true;
		while (_running) {
			this_thread::sleep_for(1s);

			onSecondTick();
		}
	}
	catch (exception& e) {
		Print(PrefixType::Error, format("[ ServerConsole ] Error on run: {}\n", e.what()));
		return -1;
	}

	Print(PrefixType::Info, "[ ServerConsole ] Thread shutting down!\n");
	return 0;
}

int ServerConsole::shutdown() {
	try {
		if (_serverConsoleThread.joinable()) {
			Print(PrefixType::Info, "[ ServerConsole ] Shutting down!\n");

			_running = false;
			_serverConsoleThread.detach();
		}
	}
	catch (exception& e) {
		Print(PrefixType::Error, format("[ ServerConsole ] Error on shutdown: {}\n", e.what()));
		return -1;
	}

	return 0;
}

void ServerConsole::onSecondTick() {
	time(&_currentTime);

	static char time_buf[28];
	static struct tm ts;
	ts = *localtime(&_currentTime);

	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &ts);
	_currentTimeStr = time_buf;

	udpServer.OnSecondTick();

	_secondCount++;

	if (_secondCount == 60) {
		onMinuteTick();
		_secondCount = 0;
	}
}

void ServerConsole::onMinuteTick() {
	userManager.OnMinuteTick();
}

void ServerConsole::log(const string& text) {
	CreateDirectoryA("logs", NULL);

	FILE* file = fopen(_logFile.c_str(), "a");
	if (!file) {
		Print(PrefixType::Warn, format("[ ServerConsole ] Can't open {} to write server log!\n", _logFile), false);
		return;
	}

	fwrite(text.c_str(), text.size(), 1, file);
	fclose(file);
}