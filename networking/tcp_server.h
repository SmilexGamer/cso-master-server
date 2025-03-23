#pragma once
#include "tcp_connection.h"
#include <unordered_set>

class TCPServer {
	using OnConnectHandler = function<void(TCPConnection::pointer)>;
	using OnDisconnectHandler = function<void(TCPConnection::pointer)>;
	using OnClientPacketHandler = function<void(TCPConnection::Packet::pointer)>;

public:
	TCPServer();
	~TCPServer();

	const unordered_set<TCPConnection::pointer>& GetConnections() const noexcept {
		return _connections;
	}

	bool Init(unsigned short port);
	void Start();
	void Stop();
private:
	int run();
	int shutdown();
	void startAccept();

public:
	OnConnectHandler OnConnect;
	OnDisconnectHandler OnDisconnect;
	OnClientPacketHandler OnClientPacket;

private:
	const string get_password() const noexcept {
		return "-?H2byVtl";
	}

	unsigned short _port;

	thread _tcpServerThread;
	boost::asio::io_context _ioContext;
	boost::asio::ssl::context _sslContext;
	boost::asio::ip::tcp::acceptor _acceptor;
	optional<boost::asio::ip::tcp::socket> _socket;
	unordered_set<TCPConnection::pointer> _connections {};
};

extern TCPServer tcpServer;