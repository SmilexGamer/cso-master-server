#include "udp_server.h"
#include "usermanager.h"
#include <iostream>

UDPServer udpServer;

UDPServer::UDPServer() : _socket(_ioService, NULL) {
	_port = 0;
	_readOffset = 0;
	_recvBuffer = {};
}

UDPServer::~UDPServer() {
	shutdown();
}

bool UDPServer::Init(IPV ipv, unsigned short port) {
	try {
		_port = port;
		_socket = boost::asio::ip::udp::socket(_ioService, boost::asio::ip::udp::endpoint(ipv == IPV::V4 ? boost::asio::ip::udp::v4() : boost::asio::ip::udp::v6(), _port));
	}
	catch (exception& e) {
		cerr << format("[UDPServer] Error on Init: {}\n", e.what());
		return false;
	}

	return true;
}

void UDPServer::Start() {
	if (_udpServerThread.joinable()) {
		cout << "[UDPServer] Thread is already running!\n";
		return;
	}

	cout << format("[UDPServer] Starting on port {}!\n", _port);

	_udpServerThread = thread(&UDPServer::run, this);
}

void UDPServer::Stop() {
	if (!_udpServerThread.joinable()) {
		cout << "[UDPServer] Thread is already shut down!\n";
		return;
	}

	shutdown();
}

int UDPServer::run() {
	cout << "[UDPServer] Thread starting!\n";

	try {
		startReceive();
		_ioService.run();
	}
	catch (exception& e) {
		cerr << format("[UDPServer] Error on run: {}\n", e.what());
		return -1;
	}

	cout << "[UDPServer] Thread shutting down!\n";
	return 0;
}

int UDPServer::shutdown() {
	try {
		if (_udpServerThread.joinable()) {
			cout << "[UDPServer] Shutting down!\n";

			_ioService.stop();
			_udpServerThread.detach();
		}
	}
	catch (exception& e) {
		cerr << format("[UDPServer] Error on shutdown: {}\n", e.what());
		return -1;
	}

	return 0;
}

void UDPServer::startReceive() {
	_socket.async_receive_from(boost::asio::buffer(_recvBuffer), _endpoint, boost::bind(&UDPServer::handleReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UDPServer::handleReceive(const boost::system::error_code& ec, size_t bytes_transferred) {
	if (!ec || ec == boost::asio::error::message_size) {
		_readOffset = 0;

		if (ReadUInt8() != UDP_PACKET_SIGNATURE) {
			cout << format("[UDPServer] Client ({}) sent UDP packet with invalid signature\n", _endpoint.address().to_string());
			startReceive();
			return;
		}

		unsigned long userID = ReadUInt32_LE();

		User* user = userManager.GetUserByUserID(userID);
		if (user == NULL) {
			cout << format("[UDPServer] Client ({}) sent UDP packet, but it's not logged in\n", _endpoint.address().to_string());
			startReceive();
			return;
		}

		if (_endpoint.address().to_string() != user->GetUserIPAddress()) {
			cout << format("[UDPServer] Client ({}) sent UDP packet, but its IP address doesn't match the user's IP address: {}\n", _endpoint.address().to_string(), user->GetUserIPAddress());
			startReceive();
			return;
		}

		unsigned char type = ReadUInt8();

		switch (type) {
			case 0: {
				unsigned char portType = ReadUInt8();
				unsigned long localIP = ~ReadUInt32_LE();
				unsigned short localPort = ReadUInt16_LE();
				unsigned char retryNum = ReadUInt8();

				cout << format("[UDPServer] Client ({}) sent UDP packet - userID: {}, type: {}, portType: {}, localIP: {}.{}.{}.{}, localPort: {}, retryNum: {}\n", _endpoint.address().to_string(), userID, type, portType, (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), localPort, retryNum);

				user->SetUserNetwork(portType, localIP, localPort, _endpoint.port());

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_PACKET_SIGNATURE);
				message->push_back(0);
				message->push_back(1);

				_socket.async_send_to(boost::asio::buffer(*message), _endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

		#ifdef _DEBUG
				string buffer;
				for (auto& c : *message) {
					buffer += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
				}

				cout << format("[UDPServer] Sent UDP packet to client ({}):{}\n", _endpoint.address().to_string(), buffer);
		#endif
				break;
			}
			case 1: {
				unsigned char retryNum = ReadUInt8();

		#ifdef _DEBUG
				cout << format("[UDPServer] Client ({}) sent UDP packet - userID: {}, type: {}, retryNum: {}\n", _endpoint.address().to_string(), userID, type, retryNum);
		#endif
				startReceive();
				break;
			}
			default: {
				cout << format("[UDPServer] Client ({}) sent UDP packet with invalid signature\n", _endpoint.address().to_string());
				startReceive();
				break;
			}
		}
	}
	else {
		startReceive();
	}
}

void UDPServer::handleSend(shared_ptr<vector<unsigned char>> message, const boost::system::error_code& ec, size_t bytes_transferred) {
	startReceive();
}