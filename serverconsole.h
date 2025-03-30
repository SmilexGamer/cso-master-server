#pragma once
#include <string>

using namespace std;

typedef void* HANDLE;

enum PrintType {
	NoType = 0,
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

	void Start();
	void Stop();
	void Log(const string& text);
	void Log(PrintType printType, const string& text);
	void Print(PrintType printType, const string& text, bool logToFile = true);
private:
	int run();
	int shutdown();

private:
	HANDLE _console;
	string _logFile;
	bool _running;
};

extern ServerConsole serverConsole;