#include "packet_transfermanager.h"
#include "packet_serverlistmanager.h"
#include "usermanager.h"
#include <iostream>

Packet_TransferManager packet_TransferManager;

void Packet_TransferManager::ParsePacket_TransferLogin(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_TransferManager] Parsing Packet_TransferLogin from client ({})\n", packet->GetConnection()->GetEndPoint());

	string unk1 = packet->ReadString();
	vector<unsigned char> hardwareID = packet->ReadArray_UInt8(16);
	unsigned long pcBang = packet->ReadUInt32_LE();
	unsigned char unk2 = packet->ReadUInt8();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_TransferLogin - unk1: {}, hardwareID:{}, pcBang: {}, unk2: {}\n", packet->GetConnection()->GetEndPoint(), unk1.c_str(), hardwareIDStr.c_str(), pcBang, unk2);
}

void Packet_TransferManager::ParsePacket_RequestTransfer(TCPConnection::Packet::pointer packet) {
	cout << format("[Packet_TransferManager] Parsing Packet_RequestTransfer from client ({})\n", packet->GetConnection()->GetEndPoint());

	unsigned char serverID = packet->ReadUInt8();
	unsigned char channelID = packet->ReadUInt8();

	cout << format("[Packet_TransferManager] Client ({}) has sent Packet_RequestTransfer - serverID: {}, channelID: {}\n", packet->GetConnection()->GetEndPoint(), serverID, channelID);

	vector<User*> users = userManager.GetUsers();
	vector<UserCharacter> userCharacters;
	users.erase(
		remove_if(
			users.begin(),
			users.end(),
			[&userCharacters](User* user) -> bool {
				UserCharacter userCharacter;
				userCharacter.flag = UserInfoFlag::All;

				if (!user->GetCharacter(userCharacter)) {
					return true;
				}

				userCharacters.push_back(userCharacter);
				return false;
			}
		),
		users.end()
	);

	packet_ServerListManager.SendPacket_Lobby(packet->GetConnection(), users, userCharacters);
	packet_ServerListManager.SendPacket_RoomList(packet->GetConnection());
}