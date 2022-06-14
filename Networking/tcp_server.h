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
	using OnConnectHandler = std::function<void(TCPConnection::pointer)>;
	using OnDisconnectHandler = std::function<void(TCPConnection::pointer)>;
	using OnClientPacketHandler = std::function<void(TCPConnection::Packet::pointer)>;

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

	std::thread _serverThread;
	io::io_context _ioContext;
	io::ip::tcp::acceptor _acceptor;
	std::optional<io::ip::tcp::socket> _socket;
	std::unordered_set<TCPConnection::pointer> _connections {};
};