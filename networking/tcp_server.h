#pragma once
#include "tcp_connection.h"
#include <functional>
#include <optional>
#include <unordered_set>
#include <thread>

class TCPServer {
	using OnConnectHandler = function<void(TCPConnection::pointer)>;
	using OnDisconnectHandler = function<void(TCPConnection::pointer)>;
	using OnClientPacketHandler = function<void(TCPConnection::Packet::pointer)>;

public:
	TCPServer();
	~TCPServer();

	unordered_set<TCPConnection::pointer> GetConnections() {
		return _connections;
	}

	bool Init(IPV ipv, unsigned short port);
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
	string get_password() const {
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