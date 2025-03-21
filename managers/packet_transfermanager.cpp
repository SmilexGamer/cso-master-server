#include "packet_transfermanager.h"
#include "packet_serverlistmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include <iostream>

Packet_TransferManager packet_TransferManager;

void Packet_TransferManager::ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user != NULL) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_TransferLogin, but it's already logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_TransferManager] Parsing Packet_TransferLogin from client ({})\n", packet->GetConnection()->GetEndPoint());

	string userName = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_TransferLogin - userName: {}, hardwareID:{}, pcBang: {}, unk: {}\n", packet->GetConnection()->GetEndPoint(), userName.c_str(), hardwareIDStr.c_str(), pcBang, unk);
}

void Packet_TransferManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet) {
	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (user == NULL) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer, but it's not logged in\n", packet->GetConnection()->GetEndPoint());
		return;
	}

	cout << format("[Packet_TransferManager] Parsing Packet_RequestTransfer from client ({})\n", user->GetConnection()->GetEndPoint());

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	if (!serverID || serverID > serverConfig.serverList.size()) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer with invalid serverID: {}\n", user->GetConnection()->GetEndPoint(), serverID);
		return;
	}

	if (!channelID || channelID > serverConfig.serverList[serverID - 1].channels.size()) {
		cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer with invalid channelID: {}, serverID: {}\n", user->GetConnection()->GetEndPoint(), channelID, serverID);
		return;
	}

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", user->GetConnection()->GetEndPoint(), serverID, channelID);

	if (serverID == serverConfig.serverID && channelID == serverConfig.channelID) {
		vector<User*> users = userManager.GetUsers();
		vector<UserFull> newUsers;
		for (auto& user : users) {
			UserCharacterResult result = user->GetUserCharacter(UserCharacterFlag::All);
			if (result.result) {
				newUsers.push_back({ user, result.userCharacter });
			}
		}

		users.clear();

		packet_ServerListManager.SendPacket_Lobby(user->GetConnection(), newUsers);
		packet_ServerListManager.SendPacket_RoomList(user->GetConnection());
	}
	else {
		struct sockaddr_in addr {};

		inet_pton(AF_INET, serverConfig.serverList[serverID - 1].channels[channelID - 1].ip.c_str(), &(addr.sin_addr));

		sendPacket_Transfer(user, (unsigned long)addr.sin_addr.S_un.S_addr, serverConfig.serverList[serverID - 1].channels[channelID - 1].port);
	}
}

void Packet_TransferManager::sendPacket_Transfer(User* user, unsigned long ip, unsigned short port) {
	auto packet = TCPConnection::Packet::Create(PacketSource::Server, user->GetConnection(), {PacketID::Transfer});

	packet->WriteUInt32_LE(ip);
	packet->WriteUInt16_LE(port);
	packet->WriteUInt8(0); // unk
	packet->WriteString(user->GetUserName());

	packet->Send();
}