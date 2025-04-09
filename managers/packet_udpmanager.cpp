#include "packet_udpmanager.h"
#include "usermanager.h"
#include "serverconsole.h"

Packet_UdpManager packet_UdpManager;

void Packet_UdpManager::ParsePacket_Udp(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (!userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] Client ({}) has sent Packet_Udp, but it's not logged in!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] Parsing Packet_Udp from user ({})\n", user->GetUserLogName()));

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case 0: {
			// nothing to read
			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}\n", user->GetUserLogName(), type));
			break;
		}
		case 1: {
			unsigned long unk1 = packet->ReadUInt32_LE();

			serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, unk1: {}\n", user->GetUserLogName(), type, unk1));
			break;
		}
		case 2: {
			unsigned char subType = packet->ReadUInt8();

			switch (subType) {
				case 0: {
					unsigned long localIP = packet->ReadUInt32_LE();
					unsigned short retryNumPortType0 = packet->ReadUInt16_LE();
					unsigned short retryNumPortType1 = packet->ReadUInt16_LE();

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, localIP: {}.{}.{}.{}, retryNumPortType0: {}, retryNumPortType1: {}\n", user->GetUserLogName(), type, subType, (unsigned char)localIP, (unsigned char)(localIP >> 8), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 24), retryNumPortType0, retryNumPortType1));
					break;
				}
				case 1: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();
					unsigned short unk3 = packet->ReadUInt16_LE();

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}, unk3: {}\n", user->GetUserLogName(), type, subType, unk1, unk2, unk3));
					break;
				}
				case 2: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}\n", user->GetUserLogName(), type, subType, unk1, unk2));
					break;
				}
				case 3: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}\n", user->GetUserLogName(), type, subType, unk1, unk2));
					break;
				}
				case 4: {
					unsigned long unk1 = packet->ReadUInt32_LE();

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}\n", user->GetUserLogName(), type, subType, unk1));
					break;
				}
				case 5: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();

					string log;

					for (unsigned short i = 0; i < unk2; i++) {
						unsigned char unk3 = packet->ReadUInt8();
						log += format(", unk3_{}: {}", i, unk3);
					}

					serverConsole.Print(PrefixType::Info, format("[ Packet_UdpManager ] User ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}{}\n", user->GetUserLogName(), type, subType, unk1, unk2, log));
					break;
				}
				default: {
					serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent unregistered Packet_Udp type: {}, subType: {}!\n", user->GetUserLogName(), type, subType));
					break;
				}
			}

			break;
		}
		default: {
			serverConsole.Print(PrefixType::Warn, format("[ Packet_UdpManager ] User ({}) has sent unregistered Packet_Udp type: {}!\n", user->GetUserLogName(), type));
			break;
		}
	}
}