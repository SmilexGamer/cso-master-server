#include "tcp_server.h"
#include "packetmanager.h"
#include <iostream>

int main() {
	TCPServer server{IPV::V4, 30002};

	server.OnConnect = [](TCPConnection::pointer connection) {
		cout << format("[TCPServer] Client ({}) has connected to the server\n", connection->GetEndPoint());
	};

	server.OnDisconnect = [](TCPConnection::pointer connection) {
		cout << format("[TCPServer] Client ({}) has disconnected from the server\n", connection->GetEndPoint());
	};

	server.OnClientPacket = [](TCPConnection::Packet::pointer packet) {
		packetManager.QueuePacket(packet);
	};

	server.Start();
	packetManager.Start();
	
	string command;
	while (getline(cin >> ws, command)) {
		if (command == "stop") {
			packetManager.Stop();
			server.Stop();
			break;
		}
		else {
			cout << "Available commands: stop\n";
			continue;
		}
	}

	return 0;
}