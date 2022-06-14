#pragma once
#include <boost/asio.hpp>
#include <queue>
#include <vector>

using boost::asio::ip::tcp;
namespace io = boost::asio;

enum class PacketSource {
	Client,
	Server
};

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
public:
	using pointer = std::shared_ptr<TCPConnection>;

	class Packet : public std::enable_shared_from_this<TCPConnection::Packet> {
	public:
		~Packet();

		using pointer = std::shared_ptr<TCPConnection::Packet>;

		static pointer Create(PacketSource source, TCPConnection::pointer connection, std::vector<unsigned char> buffer = {}) {
			return pointer(new TCPConnection::Packet(source, connection, buffer));
		}

		const unsigned char& PacketSignature = 'U';

		TCPConnection::pointer GetConnection() {
			return _connection;
		}

		void WriteHeader() {
			_buffer.insert(_buffer.begin(), _length << 8);
			_buffer.insert(_buffer.begin(), _length);
			_buffer.insert(_buffer.begin(), _number);
			_buffer.insert(_buffer.begin(), _signature);

			if (_connection->_outgoingPacketNumber > 255)
				_connection->_outgoingPacketNumber = 1;
		}

		bool IsValid() {
			return _signature == PacketSignature;
		}

		unsigned char GetNumber() {
			return _number;
		}

		unsigned short GetLength() {
			return _length;
		}

		void SetBuffer(std::vector<unsigned char> buffer) noexcept {
			_buffer = buffer;
		}
		const std::vector<unsigned char> GetBuffer() const noexcept {
			return _buffer;
		}

		std::string ByteStr(bool LE) const noexcept {
			std::stringstream byteStr;
			byteStr << std::hex << std::setfill('0');

			if (LE == true) {
				for (unsigned long long i = 0; i < _buffer.size(); ++i)
					byteStr << std::setw(2) << (unsigned short)_buffer[i] << " ";
			}
			else {
				unsigned long long size = _buffer.size();
				for (unsigned long long i = 0; i < size; ++i)
					byteStr << std::setw(2) << (unsigned short)_buffer[size - i - 1] << " ";
			}

			return byteStr.str();
		}

		template <class T> void WriteBytes(const T& val, bool LE = true) {
			unsigned int size = sizeof(T);

			if (LE == true) {
				for (unsigned int i = 0, mask = 0; i < size; ++i, mask += 8)
					_buffer.push_back(val >> mask);
			}
			else {
				unsigned const char* array = reinterpret_cast<unsigned const char*>(&val);
				for (unsigned int i = 0; i < size; ++i)
					_buffer.push_back(array[size - i - 1]);
			}
			_writeOffset += size;
			_length += size;
		}

		int GetWriteOffset() const noexcept {
			return _writeOffset;
		}

		void WriteBool(bool val) noexcept {
			WriteBytes<bool>(val);
		}
		void WriteString(const std::string& str) noexcept {
			for (const unsigned char& s : str) WriteInt8(s);
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

		void WriteArray_Int8(const std::vector<char>& vec) noexcept {
			for (const char& v : vec) WriteInt8(v);
		}
		void WriteArray_UInt8(const std::vector<unsigned char>& vec) noexcept {
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
		std::string ReadString() noexcept {
			std::string result;

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

		std::vector<char> ReadArray_Int8(int len) noexcept {
			std::vector<char> result;

			while (len) {
				result.push_back(ReadInt8());
				len--;
			}

			return result;
		}
		std::vector<unsigned char> ReadArray_UInt8(int len) noexcept {
			std::vector<unsigned char> result;

			while (len) {
				result.push_back(ReadUInt8());
				len--;
			}

			return result;
		}

		void Send() {
			WriteHeader();
			_connection->WritePacket(_buffer);
		}

	private:
		explicit Packet(PacketSource source, TCPConnection::pointer connection, std::vector<unsigned char> buffer);

	private:
		PacketSource _source;
		TCPConnection::pointer _connection;
		std::vector<unsigned char> _buffer;

		unsigned char _signature;
		unsigned char _number;
		unsigned short _length;

		int _readOffset;
		int _writeOffset;
	};

	using PacketHandler = std::function<void(TCPConnection::Packet::pointer)>;
	using ErrorHandler = std::function<void()>;

	static pointer Create(io::ip::tcp::socket&& socket) {
		return pointer(new TCPConnection(std::move(socket)));
	}

	tcp::socket& Socket() {
		return _socket;
	}

	const std::string& GetEndPoint() const {
		return _endpoint;
	}

	const std::string& WelcomeMessage{ "~SERVERCONNECTED\n\0" };

	void Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler);
	void WritePacket(const std::vector<unsigned char>& buffer);

private:
	explicit TCPConnection(io::ip::tcp::socket&& socket);

	void asyncRead();
	void onRead(boost::system::error_code ec, size_t bytesTransferred);

	void asyncWrite();
	void onWrite(boost::system::error_code ec, size_t bytesTransferred);

private:
	tcp::socket _socket;
	std::string _endpoint;

	std::queue<std::vector<unsigned char>> _outgoingPackets;
	unsigned char _outgoingPacketNumber = 1;
	io::streambuf _streamBuf {4 + 65536};

	PacketHandler _packetHandler;
	ErrorHandler _errorHandler;
};