#pragma once
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>

using namespace std;

#define UDP_PACKET_SIGNATURE 'W'
#define UDP_PACKET_MAX_SIZE 4010

class UDPServer {
public:
	UDPServer();
	~UDPServer();

	bool Init(unsigned short port);
	void Start();
	void Stop();

	unsigned char ReadUInt8() noexcept {
		return ReadBytes<unsigned char>();
	}
	unsigned short ReadUInt16_LE() noexcept {
		return ReadBytes<unsigned short>();
	}
	unsigned int ReadUInt32_LE() noexcept {
		return ReadBytes<unsigned int>();
	}
	unsigned int ReadUInt32_BE() noexcept {
		return ReadBytes<unsigned int>(false);
	}

	template <class T> T ReadBytes(bool LE = true) {
		T result = 0;
		unsigned int size = sizeof(T);

		// Do not overflow
		if (_readOffset + size > (int)_recvBuffer.size())
			return result;

		char* dst = (char*)&result;
		char* src = (char*)&_recvBuffer[_readOffset];

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

private:
	int run();
	int shutdown();
	void startReceive();
	void handleReceive(const boost::system::error_code& ec, size_t bytes_transferred);
	void handleSend(shared_ptr<vector<unsigned char>> message, const boost::system::error_code& ec, size_t bytes_transferred);

	unsigned short _port;

	thread _udpServerThread;
	boost::asio::io_service _ioService;
	boost::asio::ip::udp::socket _socket;
	boost::asio::ip::udp::endpoint _endpoint;
	array<unsigned char, UDP_PACKET_MAX_SIZE> _recvBuffer;
	int _readOffset;
};

extern UDPServer udpServer;