#include "udp_server.h"
#include "usermanager.h"
#include "serverconsole.h"
#include "serverconfig.h"
#include "databasemanager.h"
#include "roommanager.h"
#include "packet_umsgmanager.h"

UDPServer udpServer;

UDPPacket::UDPPacket(boost::asio::ip::udp::endpoint endpoint, vector<unsigned char> buffer) : _endpoint(endpoint), _buffer(buffer) {
	_signature = buffer.empty() ? 0 : buffer[0];
	_readOffset = 0;
	_writeOffset = (int)_buffer.size();
}

UDPServer::UDPServer() : _socket(_ioContext, NULL) {
	_port = 0;
	_recvBuffer = {};
	_pendingGoodbyePackets = 0;
	_isShuttingDown = false;
	_currentTime = 0;
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

void UDPServer::SendNumPlayersPacketToAll() {
	if (_isShuttingDown || !_udpServerThread.joinable()) {
		return;
	}

	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

				packet->WriteString(serverConfig.udpAuthKey);
				packet->WriteUInt8(serverConfig.serverID);
				packet->WriteUInt8(serverConfig.channelID);
				packet->WriteUInt8(ServerPacketType::NUMPLAYERS);
				packet->WriteUInt16_LE((unsigned short)userManager.GetUsers().size());

				sendPacket(packet, true);
			}
		}
	}
}

void UDPServer::OnSecondTick() {
	if (_isShuttingDown || !_udpServerThread.joinable()) {
		return;
	}

	vector<User*> users = userManager.GetUsers();

	users.erase(remove_if(users.begin(), users.end(), [](User* user) {
		return (user == NULL || user->GetConnection() == NULL);
	}), users.end());

	if (_currentTime + UDP_SERVER_HEARTBEAT_TIMEOUT < serverConsole.GetCurrentTime()) {
		for (auto& server : serverConfig.serverList) {
			if (server.id != serverConfig.serverID) {
				serverConfig.serverList[(unsigned char)(server.id - 1)].status = ServerStatus::NotReady;
			}

			for (auto& channel : server.channels) {
				if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
					continue;
				}

				serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].isOnline = false;
				serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].lastHeartbeat = 0;
				serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].numPlayers = 0;
			}
		}

		sendHelloPacketToAll();

		char isUserSessionAddedResult = 0;
		for (auto& user : users) {
			isUserSessionAddedResult = databaseManager.IsUserSessionAdded(user->GetUserID());
			if (isUserSessionAddedResult < 0 || isUserSessionAddedResult == 2) {
				userManager.DisconnectUser(user);
			}
			else if (isUserSessionAddedResult == 0) {
				if (!user->AddUserSession()) {
					userManager.DisconnectUser(user);
				}
			}
		}

		databaseManager.RemoveAllRooms();

		vector<Room*> rooms = roomManager.GetRooms();

		rooms.erase(remove_if(rooms.begin(), rooms.end(), [](Room* room) {
			return (room == NULL);
		}), rooms.end());

		for (auto& room : rooms) {
			roomManager.AddRoom(room);
		}
	}

	_currentTime = serverConsole.GetCurrentTime();

	UserNetwork userNetwork;
	for (auto& user : users) {
		userNetwork = user->GetUserNetwork();
		if (userNetwork.networkSet == 0x3 && (userNetwork.lastGuestHeartbeat + UDP_CLIENT_HEARTBEAT_TIMEOUT < _currentTime || userNetwork.lastHostHeartbeat + UDP_CLIENT_HEARTBEAT_TIMEOUT < _currentTime)) {
			packet_UMsgManager.SendPacket_UMsg_ServerMessage(user->GetConnection(), Packet_UMsgType::WarningMessage, "DISCONNECTED_BY_NETWORK_FAILURE");
			userManager.DisconnectUser(user);
		}
	}

	_pingRequests.erase(remove_if(_pingRequests.begin(), _pingRequests.end(), [this](PingRequest p) {
		return (p.timestamp + 3 > _currentTime);
	}), _pingRequests.end());

	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				if (channel.lastHeartbeat + UDP_SERVER_HEARTBEAT_TIMEOUT < _currentTime) {
					serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].isOnline = false;
					serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].lastHeartbeat = 0;
					serverConfig.serverList[(unsigned char)(server.id - 1)].channels[(unsigned char)(channel.id - 1)].numPlayers = 0;

					if (server.id != serverConfig.serverID) {
						serverConfig.serverList[(unsigned char)(server.id - 1)].status = ServerStatus::NotReady;
						for (auto& channel : serverConfig.serverList[(unsigned char)(server.id - 1)].channels) {
							if (channel.isOnline) {
								serverConfig.serverList[(unsigned char)(server.id - 1)].status = ServerStatus::Ready;
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
		_currentTime = serverConsole.GetCurrentTime();
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
	if (_isShuttingDown) {
		return;
	}

	if (!ec || ec == boost::asio::error::message_size) {
		vector<unsigned char> buffer(_recvBuffer.begin(), _recvBuffer.begin() + bytes_transferred);
		UDPPacket* packet = new UDPPacket(_endpoint, buffer);

		startReceive();

		unsigned char signature = packet->ReadUInt8();

		switch (signature) {
			case UDP_CLIENT_PACKET_SIGNATURE:
			case UDP_RELAY_PACKET_SIGNATURE: 
			case UDP_PING_PACKET_SIGNATURE: {
				bool queueIdle = false;
				{
					lock_guard<mutex> lock(_clientIncomingMutex);
					queueIdle = _clientIncomingQueue.empty();
					_clientIncomingQueue.push(packet);
				}

				if (queueIdle) {
					handleIncomingClientPacket();
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
					handleIncomingServerPacket();
				}
				break;
			}
			default: {
#ifdef _DEBUG
				serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Client ({}:{}) sent UDP packet with invalid signature!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
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

void UDPServer::handleIncomingClientPacket() {
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet;
	while (true) {
		{
			lock_guard<mutex> lock(_clientIncomingMutex);

			if (_clientIncomingQueue.empty()) {
				return;
			}

			packet = _clientIncomingQueue.front();
		}

#ifdef _DEBUG
		vector<unsigned char> buffer(packet->GetBuffer());
		string bufferStr;
		for (auto& c : buffer) {
			bufferStr += format(" {:02X}", c);
		}

		serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Received client UDP packet from client ({}:{}):{}\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), bufferStr));
#endif

		switch (packet->GetSignature()) {
			case UDP_CLIENT_PACKET_SIGNATURE: {
				unsigned long userID = packet->ReadUInt32_LE();

				User* user = userManager.GetUserByUserID(userID);
				if (!userManager.IsUserLoggedIn(user)) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent client UDP packet, but it's not logged in!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
					break;
				}

				if (packet->GetEndpoint().address().to_string() != user->GetUserIPAddress()) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent client UDP packet, but its IP address doesn't match the user's IP address: {}!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), user->GetUserIPAddress()));
					break;
				}

				unsigned char type = packet->ReadUInt8();

				switch (type) {
					case ClientPacketType::SocketInfo: {
						unsigned char portType = packet->ReadUInt8();
						if (portType != PortType::Guest && portType != PortType::Host) {
							serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent client UDP packet SocketInfo, but its portType is invalid: {}!\n", user->GetUserLogName(), portType));
							break;
						}

						unsigned long localIP = ~packet->ReadUInt32_LE();
						unsigned short localPort = packet->ReadUInt16_LE();
						unsigned char retryNum = packet->ReadUInt8();

						serverConsole.Print(PrefixType::Info, format("[ UDPServer ] User ({}) sent client UDP packet SocketInfo - userID: {}, portType: {}, localIP: {}.{}.{}.{}, localPort: {}, retryNum: {}, externalPort: {}\n", user->GetUserLogName(), userID, portType == PortType::Guest ? "guest" : "host", (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), localPort, retryNum, packet->GetEndpoint().port()));

						struct sockaddr_in addr {};
						inet_pton(AF_INET, packet->GetEndpoint().address().to_string().c_str(), &(addr.sin_addr));

						user->SetUserNetwork((PortType)portType, addr.sin_addr.S_un.S_addr, packet->GetEndpoint().port(), localIP, localPort, _currentTime);

						sendClientPacket(packet->GetEndpoint(), user->GetUserLogName());
						break;
					}
					case ClientPacketType::Heartbeat: {
						unsigned char retryNum = packet->ReadUInt8();

						unsigned short port = packet->GetEndpoint().port();
						PortType portType;
						const UserNetwork& userNetwork = user->GetUserNetwork();

						if (port == userNetwork.externalGuestPort) {
							portType = PortType::Guest;
						}
						else if (port == userNetwork.externalHostPort) {
							portType = PortType::Host;
						}
						else {
							serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent client UDP packet Heartbeat, but its port doesn't match any of the user's ports: {} and {}!\n", packet->GetEndpoint().address().to_string(), port, userNetwork.externalHostPort, userNetwork.externalGuestPort));
							break;
						}

#ifdef _DEBUG
						serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] User ({}) sent client UDP packet Heartbeat - userID: {}, retryNum: {}, port: {}\n", user->GetUserLogName(), userID, retryNum, port));
#endif

						user->SetUserNetworkHeartbeat(portType, _currentTime);

						break;
					}
					default: {
						serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent client UDP packet, but its type is invalid: {}!\n", user->GetUserLogName(), type));
						break;
					}
				}
				break;
			}
			case UDP_RELAY_PACKET_SIGNATURE: {
				unsigned long userID = packet->ReadUInt32_LE();

				User* user = userManager.GetUserByUserID(userID);
				if (!userManager.IsUserLoggedIn(user)) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent relay UDP packet, but it's not logged in!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
					break;
				}

				if (packet->GetEndpoint().address().to_string() != user->GetUserIPAddress()) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent relay UDP packet, but its IP address doesn't match the user's IP address: {}!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), user->GetUserIPAddress()));
					break;
				}

				const UserNetwork& userNetwork = user->GetUserNetwork();
				if (packet->GetEndpoint().port() != userNetwork.externalHostPort && packet->GetEndpoint().port() != userNetwork.externalGuestPort) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent relay UDP packet, but its port doesn't match any of the user's ports: {} and {}!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), userNetwork.externalHostPort, userNetwork.externalGuestPort));
					break;
				}

				Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
				if (room == NULL) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent relay UDP packet, but its room is NULL!\n", user->GetUserLogName()));
					break;
				}

				unsigned char portType = packet->ReadUInt8();
				if (portType != PortType::Guest && portType != PortType::Host) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent relay UDP packet, but its portType is invalid: {}!\n", user->GetUserLogName(), portType));
					break;
				}

				unsigned long targetUserID = packet->ReadUInt32_LE();
				if (!targetUserID) {
					// TODO: Figure out what this packet is about. Known structure (16 bytes):
					// relay signature (Y) - 1 byte
					// userID - 4 bytes
					// portType - 1 byte
					// unk1 (always 0) - 4 bytes
					// unk2 (always 0) - 1 byte
					// userID (again) - 4 bytes
					// retryNum - 1 byte
					break;
				}

				User* targetUser = userManager.GetUserByUserID(targetUserID);
				if (!userManager.IsUserLoggedIn(targetUser)) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent relay UDP packet, but targetUser is not logged in!\n", user->GetUserLogName()));
					break;
				}

				if (roomManager.GetRoomByRoomID(targetUser->GetCurrentRoomID()) != room) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] User ({}) sent relay UDP packet, but targetUser is not in the same room!\n", user->GetUserLogName()));
					break;
				}

#ifdef _DEBUG
				serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] User ({}) sent relay UDP packet - userID: {}, portType: {}, targetUserID: {}, port: {}\n", user->GetUserLogName(), userID, portType == PortType::Guest ? "guest" : "host", targetUserID, packet->GetEndpoint().port()));
#endif

				const UserNetwork& targetUserNetwork = targetUser->GetUserNetwork();

				sendRelayPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(targetUser->GetUserIPAddress()), portType == PortType::Host ? targetUserNetwork.externalHostPort : targetUserNetwork.externalGuestPort), packet->GetBuffer(), targetUser->GetUserLogName());
				break;
			}
			case UDP_PING_PACKET_SIGNATURE: {
				unsigned char type = packet->ReadUInt8();
				if (type != PingPacketType::Request && type != PingPacketType::Reply) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent ping UDP packet, but its type is invalid: {}!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), type));
					break;
				}

				unsigned short roomID = packet->ReadUInt16_LE();

				Room* room = roomManager.GetRoomByRoomID(roomID);
				if (room == NULL) {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent ping UDP packet, but room is NULL!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
					break;
				}

				User* roomHostUser = room->GetRoomHostUser();
				if (roomHostUser == NULL) {
					break;
				}

				unsigned char retryNum = packet->ReadUInt8();

#ifdef _DEBUG
				serverConsole.Log(PrefixType::Info, format("[ UDPServer ] Client ({}:{}) sent ping UDP packet - type: {}, roomID: {}, retryNum: {}\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port(), type, roomID, retryNum));
#endif

				switch (type) {
					case PingPacketType::Request: {
						const UserNetwork& userNetwork = roomHostUser->GetUserNetwork();
						boost::asio::ip::udp::endpoint endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(roomHostUser->GetUserIPAddress()), userNetwork.externalHostPort);

						_pingRequests.push_back({ packet->GetEndpoint(), endpoint });
						sendPingPacket(endpoint, packet->GetBuffer());
						break;
					}
					case PingPacketType::Reply: {
						auto it = find_if(_pingRequests.begin(), _pingRequests.end(), [&packet](PingRequest p) { return p.replyEndpoint == packet->GetEndpoint(); });
						if (it != _pingRequests.end()) {
							PingRequest pingRequest = _pingRequests.at(distance(_pingRequests.begin(), it));

							sendPingPacket(pingRequest.requestEndpoint, packet->GetBuffer());
							_pingRequests.erase(it);
						}
						break;
					}
				}
				break;
			}
		}

		delete packet;
		{
			lock_guard<mutex> lock(_clientIncomingMutex);
			_clientIncomingQueue.pop();
		}
	}
}

void UDPServer::handleIncomingServerPacket() {
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet;
	while (true) {
		{
			lock_guard<mutex> lock(_serverIncomingMutex);

			if (_serverIncomingQueue.empty()) {
				return;
			}

			packet = _serverIncomingQueue.front();
		}

		const string& authKey = packet->ReadString();
		if (authKey == serverConfig.udpAuthKey) {
			unsigned char serverID = packet->ReadUInt8();
			unsigned char channelID = packet->ReadUInt8();

			if (!serverID || serverID > serverConfig.serverList.size()) {
				serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}:{}) sent UDP packet, but its serverID is invalid!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
				goto DeletePacket;
			}

			if (!channelID || channelID > serverConfig.serverList[(unsigned char)(serverID - 1)].channels.size()) {
				serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}:{}) sent UDP packet, but its channelID is invalid!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
				goto DeletePacket;
			}

			unsigned char type = packet->ReadUInt8();

			switch (type) {
				case ServerPacketType::HELLO: 
				case ServerPacketType::HELLO_ANSWER: {
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].isOnline = true;
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].lastHeartbeat = _currentTime;
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].numPlayers = packet->ReadUInt16_LE();

					if (serverID != serverConfig.serverID) {
						serverConfig.serverList[(unsigned char)(serverID - 1)].status = ServerStatus::Ready;
					}

					if (type == ServerPacketType::HELLO) {
						sendHelloAnswerPacket(packet->GetEndpoint());
					}
					break;
				}
				case ServerPacketType::GOODBYE: {
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].isOnline = false;
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].lastHeartbeat = 0;
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].numPlayers = 0;

					if (serverID != serverConfig.serverID) {
						serverConfig.serverList[(unsigned char)(serverID - 1)].status = ServerStatus::NotReady;
						for (auto& channel : serverConfig.serverList[(unsigned char)(serverID - 1)].channels) {
							if (channel.isOnline) {
								serverConfig.serverList[(unsigned char)(serverID - 1)].status = ServerStatus::Ready;
								break;
							}
						}
					}
					break;
				}
				case ServerPacketType::HEARTBEAT: {
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].lastHeartbeat = _currentTime;
					break;
				}
				case ServerPacketType::NUMPLAYERS: {
					serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].numPlayers = packet->ReadUInt16_LE();
					break;
				}
				default: {
					serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Server channel ({}-{}) sent unregistered UDP packet type {}!\n", serverID, channelID, type));
					break;
				}
			}
		}
		else {
			serverConsole.Print(PrefixType::Warn, format("[ UDPServer ] Client ({}:{}) sent UDP server packet, but its authKey is invalid!\n", packet->GetEndpoint().address().to_string(), packet->GetEndpoint().port()));
		}

DeletePacket:
		delete packet;
		{
			lock_guard<mutex> lock(_serverIncomingMutex);
			_serverIncomingQueue.pop();
		}
	}
}

void UDPServer::handleOutgoingClientPacket() {
	if (_isShuttingDown) {
		return;
	}

	while (true) {
		UDPPacket* packet;
		{
			lock_guard<mutex> lock(_clientOutgoingMutex);

			if (_clientOutgoingQueue.empty()) {
				return;
			}

			packet = _clientOutgoingQueue.front();
		}

		_socket.async_send_to(boost::asio::buffer(packet->GetBuffer()), packet->GetEndpoint(), [packet](const boost::system::error_code& ec, size_t bytesTransferred) {
			delete packet;
		});

		{
			lock_guard<mutex> lock(_clientOutgoingMutex);
			_clientOutgoingQueue.pop();
		}
	}
}

void UDPServer::handleOutgoingServerPacket() {
	if (_isShuttingDown) {
		return;
	}

	while (true) {
		UDPPacket* packet;
		{
			lock_guard<mutex> lock(_serverOutgoingMutex);

			if (_serverOutgoingQueue.empty()) {
				return;
			}

			packet = _serverOutgoingQueue.front();
		}

		_socket.async_send_to(boost::asio::buffer(packet->GetBuffer()), packet->GetEndpoint(), [packet](const boost::system::error_code& ec, size_t bytesTransferred) {
			delete packet;
		});

		{
			lock_guard<mutex> lock(_serverOutgoingMutex);
			_serverOutgoingQueue.pop();
		}
	}
}

void UDPServer::sendPacket(UDPPacket* packet, bool isServer) {
	if (_isShuttingDown || packet == NULL) {
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
		isServer ? handleOutgoingServerPacket() : handleOutgoingClientPacket();
	}
}

void UDPServer::sendClientPacket(const boost::asio::ip::udp::endpoint& endpoint, const string& userLogName) {
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet = new UDPPacket(endpoint, { UDP_CLIENT_PACKET_SIGNATURE });

	packet->WriteUInt8(0);
	packet->WriteUInt8(1);

	sendPacket(packet);

#ifdef _DEBUG
	vector<unsigned char> buffer(packet->GetBuffer());
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {:02X}", c);
	}

	serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Sent client UDP packet to user ({}):{}\n", userLogName, bufferStr));
#endif
}

void UDPServer::sendRelayPacket(const boost::asio::ip::udp::endpoint& endpoint, vector<unsigned char> buffer, const string& userLogName) {
	if (_isShuttingDown) {
		return;
	}

	buffer.erase(buffer.begin(), buffer.begin() + 10);
	UDPPacket* packet = new UDPPacket(endpoint, buffer);

	sendPacket(packet);

#ifdef _DEBUG
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {:02X}", c);
	}

	serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Sent relay UDP packet to user ({}):{}\n", userLogName, bufferStr));
#endif
}

void UDPServer::sendPingPacket(const boost::asio::ip::udp::endpoint& endpoint, const vector<unsigned char>& buffer) {
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet = new UDPPacket(endpoint, buffer);

	sendPacket(packet);

#ifdef _DEBUG
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {:02X}", c);
	}

	serverConsole.Log(PrefixType::Debug, format("[ UDPServer ] Sent ping UDP packet to client ({}:{}):{}\n", endpoint.address().to_string(), endpoint.port(), bufferStr));
#endif
}

void UDPServer::sendHelloAnswerPacket(const boost::asio::ip::udp::endpoint& endpoint) {
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet = new UDPPacket(endpoint, { UDP_SERVER_PACKET_SIGNATURE });

	packet->WriteString(serverConfig.udpAuthKey);
	packet->WriteUInt8(serverConfig.serverID);
	packet->WriteUInt8(serverConfig.channelID);
	packet->WriteUInt8(ServerPacketType::HELLO_ANSWER);
	packet->WriteUInt16_LE((unsigned short)userManager.GetUsers().size());

	sendPacket(packet, true);
}

void UDPServer::sendHelloPacketToAll() {
	if (_isShuttingDown) {
		return;
	}

	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

			packet->WriteString(serverConfig.udpAuthKey);
			packet->WriteUInt8(serverConfig.serverID);
			packet->WriteUInt8(serverConfig.channelID);
			packet->WriteUInt8(ServerPacketType::HELLO);
			packet->WriteUInt16_LE((unsigned short)userManager.GetUsers().size());

			sendPacket(packet, true);
		}
	}
}

void UDPServer::sendGoodbytePacketToAll() {
	if (!_isShuttingDown) {
		return;
	}

	_pendingGoodbyePackets = 0;
	for (auto& server : serverConfig.serverList) {
		for (auto& channel : server.channels) {
			if (server.id == serverConfig.serverID && channel.id == serverConfig.channelID) {
				continue;
			}

			if (channel.isOnline) {
				_pendingGoodbyePackets++;
				UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(channel.ip), channel.port), { UDP_SERVER_PACKET_SIGNATURE });

				packet->WriteString(serverConfig.udpAuthKey);
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
	if (_isShuttingDown) {
		return;
	}

	UDPPacket* packet = new UDPPacket(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].ip), serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].port), { UDP_SERVER_PACKET_SIGNATURE });

	packet->WriteString(serverConfig.udpAuthKey);
	packet->WriteUInt8(serverConfig.serverID);
	packet->WriteUInt8(serverConfig.channelID);
	packet->WriteUInt8(ServerPacketType::HEARTBEAT);

	sendPacket(packet, true);
}