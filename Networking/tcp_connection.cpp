#include "tcp_connection.h"
#include <iostream>

TCPConnection::Packet::Packet(PacketSource source, TCPConnection::pointer connection, vector<unsigned char> buffer) : _source(source), _connection(connection), _buffer(buffer) {
	_readOffset = 0;
	_writeOffset = 0;

	if (source == PacketSource::Client) {
		unsigned char incomingSequence = _connection->GetIncomingSequence();
		if (incomingSequence == UINT8_MAX) {
			_connection->SetIncomingSequence(0);
		}
		else {
			_connection->SetIncomingSequence(++incomingSequence);
		}

		_signature = ReadUInt8();
		_sequence = ReadUInt8();
		_length = ReadUInt16_LE();
	}
	else {
		unsigned char outgoingSequence = _connection->GetOutgoingSequence();
		if (outgoingSequence == UINT8_MAX) {
			_connection->SetOutgoingSequence(0);
		}
		else {
			_connection->SetOutgoingSequence(++outgoingSequence);
		}

		_signature = PacketSignature;
		_sequence = outgoingSequence;
		_length = (unsigned short)_buffer.size();
	}
}

TCPConnection::Packet::~Packet() {
#ifdef _DEBUG
	if (_source == PacketSource::Client && _readOffset < _length + 4) {
		int readOffset = _readOffset;

		string unreadData;
		for (auto c : ReadArray_UInt8(_length + 4 - _readOffset)) {
			unreadData += format(" {:#x}", c & 0xFF);
		}

		cout << format("[TCPConnection::Packet] Packet from {} has unread data (_readOffset: {}, _length: {}):{}\n", _connection->GetEndPoint(), readOffset, _length, unreadData.c_str());
	}
#endif
}

TCPConnection::TCPConnection(io::ip::tcp::socket&& socket) : _socket(move(socket)) {
	stringstream endpoint;
	endpoint << _socket.remote_endpoint();

	_endpoint = endpoint.str();
}

TCPConnection::~TCPConnection() {
	_socket.close();
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
		_socket.close();
	}
	else if (packet->GetSequence() != _incomingSequence) {
		cout << format("[TCPConnection] Client ({}) sent packet with incorrect sequence! Expected {}, got {}\n", _endpoint, _incomingSequence, packet->GetSequence());
		_streamBuf.consume(bytesTransferred);
		_socket.close();
	}
	else if (!packet->GetLength()) {
		cout << format("[TCPConnection] Client ({}) sent packet with size 0!\n", _endpoint);
		_streamBuf.consume(bytesTransferred);
		_socket.close();
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

#ifdef _DEBUG
			string bufferStr;
			for (auto c : buffer) {
				bufferStr += format(" {:#x}", c & 0xFF);
			}

			cout << format("[TCPConnection] Received packet from {}:{}\n", self->GetEndPoint(), bufferStr.c_str());
#endif

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

#ifdef _DEBUG
	string buffer;
	for (auto c : _outgoingPackets.front()) {
		buffer += format(" {:#x}", c & 0xFF);
	}

	cout << format("[TCPConnection] Sent packet to {}:{}\n", GetEndPoint(), buffer.c_str());
#endif

	_outgoingPackets.pop();

	if (!_outgoingPackets.empty()) {
		asyncWrite();
	}
}