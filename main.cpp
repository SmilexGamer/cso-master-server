#include "serverconfig.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "packetmanager.h"
#include "databasemanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "serverconsole.h"

HANDLE hMutex;

BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType) {
	switch (CtrlType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT: {
			serverConsole.Stop();
			roomManager.RemoveAllRooms();
			userManager.RemoveAllUsers();
			packetManager.Stop();
			databaseManager.Shutdown();
			tcpServer.Stop();
			udpServer.Stop();
			ReleaseMutex(hMutex);
			return TRUE;
		}
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT: {
			return TRUE;
		}
		default:
			break;
	}

	return FALSE;
}

int main() {
	if (!serverConsole.Init()) {
		serverConsole.Print(PrefixType::Fatal, "[ ServerConsole ] Failed to initialize server console!\n");
		return -1;
	}

	serverConsole.Start();

	if (!serverConfig.Load()) {
		serverConsole.Print(PrefixType::Fatal, "[ ServerConfig ] Failed to load server configs!\n");
		return -1;
	}

	if (!databaseManager.Init(serverConfig.sqlServer, serverConfig.sqlUser, serverConfig.sqlPassword, serverConfig.sqlDatabase)) {
		serverConsole.Print(PrefixType::Fatal, "[ DatabaseManager ] Failed to initialize database manager!\n");
		return -1;
	}

	string mutexName = format("cso-master-server-{}-{}", serverConfig.serverID, serverConfig.channelID);
	hMutex = CreateMutexA(NULL, FALSE, mutexName.c_str());
	if (hMutex) {
		DWORD dwWaitResult = WaitForSingleObject(hMutex, 0);
		if (!dwWaitResult || dwWaitResult == WAIT_ABANDONED) {
			databaseManager.RemoveAllUserSessions();
			databaseManager.RemoveAllUserTransfers();
			databaseManager.RemoveServerChannel();
		}
	}
	else {
		serverConsole.Print(PrefixType::Fatal, "[ Main ] Failed to create mutex for server instance!\n");
		return -1;
	}

	if (!databaseManager.AddServerChannel()) {
		serverConsole.Print(PrefixType::Fatal, "[ DatabaseManager ] Failed to add server channel to the database!\n");
		return -1;
	}

	databaseManager.GetAllChannelsNumPlayers();

	if (!tcpServer.Init(serverConfig.port)) {
		serverConsole.Print(PrefixType::Fatal, "[ TCPServer ] Failed to initialize TCP server!\n");
		return -1;
	}

	tcpServer.Start();

	if (!udpServer.Init(serverConfig.port)) {
		serverConsole.Print(PrefixType::Fatal, "[ UDPServer ] Failed to initialize UDP server!\n");
		return -1;
	}

	udpServer.Start();

	packetManager.Start();
	
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	serverConsole.StartRead();

	ReleaseMutex(hMutex);

	return 0;
}