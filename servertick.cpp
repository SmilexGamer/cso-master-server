#include "servertick.h"
#include "serverconsole.h"
#include "usermanager.h"

ServerTick serverTick;

ServerTick::ServerTick() {
	_running = false;
	time_t t;
	time(&t);
	_currentTime = t;
	_secondCount = 0;
}

ServerTick::~ServerTick() {
	shutdown();
}

void ServerTick::Start() {
	if (_serverTickThread.joinable()) {
		serverConsole.Print(PrintType::Warn, "[ ServerTick ] Thread is already running!\n");
		return;
	}

	serverConsole.Print(PrintType::Info, "[ ServerTick ] Starting!\n");

	_serverTickThread = thread(&ServerTick::run, this);
}

void ServerTick::Stop() {
	if (!_serverTickThread.joinable()) {
		serverConsole.Print(PrintType::Warn, "[ ServerTick ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

int ServerTick::run() {
	serverConsole.Print(PrintType::Info, "[ ServerTick ] Thread starting!\n");

	try {
		_running = true;
		while (_running) {
			this_thread::sleep_for(1s);

			onSecondTick();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrintType::Error, format("[ ServerTick ] Error on run: {}\n", e.what()));
		return -1;
	}

	serverConsole.Print(PrintType::Info, "[ ServerTick ] Thread shutting down!\n");
	return 0;
}

int ServerTick::shutdown() {
	try {
		if (_serverTickThread.joinable()) {
			serverConsole.Print(PrintType::Info, "[ ServerTick ] Shutting down!\n");

			_running = false;
			_serverTickThread.detach();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrintType::Error, format("[ ServerTick ] Error on shutdown: {}\n", e.what()));
		return -1;
	}

	return 0;
}

void ServerTick::onSecondTick() {
	static time_t t;
	time(&t);
	_currentTime = t;
	_secondCount++;

	if (_secondCount == 60) {
		onMinuteTick();
		_secondCount = 0;
	}
}

void ServerTick::onMinuteTick() {
	userManager.OnMinuteTick();
}