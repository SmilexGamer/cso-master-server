#include "udp_server.h"
#include "usermanager.h"
#include "serverconsole.h"

UDPServer udpServer;

UDPServer::UDPServer() : _socket(_ioContext, NULL) {
	_port = 0;
	_readOffset = 0;
	_recvBuffer = {};
}

UDPServer::~UDPServer() {
	shutdown();
}

bool UDPServer::Init(unsigned short port) {
	try {
		_port = port;
		_socket = boost::asio::ip::udp::socket(_ioContext, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), _port));
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ UDPServer ] Error on Init: {}\n", e.what()));
		return false;
	}

	return true;
}

void UDPServer::Start() {
	if (_udpServerThread.joinable()) {
		serverConsole.Print(PrefixType::Warn, "[ UDPServer ] Thread is already running!\n");
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ UDPServer ] Starting on port {}!\n", _port));

	_udpServerThread = thread(&UDPServer::run, this);
}

void UDPServer::Stop() {
	if (!_udpServerThread.joinable()) {
		serverConsole.Print(PrefixType::Warn, "[ UDPServer ] Thread is already shut down!\n");
		return;
	}

	shutdown();
}

int UDPServer::run() {
	serverConsole.Print(PrefixType::Info, "[ UDPServer ] Thread starting!\n");

	try {
		startReceive();
		_ioContext.run();
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ UDPServer ] Error on run: {}\n", e.what()));
		return -1;
	}

	serverConsole.Print(PrefixType::Info, "[ UDPServer ] Thread shutting down!\n");
	return 0;
}

int UDPServer::shutdown() {
	try {
		if (_udpServerThread.joinable()) {
			serverConsole.Print(PrefixType::Info, "[ UDPServer ] Shutting down!\n");

			_ioContext.stop();
			_udpServerThread.detach();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ UDPServer ] Error on shutdown: {}\n", e.what()));
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
#ifdef _DEBUG
			serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Client ({}) sent UDP packet with invalid signature!\n", _endpoint.address().to_string()));
#endif
			startReceive();
			return;
		}

		unsigned long userID = ReadUInt32_LE();

		User* user = userManager.GetUserByUserID(userID);
		if (!userManager.IsUserLoggedIn(user)) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but it's not logged in!\n", _endpoint.address().to_string()));
			startReceive();
			return;
		}

		if (_endpoint.address().to_string() != user->GetUserIPAddress()) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but its IP address doesn't match the user's IP address: {}!\n", _endpoint.address().to_string(), user->GetUserIPAddress()));
			startReceive();
			return;
		}

		unsigned char type = ReadUInt8();

#ifdef _DEBUG
		vector<unsigned char> buffer(_recvBuffer.begin(), _recvBuffer.begin() + bytes_transferred);
		string bufferStr;
		for (auto& c : buffer) {
			bufferStr += format(" {}{:X}", c < 0x10 ? "0" : "", c);
		}

		if (type == 1) {
			serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Received UDP packet from user ({}):{}\n", user->GetUserLogName(), bufferStr));
		}
		else {
			serverConsole.Print(PrefixType::Debug, format("[ UDPServer ] Received UDP packet from user ({}):{}\n", user->GetUserLogName(), bufferStr));
		}
#endif

		switch (type) {
			case 0: {
				unsigned char portType = ReadUInt8();
				unsigned long localIP = ~ReadUInt32_LE();
				unsigned short localPort = ReadUInt16_LE();
				unsigned char retryNum = ReadUInt8();

				serverConsole.Print(PrefixType::Info, format("[ UDPServer ] User ({}) sent UDP packet - userID: {}, type: {}, portType: {}, localIP: {}.{}.{}.{}, localPort: {}, retryNum: {}, externalPort: {}\n", user->GetUserLogName(), userID, type, portType, (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), localPort, retryNum, _endpoint.port()));

				user->SetUserNetwork((PortType)portType, localIP, localPort, _endpoint.port());

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_PACKET_SIGNATURE);
				message->push_back(0);
				message->push_back(1);

				_socket.async_send_to(boost::asio::buffer(*message), _endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

#ifdef _DEBUG
				string buffer;
				for (auto& c : *message) {
					buffer += format(" {}{:X}", c < 0x10 ? "0" : "", c);
				}

				serverConsole.Print(PrefixType::Debug, format("[ UDPServer ] Sent UDP packet to user ({}):{}\n", user->GetUserLogName(), buffer));
#endif
				break;
			}
			case 1: {
				unsigned char retryNum = ReadUInt8();

#ifdef _DEBUG
				serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] User ({}) sent UDP packet - userID: {}, type: {}, retryNum: {}, externalPort: {}\n", user->GetUserLogName(), userID, type, retryNum, _endpoint.port()));
#endif
				startReceive();
				break;
			}
			default: {
				serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent unregistered UDP packet type {}!\n", user->GetUserLogName(), type));
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