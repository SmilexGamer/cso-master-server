#include "serverconsole.h"
#include "tcp_server.h"
#include "usermanager.h"
#include <iostream>
#include <format>
#include "servertick.h"

ServerConsole serverConsole;

ServerConsole::ServerConsole() {
	_console = GetStdHandle(STD_OUTPUT_HANDLE);
	_running = false;

	time_t t;
	time(&t);

	char time_buf[24];
	struct tm ts;
	ts = *localtime(&t);
	strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &ts);

	_logFile = format("logs/serverlog_{}.log", time_buf);
}

ServerConsole::~ServerConsole() {
	shutdown();
}

void ServerConsole::Start() {
	if (_running) {
		Print(PrintType::Warn, "[ ServerConsole ] Thread is already running!\n");
		return;
	}

	Print(PrintType::Info, "[ ServerConsole ] Starting!\n");

	run();
}

void ServerConsole::Stop() {
	if (!_running) {
		Print(PrintType::Warn, "[ ServerConsole ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

void ServerConsole::Log(const string& text) {
	CreateDirectoryA("logs", NULL);

	FILE* file = fopen(_logFile.c_str(), "a");
	if (!file) {
		Print(PrintType::Warn, format("[ ServerConsole ] Can't open {} to write server log!\n", _logFile), false);
		return;
	}

	fwrite(text.c_str(), text.size(), 1, file);
	fclose(file);
}

void ServerConsole::Log(PrintType printType, const string& text) {
	string prefix;

	switch (printType) {
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

	char time_buf[32];
	struct tm ts;
	time_t t = serverTick.GetCurrentTime();
	ts = *localtime(&t);
	strftime(time_buf, sizeof(time_buf), "[ %Y-%m-%d %H:%M:%S ]", &ts);

	Log(format("{}{}{}", time_buf, prefix, text));
}

void ServerConsole::Print(PrintType printType, const string& text, bool logToFile) {
	string prefix;

	switch (printType) {
		case Info: {
			SetConsoleTextAttribute(_console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			prefix = INFO_PREFIX;
			break;
		}
		case Warn: {
			SetConsoleTextAttribute(_console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			prefix = WARN_PREFIX;
			break;
		}
		case Error: {
			SetConsoleTextAttribute(_console, FOREGROUND_RED | FOREGROUND_INTENSITY);
			prefix = ERROR_PREFIX;
			break;
		}
		case Fatal: {
			SetConsoleTextAttribute(_console, FOREGROUND_RED);
			prefix = FATAL_PREFIX;
			break;
		}
		case Debug: {
			SetConsoleTextAttribute(_console, FOREGROUND_INTENSITY);
			prefix = DEBUG_PREFIX;
			break;
		}
		default: {
			SetConsoleTextAttribute(_console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
		}
	}

	char time_buf[32];
	struct tm ts;
	time_t t = serverTick.GetCurrentTime();
	ts = *localtime(&t);
	strftime(time_buf, sizeof(time_buf), "[ %Y-%m-%d %H:%M:%S ]", &ts);

	const string& output = format("{}{}{}", time_buf, prefix, text);

	cout << output;

	SetConsoleTextAttribute(_console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

	if (logToFile) {
		Log(output);
	}
}

int ServerConsole::run() {
	Print(PrintType::Info, "[ ServerConsole ] Thread starting!\n");

	try {
		_running = true;
		static string command;
		while (getline(cin >> ws, command)) {
			if (command == "stop") {
				break;
			}
			else if (command == "status") {
				Print(PrintType::Info, format("[ ServerConsole ] Connected clients: {}, connected users: {}\n", tcpServer.GetConnections().size(), userManager.GetUsers().size()));
				continue;
			}
			else {
				Print(PrintType::Info, "[ ServerConsole ] Available commands: stop, status\n");
				continue;
			}
		}

		shutdown();
	}
	catch (exception& e) {
		Print(PrintType::Error, format("[ ServerConsole ] Error on run: {}\n", e.what()));
		return -1;
	}

	Print(PrintType::Info, "[ ServerConsole ] Thread shutting down!\n");
	return 0;
}

int ServerConsole::shutdown() {
	try {
		if (_running) {
			Print(PrintType::Info, "[ ServerConsole ] Shutting down!\n");

			_running = false;
		}
	}
	catch (exception& e) {
		Print(PrintType::Error, format("[ ServerConsole ] Error on shutdown: {}\n", e.what()));
		return -1;
	}

	return 0;
}