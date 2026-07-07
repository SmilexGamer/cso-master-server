#include "packet_loginmanager.h"
#include "packet_charactermanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include "serverconsole.h"
#include "databasemanager.h"

Packet_LoginManager packet_LoginManager;

void Packet_LoginManager::ParsePacket_Login(TCPConnection::Packet::pointer packet) {
	if (packet == NULL) {
		return;
	}

	auto& connection = packet->GetConnection();
	if (connection == NULL) {
		return;
	}

	if (!connection->IsVersionReceived()) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_LoginManager ] Client ({}) has sent Packet_Login, but it hasn't sent Packet_Version!\n", connection->GetLogEndpoint()));
		return;
	}

	User* user = userManager.GetUserByConnection(connection);
	if (userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_LoginManager ] User ({}) has sent Packet_Login, but it's already logged in!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_LoginManager ] Parsing Packet_Login from client ({})\n", connection->GetLogEndpoint()));

	const string& userName = packet->ReadString();
	const string& password = packet->ReadString();
	const vector<unsigned char>& hardwareID = packet->ReadArray_UInt8(HARDWARE_ID_SIZE);
	unsigned long pcBang = packet->ReadUInt32_LE();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {:02X}", c);
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_LoginManager ] Client ({}) has sent Packet_Login - userName: {}, password: {}, hardwareID:{}, pcBang: {}\n", connection->GetLogEndpoint(), userName, password, hardwareIDStr, pcBang));

	const LoginResult& loginResult = databaseManager.Login(userName, password);
	if (loginResult.reply > Packet_ReplyType::LoginSuccess) {
		packetManager.SendPacket_Reply(connection, loginResult.reply);
		return;
	}

	User* newUser = new User(connection, loginResult.userID, userName);
	char userResult = userManager.AddUser(newUser);
	if (!userResult) {
		if (userResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);

			delete newUser;
			newUser = NULL;
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::Playing);

		delete newUser;
		newUser = NULL;
		return;
	}

	if (userResult == Packet_ReplyType::EXCEED_MAX_CONNECTION) {
		packetManager.SendPacket_Reply(connection, Packet_ReplyType::EXCEED_MAX_CONNECTION);

		delete newUser;
		newUser = NULL;
		return;
	}

	char userCharacterExistsResult = newUser->IsUserCharacterExists();
	if (!userCharacterExistsResult) {
		if (userCharacterExistsResult < 0) {
			packetManager.SendPacket_Reply(connection, Packet_ReplyType::SysError);

			userManager.RemoveUser(newUser);
			return;
		}

		packetManager.SendPacket_Reply(connection, Packet_ReplyType::LoginSuccess);
		packet_CharacterManager.SendPacket_Character(connection);
		return;
	}

	userManager.SendLoginPackets(newUser, Packet_ReplyType::LoginSuccess);
}