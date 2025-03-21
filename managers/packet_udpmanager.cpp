#include "packet_udpmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_UdpManager packet_UdpManager;

void Packet_UdpManager::ParsePacket_Udp(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp, but it's not logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_UdpManager] Parsing Packet_Udp from client ({})\n", user->GetConnection()->GetEndPoint());

	unsigned char type = packet->ReadUInt8();

	switch (type) {
		case 0: {
			// nothing to read
			cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}\n", user->GetConnection()->GetEndPoint(), type);
			break;
		}
		case 1: {
			unsigned long unk1 = packet->ReadUInt32_LE();

			cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, unk1: {}\n", user->GetConnection()->GetEndPoint(), type, unk1);
			break;
		}
		case 2: {
			unsigned char subType = packet->ReadUInt8();

			switch (subType) {
				case 0: {
					unsigned long localIP = packet->ReadUInt32_BE();
					unsigned short retryNumPortType0 = packet->ReadUInt16_LE();
					unsigned short retryNumPortType1 = packet->ReadUInt16_LE();

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, localIP: {}.{}.{}.{}, retryNumPortType0: {}, retryNumPortType1: {}\n", user->GetConnection()->GetEndPoint(), type, subType, (unsigned char)(localIP >> 24), (unsigned char)(localIP >> 16), (unsigned char)(localIP >> 8), (unsigned char)localIP, retryNumPortType0, retryNumPortType1);
					break;
				}
				case 1: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();
					unsigned short unk3 = packet->ReadUInt16_LE();

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}, unk3: {}\n", user->GetConnection()->GetEndPoint(), type, subType, unk1, unk2, unk3);
					break;
				}
				case 2: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}\n", user->GetConnection()->GetEndPoint(), type, subType, unk1, unk2);
					break;
				}
				case 3: {
					unsigned long unk1 = packet->ReadUInt32_LE();
					unsigned short unk2 = packet->ReadUInt16_LE();

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}\n", user->GetConnection()->GetEndPoint(), type, subType, unk1, unk2);
					break;
				}
				case 4: {
					unsigned long unk1 = packet->ReadUInt32_LE();

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}\n", user->GetConnection()->GetEndPoint(), type, subType, unk1);
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

					cout << format("[Packet_UdpManager] Client ({}) has sent Packet_Udp - type: {}, subType: {}, unk1: {}, unk2: {}{}\n", user->GetConnection()->GetEndPoint(), type, subType, unk1, unk2, log);
					break;
				}
				default: {
					cout << format("[Packet_UdpManager] Client ({}) has sent unregistered Packet_Udp type: {}, subType: {}!\n", user->GetConnection()->GetEndPoint(), type, subType);
					break;
				}
			}

			break;
		}
		default: {
			cout << format("[Packet_UdpManager] Client ({}) has sent unregistered Packet_Udp type: {}!\n", packet->GetConnection()->GetEndPoint(), type);
			break;
		}
	}
}