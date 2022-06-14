#include "tcp_server.h"
#include "PacketManager.h"
#include <iostream>

int main() {
	TCPServer server{IPV::V4, 30002};
	PacketManager packetManager;

	server.OnConnect = [](TCPConnection::pointer connection) {
		std::cout << std::format("[TCPServer] Client ({}) has connected to the server", connection->GetEndPoint()) << std::endl;
	};

	server.OnDisconnect = [](TCPConnection::pointer connection) {
		std::cout << std::format("[TCPServer] Client ({}) has disconnected from the server", connection->GetEndPoint()) << std::endl;
	};

	server.OnClientPacket = [&packetManager](TCPConnection::Packet::pointer packet) {
		packetManager.QueuePacket(packet);
	};

	server.Start();
	packetManager.Start();
	packetManager.packet_VersionManager.Start();
	packetManager.packet_LoginManager.Start();
	packetManager.packet_UMsgManager.Start();
	
	std::cin.get();
	return 0;
}