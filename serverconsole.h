#pragma once
#include <thread>
#include <string>

using namespace std;

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

typedef void* HANDLE;

enum PrefixType {
	NoPrefix = 0,
	Info = 1,
	Warn = 2,
	Error = 3,
	Fatal = 4,
	Debug = 5
};

#define INFO_PREFIX "[ Info ]"
#define WARN_PREFIX "[ Warn ]"
#define ERROR_PREFIX "[ Error ]"
#define FATAL_PREFIX "[ Fatal ]"
#define DEBUG_PREFIX "[ Debug ]"

class ServerConsole {
public:
	ServerConsole();
	~ServerConsole();

	bool Init();
	void Start();
	void Stop();
	void Log(PrefixType prefixType, const string& text);
	void Print(PrefixType prefixType, const string& text, bool logToFile = true);
	void StartRead();

	unsigned long long GetCurrentTime() const noexcept {
		return _currentTime;
	}
private:
	int run();
	int shutdown();
	void onSecondTick();
	void onMinuteTick();
	void log(const string& text);

private:
	thread _serverConsoleThread;
	bool _running;
	time_t _currentTime;
	string _currentTimeStr;
	char _secondCount;
	HANDLE _consoleOutput;
	string _logFile;
};

extern ServerConsole serverConsole;