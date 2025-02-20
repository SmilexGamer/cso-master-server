#pragma once
#include "tcp_connection.h"
#include <functional>
#include <optional>
#include <unordered_set>
#include <thread>

enum class IPV {
	V4,
	V6
};

class TCPServer {
	using OnConnectHandler = function<void(TCPConnection::pointer)>;
	using OnDisconnectHandler = function<void(TCPConnection::pointer)>;
	using OnClientPacketHandler = function<void(TCPConnection::Packet::pointer)>;

public:
	TCPServer(IPV ipv, int port);
	~TCPServer();

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
	std::string get_password() const
	{
		return "-?H2byVtl";
	}

	IPV _ipVersion;
	unsigned short _port;

	thread _serverThread;
	boost::asio::io_context _ioContext;
	boost::asio::ssl::context _sslContext;
	boost::asio::ip::tcp::acceptor _acceptor;
	optional<boost::asio::ip::tcp::socket> _socket;
	unordered_set<TCPConnection::pointer> _connections {};
};