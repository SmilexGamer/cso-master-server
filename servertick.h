#pragma once
#include <thread>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

using namespace std;

class ServerTick {
public:
	ServerTick();
	~ServerTick();

	void Start();
	void Stop();

	unsigned long long GetCurrentTime() const noexcept {
		return _currentTime;
	}
private:
	int run();
	int shutdown();
	void onSecondTick();
	void onMinuteTick();

	thread _serverTickThread;
	bool _running;
	unsigned long long _currentTime;
	char _secondCount;
};

extern ServerTick serverTick;