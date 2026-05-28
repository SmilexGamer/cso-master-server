#include "udp_server.h"
#include "usermanager.h"
#include "serverconsole.h"
#include "serverconfig.h"
#include "databasemanager.h"

UDPServer udpServer;

UDPPacket::UDPPacket(boost::asio::ip::udp::endpoint endpoint, vector<unsigned char> buffer) : _endpoint(endpoint), _buffer(buffer) {
	_readOffset = 0;
	_writeOffset = (int)_buffer.size();
}

UDPServer::UDPServer() : _socket(_ioContext, NULL) {
	_port = 0;
	_recvBuffer = {};
	_pendingGoodbyePackets = 0;
	_isShuttingDown = false;
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

void UDPServer::SendNumPlayersPacketToAll(unsigned short numPlayers) {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

				packet->WriteUInt8(serverConfig.serverID);
				packet->WriteUInt8(serverConfig.channelID);
				packet->WriteUInt8(ServerPacketType::NUMPLAYERS);
				packet->WriteUInt16_LE(numPlayers);

				SendPacket(packet, true);
			}
		}
	}
}

void UDPServer::OnSecondTick() {
	if (_isShuttingDown) {
		return;
	}

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

void UDPServer::SendPacket(UDPPacket* packet, bool isServer) {
	if (packet == NULL) {
		return;
	}

	bool queueIdle = false;
	if (isServer) {
		lock_guard<mutex> lock(_serverOutgoingMutex);
		queueIdle = _serverOutgoingQueue.empty();
		_serverOutgoingQueue.push(packet);
	}
	else {
		lock_guard<mutex> lock(_clientOutgoingMutex);
		queueIdle = _clientOutgoingQueue.empty();
		_clientOutgoingQueue.push(packet);
	}

	if (queueIdle) {
		isServer ? handleOutgoingServerPacket(packet) : handleOutgoingClientPacket(packet);
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

			_isShuttingDown = true;
			sendGoodbytePacketToAll();
			_udpServerThread.join();
		}
	}
	catch (exception& e) {
		serverConsole.Print(PrefixType::Error, format("[ UDPServer ] Error on shutdown: {}\n", e.what()));
		return -1;
	}

	return 0;
}

void UDPServer::startReceive() {
	if (_isShuttingDown) {
		return;
	}

	_socket.async_receive_from(boost::asio::buffer(_recvBuffer), _endpoint, boost::bind(&UDPServer::handleReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UDPServer::handleReceive(const boost::system::error_code& ec, size_t bytes_transferred) {
	if (!ec || ec == boost::asio::error::message_size) {
		vector<unsigned char> buffer(_recvBuffer.begin(), _recvBuffer.begin() + bytes_transferred);
		UDPPacket* packet = new UDPPacket(_endpoint, buffer);

		startReceive();

		unsigned char signature = packet->ReadUInt8();

		switch (signature) {
			case UDP_CLIENT_PACKET_SIGNATURE: {
				bool queueIdle = false;
				{
					lock_guard<mutex> lock(_clientIncomingMutex);
					queueIdle = _clientIncomingQueue.empty();
					_clientIncomingQueue.push(packet);
				}

				if (queueIdle) {
					handleIncomingClientPacket(packet);
				}
				break;
			}
			case UDP_SERVER_PACKET_SIGNATURE: {
				bool queueIdle = false;
				{
					lock_guard<mutex> lock(_serverIncomingMutex);
					queueIdle = _serverIncomingQueue.empty();
					_serverIncomingQueue.push(packet);
				}

				if (queueIdle) {
					handleIncomingServerPacket(packet);
				}
				break;
			}
			default: {
#ifdef _DEBUG
				serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Client ({}) sent UDP packet with invalid signature!\n", packet->GetEndpoint().address().to_string()));
#endif
				delete packet;
				packet = NULL;
				break;
			}
		}
	}
	else {
		startReceive();
	}
}

void UDPServer::handleIncomingClientPacket(UDPPacket* packet) {
	if (packet == NULL) {
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	User* user = userManager.GetUserByUserID(userID);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but it's not logged in!\n", packet->GetEndpoint().address().to_string()));
		return;
	}

	if (user->GetConnection() == NULL) {
		return;
	}

	if (packet->GetEndpoint().address().to_string() != user->GetUserIPAddress()) {
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP packet, but its IP address doesn't match the user's IP address: {}!\n", packet->GetEndpoint().address().to_string(), user->GetUserIPAddress()));
		return;
	}

	unsigned char type = packet->ReadUInt8();

#ifdef _DEBUG
	vector<unsigned char> buffer(packet->GetBuffer());
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {:02X}", c);
	}

	if (type != 1) {
		serverConsole.Print(PrefixType::Debug, format("[ UDPServer ] Received UDP packet from user ({}):{}\n", user->GetUserLogName(), bufferStr));
	}
#endif

	switch (type) {
		case 0: {
			unsigned char portType = packet->ReadUInt8();
			unsigned long localIP = ~packet->ReadUInt32_LE();
			unsigned short localPort = packet->ReadUInt16_LE();
			unsigned char retryNum = packet->ReadUInt8();

			serverConsole.Print(PrefixType::Info, format("[ UDPServer ] User ({}) sent UDP packet - userID: {}, type: {}, portType: {}, localIP: {}.{}.{}.{}, localPort: {}, retryNum: {}, externalPort: {}\n", user->GetUserLogName(), userID, type, portType, (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), localPort, retryNum, packet->GetEndpoint().port()));

			struct sockaddr_in addr {};
			inet_pton(AF_INET, packet->GetEndpoint().address().to_string().c_str(), &(addr.sin_addr));

			user->SetUserNetwork((PortType)portType, addr.sin_addr.S_un.S_addr, packet->GetEndpoint().port(), localIP, localPort);

			sendClientPacket(packet->GetEndpoint(), user->GetUserLogName());
			break;
		}
		case 1: {
			unsigned char retryNum = packet->ReadUInt8();
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent unregistered UDP packet type {}!\n", user->GetUserLogName(), type));
			break;
		}
	}

	delete packet;
	packet = NULL;

	UDPPacket* nextPacket = NULL;
	{
		lock_guard<mutex> lock(_clientIncomingMutex);
		_clientIncomingQueue.pop();
		if (!_clientIncomingQueue.empty()) {
			nextPacket = _clientIncomingQueue.front();
		}
	}

	if (nextPacket) {
		handleIncomingClientPacket(nextPacket);
	}
}

void UDPServer::handleIncomingServerPacket(UDPPacket* packet) {
	if (packet == NULL) {
		return;
	}

	bool isServer = false;

	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (channel.ip == packet->GetEndpoint().address().to_string()) {
				isServer = true;
				break;
			}
		}
	}

	if (isServer) {
		unsigned char serverID = packet->ReadUInt8();
		unsigned char channelID = packet->ReadUInt8();

		if (!serverID || serverID > serverConfig.serverList.size()) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}) sent UDP packet, but its serverID is invalid!\n", packet->GetEndpoint().address().to_string()));
			return;
		}

		if (!channelID || channelID > serverConfig.serverList[serverID - 1].channels.size()) {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}) sent UDP packet, but its channelID is invalid!\n", packet->GetEndpoint().address().to_string()));
			return;
		}

		unsigned char type = packet->ReadUInt8();

		switch (type) {
			case ServerPacketType::HELLO: {
				serverConfig.serverList[serverID - 1].channels[channelID - 1].isOnline = true;
				serverConfig.serverList[serverID - 1].channels[channelID - 1].lastHeartBeat = serverConsole.GetCurrentTime();
				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = 0;

				if (serverID != serverConfig.serverID) {
					serverConfig.serverList[serverID - 1].status = ServerStatus::Ready;
				}

				unsigned short numPlayers = (unsigned short)userManager.GetUsers().size();
				sendHelloAnswerPacket(packet->GetEndpoint(), numPlayers);
				break;
			}
			case ServerPacketType::HELLO_ANSWER: {
				unsigned short numPlayers = packet->ReadUInt16_LE();

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
				break;
			}
			case ServerPacketType::NUMPLAYERS: {
				unsigned short numPlayers = packet->ReadUInt16_LE();

				serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = numPlayers;
				break;
			}
			case ServerPacketType::DEADCHANNEL: {
				unsigned char sID = packet->ReadUInt8();
				unsigned char chID = packet->ReadUInt8();

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
		serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}) sent UDP server packet, but it's not a server!\n", packet->GetEndpoint().address().to_string()));
	}

	delete packet;
	packet = NULL;

	UDPPacket* nextPacket = NULL;
	{
		lock_guard<mutex> lock(_serverIncomingMutex);
		_serverIncomingQueue.pop();
		if (!_serverIncomingQueue.empty()) {
			nextPacket = _serverIncomingQueue.front();
		}
	}

	if (nextPacket) {
		handleIncomingServerPacket(nextPacket);
	}
}

void UDPServer::handleOutgoingClientPacket(UDPPacket* packet) {
	if (packet == NULL || _isShuttingDown) {
		return;
	}

	_socket.async_send_to(boost::asio::buffer(packet->GetBuffer()), packet->GetEndpoint(), [packet](const boost::system::error_code& ec, size_t bytesTransferred) {
		delete packet;
	});

	UDPPacket* nextPacket = NULL;
	{
		lock_guard<mutex> lock(_clientOutgoingMutex);
		_clientOutgoingQueue.pop();
		if (!_clientOutgoingQueue.empty()) {
			nextPacket = _clientOutgoingQueue.front();
		}
	}

	if (nextPacket) {
		handleOutgoingClientPacket(nextPacket);
	}
}

void UDPServer::handleOutgoingServerPacket(UDPPacket* packet) {
	if (packet == NULL || _isShuttingDown) {
		return;
	}

	_socket.async_send_to(boost::asio::buffer(packet->GetBuffer()), packet->GetEndpoint(), [packet](const boost::system::error_code& ec, size_t bytesTransferred) {
		delete packet;
	});

	UDPPacket* nextPacket = NULL;
	{
		lock_guard<mutex> lock(_serverOutgoingMutex);
		_serverOutgoingQueue.pop();
		if (!_serverOutgoingQueue.empty()) {
			nextPacket = _serverOutgoingQueue.front();
		}
	}

	if (nextPacket) {
		handleOutgoingServerPacket(nextPacket);
	}
}

void UDPServer::sendClientPacket(const boost::asio::ip::udp::endpoint& endpoint, const string& userLogName) {
	UDPPacket* packet = new UDPPacket(endpoint, { UDP_CLIENT_PACKET_SIGNATURE });

	packet->WriteUInt8(0);
	packet->WriteUInt8(1);

	SendPacket(packet);

#ifdef _DEBUG
	vector<unsigned char> buffer(packet->GetBuffer());
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {:02X}", c);
	}

	serverConsole.Print(PrefixType::Debug, format("[ UDPServer ] Sent UDP packet to user ({}):{}\n", userLogName, bufferStr));
#endif
}

void UDPServer::sendHelloAnswerPacket(const boost::asio::ip::udp::endpoint& endpoint, unsigned short numPlayers) {
	UDPPacket* packet = new UDPPacket(endpoint, { UDP_SERVER_PACKET_SIGNATURE });

	packet->WriteUInt8(serverConfig.serverID);
	packet->WriteUInt8(serverConfig.channelID);
	packet->WriteUInt8(ServerPacketType::HELLO_ANSWER);
	packet->WriteUInt16_LE(numPlayers);

	SendPacket(packet, true);
}

void UDPServer::sendHelloPacketToAll() {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

			packet->WriteUInt8(serverConfig.serverID);
			packet->WriteUInt8(serverConfig.channelID);
			packet->WriteUInt8(ServerPacketType::HELLO);

			SendPacket(packet, true);
		}
	}
}

void UDPServer::sendGoodbytePacketToAll() {
	_pendingGoodbyePackets = 0;
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				_pendingGoodbyePackets++;
				UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

				packet->WriteUInt8(serverConfig.serverID);
				packet->WriteUInt8(serverConfig.channelID);
				packet->WriteUInt8(ServerPacketType::GOODBYE);

				_socket.async_send_to(boost::asio::buffer(packet->GetBuffer()), packet->GetEndpoint(), [this, packet](const boost::system::error_code& ec, size_t bytesTransferred) {
					delete packet;
					if (--_pendingGoodbyePackets == 0) {
						_ioContext.stop();
					}
				});
			}
		}
	}
	if (_pendingGoodbyePackets == 0) {
		_ioContext.stop();
	}
}

void UDPServer::sendHeartbeatPacket(unsigned char serverID, unsigned char channelID) {
	UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(serverConfig.serverList[serverID - 1].channels[channelID - 1].ip), serverConfig.serverList[serverID - 1].channels[channelID - 1].port), { UDP_SERVER_PACKET_SIGNATURE });

	packet->WriteUInt8(serverConfig.serverID);
	packet->WriteUInt8(serverConfig.channelID);
	packet->WriteUInt8(ServerPacketType::HEARTBEAT);

	SendPacket(packet, true);
}

void UDPServer::sendDeadChannelPacketToAll(unsigned char serverID, unsigned char channelID) {
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

				packet->WriteUInt8(serverConfig.serverID);
				packet->WriteUInt8(serverConfig.channelID);
				packet->WriteUInt8(ServerPacketType::DEADCHANNEL);
				packet->WriteUInt8(serverID);
				packet->WriteUInt8(channelID);

				SendPacket(packet, true);
			}
		}
	}
}