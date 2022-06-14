#include "tcp_connection.h"
#include <iostream>

TCPConnection::Packet::Packet(PacketSource source, TCPConnection::pointer connection, std::vector<unsigned char> buffer) : _source(source), _connection(connection), _buffer(buffer) {
	_readOffset = 0;
	_writeOffset = 0;

	_signature = source == PacketSource::Client ? ReadUInt8() : PacketSignature;
	_number = source == PacketSource::Client ? ReadUInt8() : _connection->_outgoingPacketNumber++;
	_length = source == PacketSource::Client ? ReadUInt16_LE() : (unsigned short)_buffer.size();
}

TCPConnection::Packet::~Packet() {
	if (_source == PacketSource::Client && _readOffset < _length + 4)
	{
		std::cout << std::format("[TCPConnection::Packet] Packet from {} has unread data (_readOffset: {}, _length: {}):", _connection->GetEndPoint(), _readOffset, _length);

		for (auto c : ReadArray_UInt8(_length + 4 - _readOffset))
		{
			std::cout << std::format(" {}", c & 0xFF);
		}

		std::cout << std::endl;
	}
}

TCPConnection::TCPConnection(io::ip::tcp::socket&& socket) : _socket(std::move(socket)) {
	std::stringstream endpoint;
	endpoint << _socket.remote_endpoint();

	_endpoint = endpoint.str();
}

void TCPConnection::Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler) {
	_packetHandler = std::move(packetHandler);
	_errorHandler = std::move(errorHandler);

	asyncRead();
}

void TCPConnection::WritePacket(const std::vector<unsigned char>& buffer) {
	bool queueIdle = _outgoingPackets.empty();
	_outgoingPackets.push(buffer);

	if (queueIdle) {
		asyncWrite();
	}
}

void TCPConnection::asyncRead() {
	io::async_read(_socket, _streamBuf, io::transfer_exactly(4), [self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		self->onRead(ec, bytesTransferred);
	});
}

void TCPConnection::onRead(boost::system::error_code ec, size_t bytesTransferred) {
	if (ec) {
		_socket.close(ec);

		_errorHandler();
		return;
	}

	std::vector<unsigned char> buffer(bytesTransferred);
	buffer_copy(io::buffer(buffer), _streamBuf.data());

	auto packet = TCPConnection::Packet::Create(PacketSource::Client, shared_from_this(), buffer);

	if (!packet->IsValid()) {
		std::cout << std::format("[TCPConnection] Client ({}) sent packet with invalid signature!", _endpoint) << std::endl;
		_streamBuf.consume(bytesTransferred);
		asyncRead();
	} else if (!packet->GetLength()) {
		std::cout << std::format("[TCPConnection] Client ({}) sent packet with size 0!", _endpoint) << std::endl;
		_streamBuf.consume(bytesTransferred);
		asyncRead();
	} else {
		io::async_read(_socket, _streamBuf, io::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
		(boost::system::error_code ec, size_t bytesTransferred) {
			if (ec) {
				self->_socket.close(ec);

				self->_errorHandler();
				return;
			}

			std::vector<unsigned char> buffer(4 + bytesTransferred);
			buffer_copy(io::buffer(buffer), self->_streamBuf.data());
			self->_streamBuf.consume(4 + bytesTransferred);

			packet->SetBuffer(buffer);

			std::cout << std::format("[TCPConnection] Received packet from {}:", self->GetEndPoint());

			for (auto c : buffer)
			{
				std::cout << std::format(" {}", c & 0xFF);
			}

			std::cout << std::endl;

			self->_packetHandler(packet);
			self->asyncRead();
		});
	}
}

void TCPConnection::asyncWrite() {
	io::async_write(_socket, io::buffer(_outgoingPackets.front()), [self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		self->onWrite(ec, bytesTransferred);
	});
}

void TCPConnection::onWrite(boost::system::error_code ec, size_t bytesTransferred) {
	if (ec) {
		_socket.close(ec);

		_errorHandler();
		return;
	}

	std::cout << std::format("[TCPConnection] Sent packet to {}:", GetEndPoint());

	for (auto c : _outgoingPackets.front())
	{
		std::cout << std::format(" {}", c & 0xFF);
	}

	std::cout << std::endl;

	_outgoingPackets.pop();

	if (!_outgoingPackets.empty()) {
		asyncWrite();
	}
}