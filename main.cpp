#include "tcp_server.h"
#include "PacketManager.h"
#include <iostream>

int main() {
	TCPServer server{IPV::V4, 30002};
	PacketManager packetManager;

	server.OnConnect = [](TCPConnection::pointer connection) {
		cout << format("[TCPServer] Client ({}) has connected to the server\n", connection->GetEndPoint());
	};

	server.OnDisconnect = [](TCPConnection::pointer connection) {
		cout << format("[TCPServer] Client ({}) has disconnected from the server\n", connection->GetEndPoint());
	};

	server.OnClientPacket = [&packetManager](TCPConnection::Packet::pointer packet) {
		packetManager.QueuePacket(packet);
	};

	server.Start();
	packetManager.Start();
	packetManager.packet_VersionManager.Start();
	packetManager.packet_LoginManager.Start();
	packetManager.packet_UMsgManager.Start();
	
	cin.get();
	return 0;
}