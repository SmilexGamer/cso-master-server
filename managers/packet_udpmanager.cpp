#include "packet_udpmanager.h"
#include "packet_umsgmanager.h"
#include "usermanager.h"
#include "roommanager.h"
#include "serverconsole.h"
#include "serverconfig.h"

Packet_UdpManager packet_UdpManager;

void Packet_UdpManager::ParsePacket_Udp(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] Client ({}) has sent Packet_Udp, but it's not logged in!\n", connection->GetLogEndpoint()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] Parsing Packet_Udp from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_UdpType::NetworkFailure: {
			parsePacket_Udp_NetworkFailure(user);
			break;
		}
		case Packet_UdpType::HolepunchFail: {
			parsePacket_Udp_HolepunchFail(user, packet);
			break;
		}
		case Packet_UdpType::SocketInfo: {
			parsePacket_Udp_SocketInfo(user, packet);
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent unregistered Packet_Udp type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_UdpManager::parsePacket_Udp_NetworkFailure(User* user) {
	if (user == NULL) {
		return;
	}

	auto& connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp NetworkFailure\n", user->GetUserLogName()));

	packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "DISCONNECTED_BY_NETWORK_FAILURE");
	userManager.DisconnectUser(user);
}

void Packet_UdpManager::parsePacket_Udp_HolepunchFail(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto& connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (user->GetUserStatus() != UserStatus::InRoom && user->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail, but it's not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	Room* room = roomManager.GetRoomByRoomID(user->GetCurrentRoomID());
	if (room == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail, but its room is NULL!\n", user->GetUserLogName()));
		return;
	}

	unsigned long userID = packet->ReadUInt32_LE();

	User* targetUser = userManager.GetUserByUserID(userID);
	if (targetUser == NULL) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail, but targetUser is not logged in!\n", user->GetUserLogName()));
		return;
	}

	if (targetUser->GetUserStatus() != UserStatus::InRoom && targetUser->GetUserStatus() != UserStatus::InGame) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail, but targetUser is not in a room or in game!\n", user->GetUserLogName()));
		return;
	}

	if (roomManager.GetRoomByRoomID(targetUser->GetCurrentRoomID()) != room) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail, but targetUser is not in the same room!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp HolepunchFail - userID: {}\n", user->GetUserLogName(), userID));

	sendPacket_Udp_ModifySocketInfo(connection, PortType::Guest, userID, serverConfig.ip, serverConfig.port);
	sendPacket_Udp_ModifySocketInfo(connection, PortType::Host, userID, serverConfig.ip, serverConfig.port);
}

void Packet_UdpManager::parsePacket_Udp_SocketInfo(User* user, TCPConnection::Packet::pointer packet) {
	if (user == NULL || packet == NULL) {
		return;
	}

	auto& connection = user->GetConnection();
	if (connection == NULL) {
		return;
	}

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case Packet_Udp_SocketInfoType::SocketInfoSuccess: {
			unsigned long localIP = packet->ReadUInt32_LE();
			unsigned short retryNumHost = packet->ReadUInt16_LE();
			unsigned short retryNumGuest = packet->ReadUInt16_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo SocketInfoSuccess - localIP: {}.{}.{}.{}, retryNumHost: {}, retryNumGuest: {}\n", user->GetUserLogName(), (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), retryNumHost, retryNumGuest));
			break;
		}
		case Packet_Udp_SocketInfoType::SocketInfoFail: {
			unsigned long localIP = packet->ReadUInt32_LE();
			unsigned short localHostPort = packet->ReadUInt16_LE();
			unsigned short localGuestPort = packet->ReadUInt16_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo SocketInfoFail - localIP: {}.{}.{}.{}, localHostPort: {}, localGuestPort: {}\n", user->GetUserLogName(), (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), localHostPort, localGuestPort));
			break;
		}
		case Packet_Udp_SocketInfoType::HolepunchSuccess: {
			unsigned long userID = packet->ReadUInt32_LE();
			unsigned short retryNum = packet->ReadUInt16_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo HolepunchSuccess - userID: {}, retryNum: {}\n", user->GetUserLogName(), userID, retryNum));
			break;
		}
		case Packet_Udp_SocketInfoType::RelaySuccess: {
			unsigned long userID = packet->ReadUInt32_LE();
			unsigned short retryNum = packet->ReadUInt16_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo RelaySuccess - userID: {}, retryNum: {}\n", user->GetUserLogName(), userID, retryNum));
			break;
		}
		case Packet_Udp_SocketInfoType::RelayFail: {
			unsigned long userID = packet->ReadUInt32_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo RelayFail - userID: {}\n", user->GetUserLogName(), userID));

			packet_UMsgManager.SendPacket_UMsg_ServerMessage(connection, Packet_UMsgType::WarningMessage, "DISCONNECTED_BY_NETWORK_FAILURE");
			userManager.DisconnectUser(user);
			break;
		}
		case Packet_Udp_SocketInfoType::MessageHistory: {
			unsigned long userID = packet->ReadUInt32_LE();
			unsigned short size = packet->ReadUInt16_LE();
			const vector<unsigned char>& buffer = packet->ReadArray_UInt8(size);

			string bufferStr;
			for (auto& c : buffer) {
				bufferStr += format(" {:02X}", c);
			}

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp SocketInfo MessageHistory - userID: {}, size: {}, buffer:{}\n", user->GetUserLogName(), userID, size, bufferStr));
			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent unregistered Packet_Udp SocketInfo type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}

void Packet_UdpManager::sendPacket_Udp_ModifySocketInfo(TCPConnection::pointer connection, PortType portType, unsigned long userID, unsigned long ip, unsigned short port) {
	if (connection == NULL) {
		return;
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Server, connection, { (unsigned char)PacketID::Udp });
	if (packet == NULL) {
		return;
	}

	packet->WriteUInt8(Packet_UdpType::ModifySocketInfo);
	packet->WriteUInt8(portType);
	packet->WriteUInt32_LE(userID);
	packet->WriteUInt32_LE(ip);
	packet->WriteUInt16_LE(port);

	packet->Send();
}