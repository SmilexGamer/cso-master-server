#pragma once
#include "tcp_connection.h"
#include <functional>
#include <optional>
#include <unordered_set>
#include <thread>

namespace io = boost::asio;

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
	IPV _ipVersion;
	int _port;

	thread _serverThread;
	io::io_context _ioContext;
	io::ip::tcp::acceptor _acceptor;
	optional<io::ip::tcp::socket> _socket;
	unordered_set<TCPConnection::pointer> _connections {};
};