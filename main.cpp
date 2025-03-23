#include "serverconfig.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "packetmanager.h"
#include "databasemanager.h"
#include "usermanager.h"
#include "servertick.h"
#include <iostream>

BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType)
{
	switch (CtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

int main() {
	serverTick.Start();

	if (!serverConfig.Load()) {
		cout << "[ServerConfig] Failed to load server configs!\n";
		return -1;
	}

	if (!databaseManager.Init(serverConfig.sqlServer, serverConfig.sqlUser, serverConfig.sqlPassword, serverConfig.sqlDatabase)) {
		cout << "[DatabaseManager] Failed to initialize database manager!\n";
		return -1;
	}

	if (!databaseManager.AddServerChannel()) {
		cout << "[DatabaseManager] Failed to add server channel to the database!\n";
		return -1;
	}

	databaseManager.GetAllChannelsNumPlayers();

	if (!tcpServer.Init(IPV::V4, serverConfig.tcpPort)) {
		cout << "[TCPServer] Failed to initialize TCP server!\n";
		return -1;
	}

	tcpServer.Start();

	if (!udpServer.Init(IPV::V4, serverConfig.udpPort)) {
		cout << "[UDPServer] Failed to initialize UDP server!\n";
		return -1;
	}

	udpServer.Start();

	packetManager.Start();
	
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	string command;
	while (getline(cin >> ws, command)) {
		if (command == "stop") {
			break;
		}
		else if (command == "status") {
			cout << format("Connected clients: {}, connected users: {}\n", tcpServer.GetConnections().size(), userManager.GetUsers().size());
			continue;
		}
		else {
			cout << "Available commands: stop, status\n";
			continue;
		}
	}

	return 0;
}