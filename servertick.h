#pragma once
#include <thread>

using namespace std;

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

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