#include "udp_server.h"
#include "usermanager.h"
#include "serverconsole.h"
#include "serverconfig.h"
#include "databasemanager.h"

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

bool UDPServer::IsServerChannelOnline(unsigned char serverID, unsigned char channelID) {
	auto key = make_pair(serverID, channelID);
	auto promise = make_shared<std::promise<bool>>();
	{
		lock_guard<mutex> lock(_heartbeatMutex);
		_heartbeatPromises[key] = promise;
	}

	auto future = promise->get_future();
	if (future.wait_for(5000ms) == future_status::ready) {
		{
			lock_guard<mutex> lock(_heartbeatMutex);
			_heartbeatPromises.erase(key);
		}
		return future.get();
	}

	{
		lock_guard<mutex> lock(_heartbeatMutex);
		_heartbeatPromises.erase(key);
	}
	return false;
}

void UDPServer::SendNumPlayersPacketToAll(unsigned short numPlayers) {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port);

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_SERVER_PACKET_SIGNATURE);
				message->push_back(serverConfig.serverID);
				message->push_back(serverConfig.channelID);
				message->push_back(ServerPacketType::NUMPLAYERS);
				message->push_back((unsigned char)numPlayers);
				message->push_back(numPlayers >> 8);

				_socket.async_send_to(boost::asio::buffer(*message), endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
		}
	}
}

void UDPServer::OnSecondTick() {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				if (channel.lastHeartBeat + 3 < serverConsole.GetCurrentTime()) {
					serverConfig.serverList[server.id - 1].channels[channel.id - 1].isOnline = false;
					serverConfig.serverList[server.id - 1].channels[channel.id - 1].lastHeartBeat = 0;
					serverConfig.serverList[server.id - 1].channels[channel.id - 1].numPlayers = 0;

					databaseManager.RemoveAllUserSessions(server.id, channel.id);
					databaseManager.RemoveAllUserTransfers(server.id, channel.id);
					sendDeadChannelPacketToAll(server.id, channel.id);

					if (server.id != serverConfig.serverID) {
						serverConfig.serverList[server.id - 1].status = ServerStatus::NotReady;
						for (auto& channel : serverConfig.serverList[server.id - 1].channels) {
							if (channel.isOnline) {
								serverConfig.serverList[server.id - 1].status = ServerStatus::Ready;
								break;
							}
						}
					}
				}
				else {
					sendHeartbeatPacket(server.id, channel.id);
				}
			}
		}
	}
}

int UDPServer::run() {
	serverConsole.Print(PrefixType::Info, "[ UDPServer ] Thread starting!\n");

	try {
		startReceive();
		sendHelloPacketToAll();
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

			sendGoodbytePacketToAll();
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

		unsigned char signature = ReadUInt8();

		switch (signature) {
			case UDP_CLIENT_PACKET_SIGNATURE: {
				handleClientPacket(bytes_transferred);
				break;
			}
			case UDP_SERVER_PACKET_SIGNATURE: {
				handleServerPacket(bytes_transferred);
				break;
			}
			default: {
#ifdef _DEBUG
				serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Client ({}) sent UDP packet with invalid signature!\n", _endpoint.address().to_string()));
#endif
			}
		}
	}

	startReceive();
}

void UDPServer::handleSend(shared_ptr<vector<unsigned char>> message, const boost::system::error_code& ec, size_t bytes_transferred) {
}

void UDPServer::handleClientPacket(size_t bytes_transferred) {
	unsigned long userID = ReadUInt32_LE();

	User* user = userManager.GetUserByUserID(userID);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but it's not logged in!\n", _endpoint.address().to_string()));
		return;
	}

	if (_endpoint.address().to_string() != user->GetUserIPAddress()) {
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but its IP address doesn't match the user's IP address: {}!\n", _endpoint.address().to_string(), user->GetUserIPAddress()));
		return;
	}

	unsigned char type = ReadUInt8();

#ifdef _DEBUG
	vector<unsigned char> buffer(_recvBuffer.begin(), _recvBuffer.begin() + bytes_transferred);
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {}{:X}", c < 0x10 ? "0" : "", c);
	}

	if (type != 1) {
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
			message->push_back(UDP_CLIENT_PACKET_SIGNATURE);
			message->push_back(0);
			message->push_back(1);

			_socket.async_send_to(boost::asio::buffer(*message), _endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

#ifdef _DEBUG
			bufferStr.clear();
			for (auto& c : *message) {
				bufferStr += format(" {}{:X}", c < 0x10 ? "0" : "", c);
			}

			serverConsole.Print(PrefixType::Debug, format("[ UDPServer ] Sent UDP packet to user ({}):{}\n", user->GetUserLogName(), bufferStr));
#endif
			break;
		}
		case 1: {
			unsigned char retryNum = ReadUInt8();
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent unregistered UDP packet type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void UDPServer::handleServerPacket(size_t bytes_transferred) {
	bool isServer = false;

	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (channel.ip == _endpoint.address().to_string()) {
				isServer = true;
				break;
			}
		}
	}

	if (isServer) {
		unsigned char serverID = ReadUInt8();
		unsigned char channelID = ReadUInt8();

		if (!serverID || serverID > serverConfig.serverList.size()) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}) sent UDP packet, but its serverID is invalid!\n", _endpoint.address().to_string()));
			return;
		}

		if (!channelID || channelID > serverConfig.serverList[serverID - 1].channels.size()) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}) sent UDP packet, but its channelID is invalid!\n", _endpoint.address().to_string()));
			return;
		}

		unsigned char type = ReadUInt8();

		switch (type) {
			case ServerPacketType::HELLO: {
				serverConfig.serverList[serverID - 1].channels[channelID - 1].isOnline = true;
				serverConfig.serverList[serverID - 1].channels[channelID - 1].lastHeartBeat = serverConsole.GetCurrentTime();
				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = 0;

				if (serverID != serverConfig.serverID) {
					serverConfig.serverList[serverID - 1].status = ServerStatus::Ready;
				}

				unsigned short numPlayers = (unsigned short)userManager.GetUsers().size();

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_SERVER_PACKET_SIGNATURE);
				message->push_back(serverConfig.serverID);
				message->push_back(serverConfig.channelID);
				message->push_back((unsigned char)numPlayers);
				message->push_back(numPlayers >> 8);

				_socket.async_send_to(boost::asio::buffer(*message), _endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

				break;
			}
			case ServerPacketType::HELLO_ANSWER: {
				unsigned short numPlayers = ReadUInt16_LE();

				serverConfig.serverList[serverID - 1].channels[channelID - 1].isOnline = true;
				serverConfig.serverList[serverID - 1].channels[channelID - 1].lastHeartBeat = serverConsole.GetCurrentTime();
				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = numPlayers;

				if (serverID != serverConfig.serverID) {
					serverConfig.serverList[serverID - 1].status = ServerStatus::Ready;
				}
				break;
			}
			case ServerPacketType::GOODBYE: {
				serverConfig.serverList[serverID - 1].channels[channelID - 1].isOnline = false;
				serverConfig.serverList[serverID - 1].channels[channelID - 1].lastHeartBeat = 0;
				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = 0;

				if (serverID != serverConfig.serverID) {
					serverConfig.serverList[serverID - 1].status = ServerStatus::NotReady;
					for (auto& channel : serverConfig.serverList[serverID - 1].channels) {
						if (channel.isOnline) {
							serverConfig.serverList[serverID - 1].status = ServerStatus::Ready;
							break;
						}
					}
				}
				break;
			}
			case ServerPacketType::HEARTBEAT: {
				serverConfig.serverList[serverID - 1].channels[channelID - 1].lastHeartBeat = serverConsole.GetCurrentTime();

				lock_guard<mutex> lock(_heartbeatMutex);
				auto key = make_pair(serverID, channelID);
				auto it = _heartbeatPromises.find(key);
				if (it != _heartbeatPromises.end()) {
					it->second->set_value(true);
					_heartbeatPromises.erase(it);
				}
				break;
			}
			case ServerPacketType::NUMPLAYERS: {
				unsigned short numPlayers = ReadUInt16_LE();

				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = numPlayers;
				break;
			}
			case ServerPacketType::DEADCHANNEL: {
				unsigned char sID = ReadUInt8();
				unsigned char chID = ReadUInt8();

				if (!sID || sID > serverConfig.serverList.size()) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}-{}) sent DEADCHANNEL UDP packet, but the received serverID is invalid!\n", serverID, channelID));
					break;
				}

				if (!chID || chID > serverConfig.serverList[sID - 1].channels.size()) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}-{}) sent DEADCHANNEL UDP packet, but the received channelID is invalid!\n", serverID, channelID));
					break;
				}

				serverConfig.serverList[sID - 1].channels[chID - 1].isOnline = false;
				serverConfig.serverList[sID - 1].channels[chID - 1].lastHeartBeat = 0;
				serverConfig.serverList[sID - 1].channels[chID - 1].numPlayers = 0;

				if (sID != serverConfig.serverID) {
					serverConfig.serverList[sID - 1].status = ServerStatus::NotReady;
					for (auto& channel : serverConfig.serverList[sID - 1].channels) {
						if (channel.isOnline) {
							serverConfig.serverList[sID - 1].status = ServerStatus::Ready;
							break;
						}
					}
				}
				break;
			}
			default: {
				serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}-{}) sent unregistered UDP packet type {}!\n", serverID, channelID, type));
				break;
			}
		}
	}
	else {
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP server packet, but it's not a server!\n", _endpoint.address().to_string()));
	}
}

void UDPServer::sendHelloPacketToAll() {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port);

			auto message = make_shared<vector<unsigned char>>();
			message->push_back(UDP_SERVER_PACKET_SIGNATURE);
			message->push_back(serverConfig.serverID);
			message->push_back(serverConfig.channelID);
			message->push_back(ServerPacketType::HELLO);

			_socket.async_send_to(boost::asio::buffer(*message), endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}
}

void UDPServer::sendGoodbytePacketToAll() {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port);

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_SERVER_PACKET_SIGNATURE);
				message->push_back(serverConfig.serverID);
				message->push_back(serverConfig.channelID);
				message->push_back(ServerPacketType::GOODBYE);

				_socket.async_send_to(boost::asio::buffer(*message), endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
		}
	}
}

void UDPServer::sendHeartbeatPacket(unsigned char serverID, unsigned char channelID) {
	boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(serverConfig.serverList[serverID - 1].channels[channelID - 1].ip), serverConfig.serverList[serverID - 1].channels[channelID - 1].port);

	auto message = make_shared<vector<unsigned char>>();
	message->push_back(UDP_SERVER_PACKET_SIGNATURE);
	message->push_back(serverConfig.serverID);
	message->push_back(serverConfig.channelID);
	message->push_back(ServerPacketType::HEARTBEAT);

	_socket.async_send_to(boost::asio::buffer(*message), endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UDPServer::sendDeadChannelPacketToAll(unsigned char serverID, unsigned char channelID) {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port);

				auto message = make_shared<vector<unsigned char>>();
				message->push_back(UDP_SERVER_PACKET_SIGNATURE);
				message->push_back(serverConfig.serverID);
				message->push_back(serverConfig.channelID);
				message->push_back(ServerPacketType::DEADCHANNEL);
				message->push_back(serverID);
				message->push_back(channelID);

				_socket.async_send_to(boost::asio::buffer(*message), endpoint, boost::bind(&UDPServer::handleSend, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
		}
	}
}