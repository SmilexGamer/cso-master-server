#include "tcp_server.h"
#include <iostream>

using boost::asio::ip::tcp;
TCPServer::TCPServer(IPV ipv, int port) : _ipVersion(ipv), _port(port), 
	_acceptor(_ioContext, tcp::endpoint(_ipVersion == IPV::V4 ? tcp::v4() : tcp::v6(), _port)) {}

TCPServer::~TCPServer() {
	shutdown();
}

void TCPServer::Start() {
	if (_serverThread.joinable()) {
		std::cout << "[TCPServer] Thread is already running!" << std::endl;
		return;
	}

	std::cout << "[TCPServer] Starting!" << std::endl;

	_connections.clear();
	_serverThread = std::thread(&TCPServer::run, this);
}

void TCPServer::Stop() {
	if (!_serverThread.joinable()) {
		std::cout << "[TCPServer] Thread is already shut down!" << std::endl;
		return;
	}

	shutdown();
}

int TCPServer::run() {
	std::cout << "[TCPServer] Thread starting!" << std::endl;

	try {
		startAccept();
		_ioContext.run();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	std::cout << "[TCPServer] Thread shutting down!" << std::endl;
	return 0;
}

int TCPServer::shutdown() {
	try {
		if (_serverThread.joinable()) {
			std::cout << "[TCPServer] Shutting down!" << std::endl;

			_ioContext.stop();
			_serverThread.detach();
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}

void TCPServer::startAccept() {
	_socket.emplace(_ioContext);

	// asynchronously accept the connection
	_acceptor.async_accept(*_socket, [this](const boost::system::error_code& error) {
		if (!error) {
			auto connection = TCPConnection::Create(std::move(*_socket));

			_connections.insert(connection);

			connection->Start(
				[this](TCPConnection::Packet::pointer packet) { if (OnClientPacket) OnClientPacket(packet); },
				[&, weak = std::weak_ptr(connection)]{
					if (auto shared = weak.lock(); shared && _connections.erase(shared)) {
						if (OnDisconnect) OnDisconnect(shared);
					}
				}
			);

			// Tell the client that it has successfully connected to the server
			connection->WritePacket(std::vector<unsigned char>(connection->WelcomeMessage.begin(), connection->WelcomeMessage.end()));

			if (OnConnect) {
				OnConnect(connection);
			}
		}

		startAccept();
	});
}