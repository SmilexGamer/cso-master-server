#include "tcp_connection.h"
#include <iostream>

const string& WelcomeMessage{ "~SERVERCONNECTED\n\0" };

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

TCPConnection::TCPConnection(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& context) : _sslStream(move(socket), context) {
	stringstream endpoint;
	endpoint << _sslStream.next_layer().remote_endpoint();

	_endpoint = endpoint.str();
}

void TCPConnection::Start(PacketHandler&& packetHandler, ErrorHandler&& errorHandler) {
	_packetHandler = move(packetHandler);
	_errorHandler = move(errorHandler);

	// Tell the client that it has successfully connected to the server
	WritePacket(vector<unsigned char>(WelcomeMessage.begin(), WelcomeMessage.end()), true);

#ifdef NO_SSL
	asyncRead();
#else
	_sslStream.async_handshake(boost::asio::ssl::stream_base::server, [self = shared_from_this()]
	(boost::system::error_code ec) {
		if (!ec) {
			self->asyncRead();
		}
	});
#endif
}

void TCPConnection::WritePacket(const vector<unsigned char>& buffer, bool noSSL) {
	bool queueIdle = _outgoingPackets.empty();
	_outgoingPackets.push(buffer);

	if (queueIdle) {
		asyncWrite(noSSL);
	}
}

void TCPConnection::asyncRead() {
#ifdef NO_SSL
	boost::asio::async_read(_sslStream.next_layer(), _streamBuf, boost::asio::transfer_exactly(4), [self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
			self->onRead(ec, bytesTransferred);
		});
#else
	boost::asio::async_read(_sslStream, _streamBuf, boost::asio::transfer_exactly(4), [self = shared_from_this()]
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
		cout << format("[TCPConnection] Client ({}) sent packet with invalid signature!\n", _endpoint);
		_streamBuf.consume(bytesTransferred);
		_sslStream.next_layer().close();
		_errorHandler();
	}
	else if (packet->GetSequence() != _incomingSequence) {
		cout << format("[TCPConnection] Client ({}) sent packet with incorrect sequence! Expected {}, got {}\n", _endpoint, _incomingSequence, packet->GetSequence());
		_streamBuf.consume(bytesTransferred);
		_sslStream.next_layer().close();
		_errorHandler();
	}
	else if (!packet->GetLength()) {
		cout << format("[TCPConnection] Client ({}) sent packet with size 0!\n", _endpoint);
		_streamBuf.consume(bytesTransferred);
		_sslStream.next_layer().close();
		_errorHandler();
	}
	else {
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

			vector<unsigned char> buffer(4 + bytesTransferred);
			buffer_copy(boost::asio::buffer(buffer), self->_streamBuf.data());
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
		buffer += format(" {:#x}", c & 0xFF);
	}

	cout << format("[TCPConnection] Sent packet to {}:{}\n", GetEndPoint(), buffer.c_str());
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