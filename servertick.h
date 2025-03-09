#pragma once
#include <thread>

using namespace std;

class ServerTick {
public:
	ServerTick();
	~ServerTick();

	void Start();
	void Stop();
private:
	int run();
	int shutdown();
	void onSecondTick();
	void onMinuteTick();

	thread _serverTickThread;
	bool _running;
	char _secondCount;
};

extern ServerTick serverTick;