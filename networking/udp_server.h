#pragma once
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <map>
#include <queue>

using namespace std;

constexpr auto UDP_CLIENT_PACKET_SIGNATURE = 'W';
constexpr auto UDP_RELAY_PACKET_SIGNATURE = 'Y';
constexpr auto UDP_PING_PACKET_SIGNATURE = '`';
constexpr auto UDP_SERVER_PACKET_SIGNATURE = 'Z';
constexpr auto UDP_PACKET_MAX_SIZE = 4010;
constexpr auto UDP_CLIENT_HEARTBEAT_TIMEOUT = 15;
constexpr auto UDP_SERVER_HEARTBEAT_TIMEOUT = 3;

enum ClientPacketType {
	SocketInfo = 0,
	Heartbeat = 1
};

enum PingPacketType {
	Request = 0,
	Reply = 1
};

enum ServerPacketType {
	HELLO = 0,
	HELLO_ANSWER = 1,
	GOODBYE = 2,
	HEARTBEAT = 3,
	NUMPLAYERS = 4
};

struct PingRequest {
	boost::asio::ip::udp::endpoint requestEndpoint;
	boost::asio::ip::udp::endpoint replyEndpoint;
	unsigned long long timestamp;
};

class UDPPacket {
public:
	UDPPacket(boost::asio::ip::udp::endpoint endpoint, vector<unsigned char> buffer);

	const boost::asio::ip::udp::endpoint& GetEndpoint() const noexcept {
		return _endpoint;
	}

	void SetBuffer(const vector<unsigned char> buffer) noexcept {
		_buffer = buffer;
	}
	const vector<unsigned char>& GetBuffer() const noexcept {
		return _buffer;
	}

	unsigned char GetSignature() const noexcept {
		return _signature;
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
		union { float fnum; unsigned inum; } u{};
		u.fnum = val;
		WriteUInt32_LE(u.inum);
	}
	void WriteFloat_BE(float val) noexcept {
		union { float fnum; unsigned inum; } u{};
		u.fnum = val;
		WriteUInt32_BE(u.inum);
	}
	void WriteDouble_LE(double val) noexcept {
		union { double fnum; unsigned long long inum; } u{};
		u.fnum = val;
		WriteUInt64_LE(u.inum);
	}
	void WriteDouble_BE(double val) noexcept {
		union { double fnum; unsigned long long inum; } u{};
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
		while (curChar != '\0') {
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

private:
	vector<unsigned char> _buffer;
	boost::asio::ip::udp::endpoint _endpoint;
	unsigned char _signature;

	int _readOffset;
	int _writeOffset;
};

class UDPServer {
public:
	UDPServer();
	~UDPServer();

	const boost::asio::ip::udp::socket& GetSocket() const noexcept {
		return _socket;
	}

	bool Init(unsigned short port);
	void Start();
	void Stop();
	void SendNumPlayersPacketToAll();
	void OnSecondTick();

private:
	int run();
	int shutdown();
	void startReceive();
	void handleReceive(const boost::system::error_code& ec, size_t bytes_transferred);
	void handleIncomingClientPacket();
	void handleIncomingServerPacket();
	void handleOutgoingClientPacket();
	void handleOutgoingServerPacket();
	void sendPacket(UDPPacket* packet, bool isServer = false);
	void sendClientPacket(const boost::asio::ip::udp::endpoint& endpoint, const string& userLogName);
	void sendRelayPacket(const boost::asio::ip::udp::endpoint& endpoint, vector<unsigned char> buffer, const string& userLogName);
	void sendPingPacket(const boost::asio::ip::udp::endpoint& endpoint, const vector<unsigned char>& buffer);
	void sendHelloAnswerPacket(const boost::asio::ip::udp::endpoint& endpoint);
	void sendHelloPacketToAll();
	void sendGoodbytePacketToAll();
	void sendHeartbeatPacket(unsigned char serverID, unsigned char channelID);

	unsigned short _port;

	thread _udpServerThread;
	boost::asio::io_context _ioContext;
	boost::asio::ip::udp::socket _socket;
	boost::asio::ip::udp::endpoint _endpoint;
	array<unsigned char, UDP_PACKET_MAX_SIZE> _recvBuffer;

	mutex _clientIncomingMutex;
	queue<UDPPacket*> _clientIncomingQueue;

	mutex _clientOutgoingMutex;
	queue<UDPPacket*> _clientOutgoingQueue;

	mutex _serverIncomingMutex;
	queue<UDPPacket*> _serverIncomingQueue;

	mutex _serverOutgoingMutex;
	queue<UDPPacket*> _serverOutgoingQueue;

	int _pendingGoodbyePackets;
	bool _isShuttingDown;

	vector<PingRequest> _pingRequests;
	unsigned long long _currentTime;
};

extern UDPServer udpServer;