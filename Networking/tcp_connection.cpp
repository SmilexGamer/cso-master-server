#include "tcp_connection.h"
#include <iostream>

TCPConnection::Packet::Packet(PacketSource source, TCPConnection::pointer connection, vector<unsigned char> buffer) : _source(source), _connection(connection), _buffer(buffer) {
	_readOffset = 0;
	_writeOffset = 0;

	_signature = source == PacketSource::Client ? ReadUInt8() : PacketSignature;
	_number = source == PacketSource::Client ? ReadUInt8() : _connection->_outgoingPacketNumber;
	_length = source == PacketSource::Client ? ReadUInt16_LE() : (unsigned short)_buffer.size();

	if (source == PacketSource::Server) {
		if (_connection->_outgoingPacketNumber == 255) {
			_connection->_outgoingPacketNumber = 1;
		}
		else {
			_connection->_outgoingPacketNumber++;
		}
	}
}

TCPConnection::Packet::~Packet() {
	if (_source == PacketSource::Client && _readOffset < _length + 4) {
		cout << format("[TCPConnection::Packet] Packet from {} has unread data (_readOffset: {}, _length: {}):", _connection->GetEndPoint(), _readOffset, _length);

		for (auto c : ReadArray_UInt8(_length + 4 - _readOffset)) {
			cout << format(" {}", c & 0xFF);
		}

		cout << endl;
	}
}

TCPConnection::TCPConnection(io::ip::tcp::socket&& socket) : _socket(move(socket)) {
	stringstream endpoint;
	endpoint << _socket.remote_endpoint();

	_endpoint = endpoint.str();
}

void TCPConnection::Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler) {
	_packetHandler = move(packetHandler);
	_errorHandler = move(errorHandler);

	asyncRead();
}

void TCPConnection::WritePacket(const vector<unsigned char>& buffer) {
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

	vector<unsigned char> buffer(bytesTransferred);
	buffer_copy(io::buffer(buffer), _streamBuf.data());

	auto packet = TCPConnection::Packet::Create(PacketSource::Client, shared_from_this(), buffer);

	if (!packet->IsValid()) {
		cout << format("[TCPConnection] Client ({}) sent packet with invalid signature!\n", _endpoint);
		_streamBuf.consume(bytesTransferred);
		asyncRead();
	}
	else if (!packet->GetLength()) {
		cout << format("[TCPConnection] Client ({}) sent packet with size 0!\n", _endpoint);
		_streamBuf.consume(bytesTransferred);
		asyncRead();
	}
	else {
		io::async_read(_socket, _streamBuf, io::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
		(boost::system::error_code ec, size_t bytesTransferred) {
			if (ec) {
				self->_socket.close(ec);

				self->_errorHandler();
				return;
			}

			vector<unsigned char> buffer(4 + bytesTransferred);
			buffer_copy(io::buffer(buffer), self->_streamBuf.data());
			self->_streamBuf.consume(4 + bytesTransferred);

			packet->SetBuffer(buffer);

			cout << format("[TCPConnection] Received packet from {}:", self->GetEndPoint());

			for (auto c : buffer) {
				cout << format(" {}", c & 0xFF);
			}

			cout << endl;

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

	cout << format("[TCPConnection] Sent packet to {}:", GetEndPoint());

	for (auto c : _outgoingPackets.front()) {
		cout << format(" {}", c & 0xFF);
	}

	cout << endl;

	_outgoingPackets.pop();

	if (!_outgoingPackets.empty()) {
		asyncWrite();
	}
}