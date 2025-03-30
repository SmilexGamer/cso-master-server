#include "serverconfig.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "packetmanager.h"
#include "databasemanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "servertick.h"
#include "serverconsole.h"

BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType) {
	switch (CtrlType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT: {
			serverConsole.Stop();
			serverTick.Stop();
			roomManager.RemoveAllRooms();
			userManager.RemoveAllUsers();
			packetManager.Stop();
			databaseManager.Shutdown();
			tcpServer.Stop();
			udpServer.Stop();
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
	serverTick.Start();

	if (!serverConfig.Load()) {
		serverConsole.Print(PrintType::Fatal, "[ ServerConfig ] Failed to load server configs!\n");
		return -1;
	}

	if (!databaseManager.Init(serverConfig.sqlServer, serverConfig.sqlUser, serverConfig.sqlPassword, serverConfig.sqlDatabase)) {
		serverConsole.Print(PrintType::Fatal, "[ DatabaseManager ] Failed to initialize database manager!\n");
		return -1;
	}

	if (!databaseManager.AddServerChannel()) {
		serverConsole.Print(PrintType::Fatal, "[ DatabaseManager ] Failed to add server channel to the database!\n");
		return -1;
	}

	databaseManager.GetAllChannelsNumPlayers();

	if (!tcpServer.Init(serverConfig.port)) {
		serverConsole.Print(PrintType::Fatal, "[ TCPServer ] Failed to initialize TCP server!\n");
		return -1;
	}

	tcpServer.Start();

	if (!udpServer.Init(serverConfig.port)) {
		serverConsole.Print(PrintType::Fatal, "[ UDPServer ] Failed to initialize UDP server!\n");
		return -1;
	}

	udpServer.Start();

	packetManager.Start();
	
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	serverConsole.Start();

	return 0;
}