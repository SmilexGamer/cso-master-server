#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <queue>

using namespace std;

//#define NO_SSL

#define SERVERCONNECTED "~SERVERCONNECTED\n\0"

#define TCP_PACKET_SIGNATURE 'U'

#define PACKET_HEADER_SIZE 4

#define TCP_PACKET_MAX_SIZE 0x10000

#define KEY_SIZE 32

#define BLOCK_SIZE 32

enum class PacketSource {
	Client,
	Server
};

enum class PacketID {
	Version = 0,
	Reply = 1,
	Transfer = 2,
	RecvCharacter = 2,
	Login = 3, // or Auth, whatever you want to name it
	ServerList = 5,
	TransferLogin = 5,
	Character = 6,
	Crypt = 7,
	RequestTransfer = 7,
	RequestServerList = 10,
	RecvCrypt = 12,
	Room = 65,
	ClientCheck = 66,
	UMsg = 67,
	Host = 68,
	UpdateInfo = 69,
	Udp = 70,
	Clan = 71,
	Shop = 72,
	Rank = 73,
	Option = 76,
	Favorite = 77,
	Item = 78,
	GameGuard = 79,
	SearchRoom = 80,
	Report = 83,
	UserStart = 150,
	RoomList = 151,
	Lobby = 153,
	Inventory = 154,
	ClanStock = 155,
	CafeItems = 156
};

enum Packet_ReplyType {
	NoReply = -1,
	LoginSuccess = 0,
	CreateCharacterSuccess = 1,
	InvalidName = 3,
	Playing = 4,
	NotExist = 5,
	WrongPassword = 7,
	InvalidPassword = 8,
	ServerDown = 9,
	SysError = 10,
	InvalidServer = 11,
	InvalidSlot = 12,
	ID_NOT_ALLOWD = 14,
	NEXONCOMERROR = 15,
	NEXONCOM_PASSPORTERROR = 16,
	NEXONCOM_INVALID_SESSION = 17,
	INVALID_USERINFO = 18,
	TRANSFER_ERR = 19,
	ID_TOO_SHORT = 20,
	ID_TOO_LONG = 21,
	INVALID_CHAR = 22,
	ID_DIGIT_BEFORE_CHAR = 23,
	ID_EXCEED_CHAR_COUNT = 24,
	ALREADY_EXIST = 25,
	ID_PROHIBITED = 26,
	EXCEED_MAX_CONNECTION = 27,
	NEXONCOM_UNDER_AGE = 28,
	INVALID_CLIENT_VERSION = 38
};

enum CipherMethod {
	CleanUp = 0,
	CleanUp2 = 1,
	RC4 = 2,
	RC4_40 = 3,
	Null = 4
};

struct Cipher {
	CipherMethod method = CipherMethod::CleanUp;
	EVP_CIPHER_CTX* ctx = NULL;
	unsigned char key[KEY_SIZE] = {};
	unsigned char iv[BLOCK_SIZE] = {};
};

class TCPConnection : public enable_shared_from_this<TCPConnection> {
public:
	~TCPConnection();

	using pointer = shared_ptr<TCPConnection>;

	class Packet : public enable_shared_from_this<TCPConnection::Packet> {
	public:
		~Packet();

		using pointer = shared_ptr<TCPConnection::Packet>;

		static pointer Create(PacketSource source, TCPConnection::pointer connection, vector<unsigned char> buffer = {}) {
			return pointer(new TCPConnection::Packet(source, connection, buffer));
		}

		const TCPConnection::pointer GetConnection() const noexcept {
			return _connection;
		}

		void WriteHeader() noexcept {
			_buffer.insert(_buffer.begin(), _length >> 8);
			_buffer.insert(_buffer.begin(), (unsigned char)_length);
			_buffer.insert(_buffer.begin(), _sequence);
			_buffer.insert(_buffer.begin(), _signature);
		}

		bool IsValid() const noexcept {
			return _signature == TCP_PACKET_SIGNATURE;
		}

		unsigned char GetSequence() const noexcept {
			return _sequence;
		}

		unsigned short GetLength() const noexcept {
			return _length;
		}

		void SetBuffer(const vector<unsigned char> buffer) noexcept {
			_buffer = buffer;
		}
		const vector<unsigned char>& GetBuffer() const noexcept {
			return _buffer;
		}

		string ByteStr(bool LE) const noexcept {
			stringstream byteStr;
			byteStr << hex << setfill('0');

			if (LE == true) {
				for (unsigned long long i = 0; i < _buffer.size(); ++i)
					byteStr << setw(2) << (unsigned short)_buffer[i] << " ";
			}
			else {
				unsigned long long size = _buffer.size();
				for (unsigned long long i = 0; i < size; ++i)
					byteStr << setw(2) << (unsigned short)_buffer[size - i - 1] << " ";
			}

			return byteStr.str();
		}

		template <class T> void WriteBytes(const T& val, bool LE = true) {
			unsigned int size = sizeof(T);

			if (LE == true) {
				for (unsigned int i = 0, mask = 0; i < size; ++i, mask += 8)
					_buffer.insert(_buffer.begin() + _writeOffset++, (unsigned char)(val >> mask));
			}
			else {
				unsigned const char* array = reinterpret_cast<unsigned const char*>(&val);
				for (unsigned int i = 0; i < size; ++i)
					_buffer.insert(_buffer.begin() + _writeOffset++, array[size - i - 1]);
			}
			_length += size;
		}

		void SetWriteOffset(int newOffset) noexcept {
			_writeOffset = newOffset;
		}
		int GetWriteOffset() const noexcept {
			return _writeOffset;
		}

		void WriteBool(bool val) noexcept {
			WriteBytes<bool>(val);
		}
		void WriteString(const string& str) noexcept {
			for (const unsigned char& s : str) WriteInt8(s);
			WriteInt8('\0');
		}
		void WriteInt8(char val) noexcept {
			WriteBytes<char>(val);
		}
		void WriteUInt8(unsigned char val) noexcept {
			WriteBytes<unsigned char>(val);
		}

		void WriteInt16_LE(short val) noexcept {
			WriteBytes<short>(val);
		}
		void WriteInt16_BE(short val) noexcept {
			WriteBytes<short>(val, false);
		}
		void WriteUInt16_LE(unsigned short val) noexcept {
			WriteBytes<unsigned short>(val);
		}
		void WriteUInt16_BE(unsigned short val) noexcept {
			WriteBytes<unsigned short>(val, false);
		}

		void WriteInt32_LE(int val) noexcept {
			WriteBytes<int>(val);
		}
		void WriteInt32_BE(int val) noexcept {
			WriteBytes<int>(val, false);
		}
		void WriteUInt32_LE(unsigned int val) noexcept {
			WriteBytes<unsigned int>(val);
		}
		void WriteUInt32_BE(unsigned int val) noexcept {
			WriteBytes<unsigned int>(val, false);
		}

		void WriteInt64_LE(long long val) noexcept {
			WriteBytes<long long>(val);
		}
		void WriteInt64_BE(long long val) noexcept {
			WriteBytes<long long>(val, false);
		}
		void WriteUInt64_LE(unsigned long long val) noexcept {
			WriteBytes<unsigned long long>(val);
		}
		void WriteUInt64_BE(unsigned long long val) noexcept {
			WriteBytes<unsigned long long>(val, false);
		}

		void WriteFloat_LE(float val) noexcept {
			union { float fnum; unsigned inum; } u {};
			u.fnum = val;
			WriteUInt32_LE(u.inum);
		}
		void WriteFloat_BE(float val) noexcept {
			union { float fnum; unsigned inum; } u {};
			u.fnum = val;
			WriteUInt32_BE(u.inum);
		}
		void WriteDouble_LE(double val) noexcept {
			union { double fnum; unsigned long long inum; } u {};
			u.fnum = val;
			WriteUInt64_LE(u.inum);
		}
		void WriteDouble_BE(double val) noexcept {
			union { double fnum; unsigned long long inum; } u {};
			u.fnum = val;
			WriteUInt64_BE(u.inum);
		}

		void WriteArray_Int8(const vector<char>& vec) noexcept {
			for (const char& v : vec) WriteInt8(v);
		}
		void WriteArray_UInt8(const vector<unsigned char>& vec) noexcept {
			for (const unsigned char& v : vec) WriteUInt8(v);
		}

		void SetReadOffset(int newOffset) noexcept {
			_readOffset = newOffset;
		}
		int GetReadOffset() const noexcept {
			return _readOffset;
		}
		template <class T> T ReadBytes(bool LE = true) {
			T result = 0;
			unsigned int size = sizeof(T);

			// Do not overflow
			if (_readOffset + size > (int)_buffer.size())
				return result;

			char* dst = (char*)&result;
			char* src = (char*)&_buffer[_readOffset];

			if (LE == true) {
				for (unsigned int i = 0; i < size; ++i)
					dst[i] = src[i];
			}
			else {
				for (unsigned int i = 0; i < size; ++i)
					dst[i] = src[size - i - 1];
			}
			_readOffset += size;
			return result;
		}

		bool ReadBool() noexcept {
			return ReadBytes<bool>();
		}
		string ReadString() noexcept {
			string result;

			char curChar = ReadInt8();
			while(curChar != '\0') {
				result += curChar;
				curChar = ReadInt8();
			}

			return result;
		}
		char ReadInt8() noexcept {
			return ReadBytes<char>();
		}
		unsigned char ReadUInt8() noexcept {
			return ReadBytes<unsigned char>();
		}

		short ReadInt16_LE() noexcept {
			return ReadBytes<short>();
		}
		short ReadInt16_BE() noexcept {
			return ReadBytes<short>(false);
		}
		unsigned short ReadUInt16_LE() noexcept {
			return ReadBytes<unsigned short>();
		}
		unsigned short ReadUInt16_BE() noexcept {
			return ReadBytes<unsigned short>(false);
		}

		int ReadInt32_LE() noexcept {
			return ReadBytes<int>();
		}
		int ReadInt32_BE() noexcept {
			return ReadBytes<int>(false);
		}
		unsigned int ReadUInt32_LE() noexcept {
			return ReadBytes<unsigned int>();
		}
		unsigned int ReadUInt32_BE() noexcept {
			return ReadBytes<unsigned int>(false);
		}

		long long ReadInt64_LE() noexcept {
			return ReadBytes<long long>();
		}
		long long ReadInt64_BE() noexcept {
			return ReadBytes<long long>(false);
		}
		unsigned long long ReadUInt64_LE() noexcept {
			return ReadBytes<unsigned long long>();
		}
		unsigned long long ReadUInt64_BE() noexcept {
			return ReadBytes<unsigned long long>(false);
		}

		float ReadFloat_LE() noexcept {
			return ReadBytes<float>();
		}
		float ReadFloat_BE() noexcept {
			return ReadBytes<float>(false);
		}
		double ReadDouble_LE() noexcept {
			return ReadBytes<double>();
		}
		double ReadDouble_BE() noexcept {
			return ReadBytes<double>(false);
		}

		vector<char> ReadArray_Int8(int len) noexcept {
			vector<char> result;

			while (len) {
				result.push_back(ReadInt8());
				len--;
			}

			return result;
		}
		vector<unsigned char> ReadArray_UInt8(int len) noexcept {
			vector<unsigned char> result;

			while (len) {
				result.push_back(ReadUInt8());
				len--;
			}

			return result;
		}

		void Send() {
			WriteHeader();
#ifdef NO_SSL
			_connection->WritePacket(_buffer, true);
#else
			_connection->WritePacket(_buffer);
#endif
		}

	private:
		explicit Packet(PacketSource source, TCPConnection::pointer connection, vector<unsigned char> buffer);

	private:
		PacketSource _source;
		TCPConnection::pointer _connection;
		vector<unsigned char> _buffer;

		unsigned char _signature;
		unsigned char _sequence;
		unsigned short _length;

		int _readOffset;
		int _writeOffset;
	};

	using PacketHandler = function<void(TCPConnection::Packet::pointer)>;
	using ErrorHandler = function<void(bool eraseConnection)>;

	static pointer Create(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& context) {
		return pointer(new TCPConnection(move(socket), context));
	}

	const boost::asio::ip::tcp::socket& GetSocket() noexcept {
		return _sslStream.next_layer();
	}

	const string& GetIPAddress() const noexcept {
		return _ipAddress;
	}

	unsigned char GetOutgoingSequence() const noexcept {
		return _outgoingSequence;
	}

	void SetOutgoingSequence(unsigned char outgoingSequence) noexcept {
		_outgoingSequence = outgoingSequence;
	}

	unsigned char GetIncomingSequence() const noexcept {
		return _incomingSequence;
	}

	void SetIncomingSequence(unsigned char incomingSequence) noexcept {
		_incomingSequence = incomingSequence;
	}

	Cipher GetDecryptCipher() const noexcept {
		return _decryptCipher;
	}

	Cipher GetEncryptCipher() const noexcept {
		return _encryptCipher;
	}

	void Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler);
	void WritePacket(const vector<unsigned char>& buffer, bool noSSL = false);
	void DisconnectClient(bool eraseConnection = true);
	void DisconnectClient(boost::system::error_code ec);
	bool SetupDecryptCipher(CipherMethod method);
	bool SetupEncryptCipher(CipherMethod method);

private:
	explicit TCPConnection(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& context);

	bool decrypt(vector<unsigned char>& buffer);

	void asyncRead();
	void onRead(boost::system::error_code ec, size_t bytesTransferred);

	bool encrypt(vector<unsigned char>& buffer);

	void asyncWrite(bool noSSL = false);
	void onWrite(boost::system::error_code ec, size_t bytesTransferred, vector<unsigned char> buffer);

private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _sslStream;
	string _ipAddress;

	queue<vector<unsigned char>> _outgoingPackets;
	unsigned char _outgoingSequence = 0;
	boost::asio::streambuf _streamBuf { PACKET_HEADER_SIZE + UINT16_MAX };
	unsigned char _incomingSequence = 0;

	bool _decrypt = false;
	Cipher _decryptCipher;

	bool _encrypt = false;
	Cipher _encryptCipher;

	PacketHandler _packetHandler;
	ErrorHandler _errorHandler;
};