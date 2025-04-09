#include "packet_loginmanager.h"
#include "packet_charactermanager.h"
#include "packet_cryptmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "serverconfig.h"
#include "serverconsole.h"
#include "databasemanager.h"

Packet_LoginManager packet_LoginManager;

void Packet_LoginManager::ParsePacket_Login(TCPConnection::Packet::pointer packet) {
	if (!packet->GetConnection()->IsVersionReceived()) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_LoginManager ] Client ({}) has sent Packet_Login, but it hasn't sent Packet_Version!\n", packet->GetConnection()->GetIPAddress()));
		return;
	}

	User* user = userManager.GetUserByConnection(packet->GetConnection());
	if (userManager.IsUserLoggedIn(user)) {
		serverConsole.Print(PrefixType::Warn, format("[ Packet_LoginManager ] User ({}) has sent Packet_Login, but it's already logged in!\n", user->GetUserLogName()));
		return;
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_LoginManager ] Parsing Packet_Login from client ({})\n", packet->GetConnection()->GetIPAddress()));

	const string& userName = packet->ReadString();
	const string& password = packet->ReadString();
	const vector<unsigned char>& hardwareID = packet->ReadArray_UInt8(HARDWARE_ID_SIZE);
	unsigned long pcBang = packet->ReadUInt32_LE();

	string hardwareIDStr;
	for (auto& c : hardwareID) {
		hardwareIDStr += format(" {}{:X}", c < 0x10 ? "0" : "", c);
	}

	serverConsole.Print(PrefixType::Info, format("[ Packet_LoginManager ] Client ({}) has sent Packet_Login - userName: {}, password: {}, hardwareID:{}, pcBang: {}\n", packet->GetConnection()->GetIPAddress(), userName, password, hardwareIDStr, pcBang));

	if (userManager.GetUsers().size() >= serverConfig.maxPlayers) {
		packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::EXCEED_MAX_CONNECTION);
		return;
	}

	const LoginResult& loginResult = databaseManager.Login(userName, password);
	if (loginResult.reply > Packet_ReplyType::LoginSuccess) {
		packetManager.SendPacket_Reply(packet->GetConnection(), loginResult.reply);
		return;
	}

	User* newUser = new User(packet->GetConnection(), loginResult.userID, userName);
	char userResult = userManager.AddUser(newUser);
	if (!userResult) {
		if (userResult < 0) {
			packetManager.SendPacket_Reply(packet->GetConnection(), Packet_ReplyType::SysError);

			delete newUser;
			newUser = NULL;
			return;
		}

		packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::Playing);

		delete newUser;
		newUser = NULL;
		return;
	}

#ifdef NO_SSL
	if (newUser->GetConnection()->SetupEncryptCipher(serverConfig.encryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(newUser->GetConnection(), CipherType::Encrypt, newUser->GetConnection()->GetEncryptCipher());
	}

	if (newUser->GetConnection()->SetupDecryptCipher(serverConfig.decryptCipherMethod)) {
		packet_CryptManager.SendPacket_Crypt(newUser->GetConnection(), CipherType::Decrypt, newUser->GetConnection()->GetDecryptCipher());
	}
#endif

	char userCharacterExistsResult = newUser->IsUserCharacterExists();
	if (!userCharacterExistsResult) {
		if (userCharacterExistsResult < 0) {
			packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::SysError);

			userManager.RemoveUser(newUser);
			return;
		}

		packetManager.SendPacket_Reply(newUser->GetConnection(), Packet_ReplyType::LoginSuccess);
		packet_CharacterManager.SendPacket_Character(newUser->GetConnection());
		return;
	}

	userManager.SendLoginPackets(newUser, Packet_ReplyType::LoginSuccess);
}