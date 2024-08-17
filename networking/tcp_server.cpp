#include "tcp_server.h"
#include <iostream>

TCPServer::TCPServer(IPV ipv, int port) : _ipVersion(ipv), _port(port), 
	_acceptor(_ioContext, boost::asio::ip::tcp::endpoint(_ipVersion == IPV::V4 ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), _port)),
	_sslContext(boost::asio::ssl::context::tlsv1) {
#ifndef NO_SSL
	_sslContext.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::single_dh_use);
	_sslContext.set_password_callback(std::bind(&TCPServer::get_password, this));
	_sslContext.use_certificate_chain_file("cert.pem");
	_sslContext.use_private_key_file("key.pem", boost::asio::ssl::context::pem);
	_sslContext.use_tmp_dh_file("dh2048.pem");
#endif
}

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
			auto connection = TCPConnection::Create(move(*_socket), _sslContext);

			_connections.insert(connection);

			connection->Start(
				[this](TCPConnection::Packet::pointer packet) { if (OnClientPacket) OnClientPacket(packet); },
				[&, weak = weak_ptr(connection)] {
					if (auto shared = weak.lock(); shared && _connections.erase(shared)) {
						if (OnDisconnect) OnDisconnect(shared);
					}
				}
			);

			if (OnConnect) {
				OnConnect(connection);
			}
		}

		startAccept();
	});
}