#include "tcp_server.h"
#include "packetmanager.h"
#include "serverconsole.h"

TCPServer tcpServer;

TCPServer::TCPServer() : _sslContext(boost::asio::ssl::context::sslv23), _acceptor(_ioContext, NULL) {
	_port = 0;

	OnConnect = [](TCPConnection::pointer connection) {
		serverConsole.Print(PrefixType::Info, format("[ TCPServer ] Client ({}) has connected to the server\n", connection->GetIPAddress()));
	};

	OnDisconnect = [](TCPConnection::pointer connection) {
		serverConsole.Print(PrefixType::Info, format("[ TCPServer ] Client ({}) has disconnected from the server\n", connection->GetIPAddress()));
	};

	OnClientPacket = [](TCPConnection::Packet::pointer packet) {
		packetManager.QueuePacket(packet);
	};
}

TCPServer::~TCPServer() {
	shutdown();
}

bool TCPServer::Init(unsigned short port) {
	try {
		_port = port;
		_acceptor = boost::asio::ip::tcp::acceptor(_ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port));

#ifndef NO_SSL
		_sslContext.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
		_sslContext.set_password_callback(bind(&TCPServer::get_password, this));
		_sslContext.use_certificate_chain_file("certs\\cert.pem");
		_sslContext.use_private_key_file("certs\\key.pem", boost::asio::ssl::context::pem);
		_sslContext.use_tmp_dh_file("certs\\dh2048.pem");
#endif
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ TCPServer ] Error on Init: {}\n", e.what()));
		return false;
	}

	return true;
}

void TCPServer::Start() {
	if (_tcpServerThread.joinable()) {
		serverConsole.Print(PrefixType::Warn, "[ TCPServer ] Thread is already running!\n");
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ TCPServer ] Starting on port {}!\n", _port));

	_tcpServerThread = thread(&TCPServer::run, this);
}

void TCPServer::Stop() {
	if (!_tcpServerThread.joinable()) {
		serverConsole.Print(PrefixType::Warn, "[ TCPServer ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

int TCPServer::run() {
	serverConsole.Print(PrefixType::Info, "[ TCPServer ] Thread starting!\n");

	try {
		startAccept();
		_ioContext.run();
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ TCPServer ] Error on run: {}\n", e.what()));
		return -1;
	}

	serverConsole.Print(PrefixType::Info, "[ TCPServer ] Thread shutting down!\n");
	return 0;
}

int TCPServer::shutdown() {
	try {
		if (_tcpServerThread.joinable()) {
			serverConsole.Print(PrefixType::Info, "[ TCPServer ] Shutting down!\n");

			_ioContext.stop();

			for (auto& connection : _connections) {
				connection->DisconnectClient(false);
			}

			_connections.clear();
			_tcpServerThread.detach();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ TCPServer ] Error on shutdown: {}\n", e.what()));
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
				[this](TCPConnection::Packet::pointer packet) {
					if (OnClientPacket) {
						OnClientPacket(packet);
					}
				},
				[&, weak = weak_ptr(connection)](bool eraseConnection) {
					if (auto shared = weak.lock(); shared) {
						if (eraseConnection) {
							_connections.erase(shared);
						}

						if (OnDisconnect) {
							OnDisconnect(shared);
						}
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