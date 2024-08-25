#include "tcp_connection.h"
#include <iostream>

TCPConnection::Packet::Packet(PacketSource source, TCPConnection::pointer connection, vector<unsigned char> buffer) : _source(source), _connection(connection), _buffer(buffer) {
	_readOffset = 0;
	_writeOffset = (int)_buffer.size();

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

		_signature = PACKET_SIGNATURE;
		_sequence = outgoingSequence;
		_length = (unsigned short)_buffer.size();
	}
}

TCPConnection::Packet::~Packet() {
#ifdef _DEBUG
	if (_source == PacketSource::Client && _readOffset < _buffer.size()) {
		int readOffset = _readOffset;

		string unreadData;
		for (auto c : ReadArray_UInt8((int)_buffer.size() - _readOffset)) {
			unreadData += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
		}

		cout << format("[TCPConnection] Packet from client ({}) has unread data (_readOffset: {}, _buffer.size(): {}):{}\n", _connection->GetEndPoint(), readOffset, _buffer.size(), unreadData.c_str());
	}
#endif
}

TCPConnection::TCPConnection(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& context) : _sslStream(move(socket), context) {
	stringstream endpoint;
	endpoint << _sslStream.next_layer().remote_endpoint();

	_endpoint = endpoint.str();
}

void TCPConnection::Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler) {
	_packetHandler = move(packetHandler);
	_errorHandler = move(errorHandler);

	// Tell the client that it has successfully connected to the server
	static const string serverConnected = SERVERCONNECTED;
	WritePacket(vector<unsigned char>(serverConnected.begin(), serverConnected.end()), true);

#ifdef NO_SSL
	asyncRead();
#else
	_sslStream.async_handshake(boost::asio::ssl::stream_base::server, [self = shared_from_this()]
	(boost::system::error_code ec) {
		if (ec) {
			self->_sslStream.next_layer().close(ec);
			self->_errorHandler();
			return;
		}

		self->asyncRead();
	});
#endif
}

void TCPConnection::WritePacket(const vector<unsigned char>& buffer, bool noSSL) {
	if (buffer.size() > PACKET_MAX_SIZE) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Packet not sent to client ({}) because buffer size ({}) > PACKET_MAX_SIZE ({})!\n", _endpoint, buffer.size(), PACKET_MAX_SIZE);
#endif
		return;
	}

	bool queueIdle = _outgoingPackets.empty();
	_outgoingPackets.push(buffer);

	if (queueIdle) {
		asyncWrite(noSSL);
	}
}

void TCPConnection::DisconnectClient() {
	_sslStream.next_layer().close();

	_errorHandler();
}

void TCPConnection::asyncRead() {
#ifdef NO_SSL
	boost::asio::async_read(_sslStream.next_layer(), _streamBuf, boost::asio::transfer_exactly(PACKET_HEADER_SIZE), [self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
			self->onRead(ec, bytesTransferred);
		});
#else
	boost::asio::async_read(_sslStream, _streamBuf, boost::asio::transfer_exactly(PACKET_HEADER_SIZE), [self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
			self->onRead(ec, bytesTransferred);
		});
#endif
}

void TCPConnection::onRead(boost::system::error_code ec, size_t bytesTransferred) {
	if (ec) {
		_sslStream.next_layer().close(ec);
		_errorHandler();
		return;
	}

	vector<unsigned char> buffer(bytesTransferred);
	buffer_copy(boost::asio::buffer(buffer), _streamBuf.data());

	auto packet = TCPConnection::Packet::Create(PacketSource::Client, shared_from_this(), buffer);

	if (!packet->IsValid()) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent packet with invalid signature!\n", _endpoint);
#endif

		_sslStream.next_layer().close();
		_errorHandler();
		return;
	}
	if (packet->GetSequence() != _incomingSequence) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent packet with incorrect sequence! Expected {}, got {}\n", _endpoint, _incomingSequence, packet->GetSequence());
#endif

		_sslStream.next_layer().close();
		_errorHandler();
		return;
	}
	if (!packet->GetLength()) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent packet with length 0!\n", _endpoint);
#endif

		_sslStream.next_layer().close();
		_errorHandler();
		return;
	}

#ifdef NO_SSL
	boost::asio::async_read(_sslStream.next_layer(), _streamBuf, boost::asio::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		if (ec) {
			self->_sslStream.next_layer().close(ec);
			self->_errorHandler();
			return;
		}
#else
	boost::asio::async_read(_sslStream, _streamBuf, boost::asio::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		if (ec) {
			self->_sslStream.next_layer().close(ec);
			self->_errorHandler();
			return;
		}
#endif

		vector<unsigned char> buffer(PACKET_HEADER_SIZE + bytesTransferred);
		buffer_copy(boost::asio::buffer(buffer), self->_streamBuf.data());
		self->_streamBuf.consume(PACKET_HEADER_SIZE + bytesTransferred);

		packet->SetBuffer(buffer);

#ifdef _DEBUG
		string bufferStr;
		for (auto c : buffer) {
			bufferStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
		}

		cout << format("[TCPConnection] Received packet from client ({}):{}\n", self->GetEndPoint(), bufferStr.c_str());
#endif

		self->_packetHandler(packet);
		self->asyncRead();
	});
}

void TCPConnection::asyncWrite(bool noSSL) {
	if (noSSL) {
		boost::asio::async_write(_sslStream.next_layer(), boost::asio::buffer(_outgoingPackets.front()), [self = shared_from_this()]
		(boost::system::error_code ec, size_t bytesTransferred) {
				self->onWrite(ec, bytesTransferred);
			});
	}
	else {
		boost::asio::async_write(_sslStream, boost::asio::buffer(_outgoingPackets.front()), [self = shared_from_this()]
		(boost::system::error_code ec, size_t bytesTransferred) {
				self->onWrite(ec, bytesTransferred);
			});
	}
}

void TCPConnection::onWrite(boost::system::error_code ec, size_t bytesTransferred) {
	if (ec) {
		_sslStream.next_layer().close(ec);

		_errorHandler();
		return;
	}

#ifdef _DEBUG
	string buffer;
	for (auto c : _outgoingPackets.front()) {
		buffer += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[TCPConnection] Sent packet to client ({}):{}\n", GetEndPoint(), buffer.c_str());
#endif

	_outgoingPackets.pop();

	if (!_outgoingPackets.empty()) {
#ifdef NO_SSL
		asyncWrite(true);
#else
		asyncWrite();
#endif
	}
}