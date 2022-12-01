#include "tcp_server.h"
#include <iostream>

const string& WelcomeMessage{ "~SERVERCONNECTED\n\0" };

using boost::asio::ip::tcp;
TCPServer::TCPServer(IPV ipv, int port) : _ipVersion(ipv), _port(port), 
	_acceptor(_ioContext, tcp::endpoint(_ipVersion == IPV::V4 ? tcp::v4() : tcp::v6(), _port)) {}

TCPServer::~TCPServer() {
	shutdown();
}

void TCPServer::Start() {
	if (_serverThread.joinable()) {
		cout << "[TCPServer] Thread is already running!\n";
		return;
	}

	cout << "[TCPServer] Starting!\n";

	_connections.clear();
	_serverThread = thread(&TCPServer::run, this);
}

void TCPServer::Stop() {
	if (!_serverThread.joinable()) {
		cout << "[TCPServer] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

int TCPServer::run() {
	cout << "[TCPServer] Thread starting!\n";

	try {
		startAccept();
		_ioContext.run();
	}
	catch (exception& e) {
		cerr << e.what() << endl;
		return -1;
	}

	cout << "[TCPServer] Thread shutting down!\n";
	return 0;
}

int TCPServer::shutdown() {
	try {
		if (_serverThread.joinable()) {
			cout << "[TCPServer] Shutting down!\n";

			_ioContext.stop();
			_serverThread.detach();
		}
	}
	catch (exception& e) {
		cerr << e.what() << endl;
		return -1;
	}

	return 0;
}

void TCPServer::startAccept() {
	_socket.emplace(_ioContext);

	// asynchronously accept the connection
	_acceptor.async_accept(*_socket, [this](const boost::system::error_code& error) {
		if (!error) {
			auto connection = TCPConnection::Create(move(*_socket));

			_connections.insert(connection);

			connection->Start(
				[this](TCPConnection::Packet::pointer packet) { if (OnClientPacket) OnClientPacket(packet); },
				[&, weak = weak_ptr(connection)] {
					if (auto shared = weak.lock(); shared && _connections.erase(shared)) {
						if (OnDisconnect) OnDisconnect(shared);
					}
				}
			);

			// Tell the client that it has successfully connected to the server
			connection->WritePacket(vector<unsigned char>(WelcomeMessage.begin(), WelcomeMessage.end()));

			if (OnConnect) {
				OnConnect(connection);
			}
		}

		startAccept();
	});
}