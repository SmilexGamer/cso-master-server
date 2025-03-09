#include "servertick.h"
#include "usermanager.h"
#include <iostream>
#include <format>

ServerTick serverTick;

ServerTick::ServerTick() {
	_running = false;
	_secondCount = 0;
}

ServerTick::~ServerTick() {
	shutdown();
}

void ServerTick::Start() {
	if (_serverTickThread.joinable()) {
		cout << "[ServerTick] Thread is already running!\n";
		return;
	}

	_serverTickThread = thread(&ServerTick::run, this);
}

void ServerTick::Stop() {
	if (!_serverTickThread.joinable()) {
		cout << "[ServerTick] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

int ServerTick::run() {
	cout << "[ServerTick] Thread starting!\n";

	try {
		_running = true;
		while (_running) {
			this_thread::sleep_for(1s);

			onSecondTick();
		}
	}
	catch (exception& e) {
		cerr << format("[ServerTick] Error on run: {}\n", e.what());
		return -1;
	}

	cout << "[ServerTick] Thread shutting down!\n";
	return 0;
}

int ServerTick::shutdown() {
	try {
		if (_serverTickThread.joinable()) {
			cout << "[ServerTick] Shutting down!\n";

			_running = false;
			_serverTickThread.detach();
		}
	}
	catch (exception& e) {
		cerr << format("[ServerTick] Error on shutdown: {}\n", e.what());
		return -1;
	}

	return 0;
}

void ServerTick::onSecondTick() {
	_secondCount++;

	if (_secondCount == 60) {
		onMinuteTick();
		_secondCount = 0;
	}
}

void ServerTick::onMinuteTick() {
	userManager.OnMinuteTick();
}