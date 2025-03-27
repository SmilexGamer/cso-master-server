#include "usermanager.h"
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

		_signature = TCP_PACKET_SIGNATURE;
		_sequence = outgoingSequence;
		_length = (unsigned short)_buffer.size();
	}
}

TCPConnection::Packet::~Packet() {
#ifdef _DEBUG
	if (_source == PacketSource::Client && _readOffset < _buffer.size()) {
		int readOffset = _readOffset;

		string unreadData;
		for (auto& c : ReadArray_UInt8((int)_buffer.size() - _readOffset)) {
			unreadData += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
		}

		cout << format("[TCPConnection] Packet from client ({}) has unread data (_readOffset: {}, _buffer.size(): {}):{}\n", _connection->GetIPAddress(), readOffset, _buffer.size(), unreadData);
	}
#endif
}

TCPConnection::TCPConnection(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& context) : _sslStream(move(socket), context) {
	_ipAddress = _sslStream.next_layer().remote_endpoint().address().to_string();
}

TCPConnection::~TCPConnection() {
	if (_decryptCipher.ctx) {
		EVP_CIPHER_CTX_free(_decryptCipher.ctx);
		_decryptCipher.ctx = NULL;
	}

	if (_encryptCipher.ctx) {
		EVP_CIPHER_CTX_free(_encryptCipher.ctx);
		_encryptCipher.ctx = NULL;
	}
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
			self->DisconnectClient(ec);
			return;
		}

		self->asyncRead();
	});
#endif
}

void TCPConnection::WritePacket(const vector<unsigned char>& buffer, bool noSSL) {
	if (buffer.size() > TCP_PACKET_MAX_SIZE) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Packet not sent to client ({}) because buffer size ({}) > TCP_PACKET_MAX_SIZE ({})!\n", _ipAddress, buffer.size(), TCP_PACKET_MAX_SIZE);
#endif
		return;
	}

	bool queueIdle = _outgoingPackets.empty();
	_outgoingPackets.push(buffer);

	if (queueIdle) {
		asyncWrite(noSSL);
	}
}

void TCPConnection::DisconnectClient(bool eraseConnection) {
	_sslStream.next_layer().close();
	_errorHandler(eraseConnection);
}

void TCPConnection::DisconnectClient(boost::system::error_code ec) {
	_sslStream.next_layer().close(ec);
	_errorHandler(true);
}

bool TCPConnection::SetupDecryptCipher(CipherMethod method) {
	_decryptCipher.ctx = EVP_CIPHER_CTX_new();
	if (_decryptCipher.ctx == NULL) {
		cout << format("[TCPConnection] Failed to setup decrypt cipher for client ({}): EVP_CIPHER_CTX_new() for _decryptCipher.ctx failed!\n", _ipAddress);
		return false;
	}

	int rc = RAND_bytes(_decryptCipher.key, KEY_SIZE);
	if (rc != 1) {
		cout << format("[TCPConnection] Failed to setup decrypt cipher for client ({}): RAND_bytes() for _decryptCipher.key failed!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_decryptCipher.ctx);
		_decryptCipher.ctx = NULL;
		return false;
	}

	rc = RAND_bytes(_decryptCipher.iv, BLOCK_SIZE);
	if (rc != 1) {
		cout << format("[TCPConnection] Failed to setup decrypt cipher for client ({}): RAND_bytes() for _decryptCipher.iv failed!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_decryptCipher.ctx);
		_decryptCipher.ctx = NULL;
		return false;
	}

	_decryptCipher.method = method;

	if (EVP_DecryptInit(_decryptCipher.ctx, _decryptCipher.method == CipherMethod::RC4_40 ? EVP_rc4_40() : (_decryptCipher.method == CipherMethod::RC4 ? EVP_rc4() : EVP_enc_null()), _decryptCipher.key, _decryptCipher.iv) != 1) {
		cout << format("[TCPConnection] Failed to setup decrypt cipher for client ({}): EVP_DecryptInit() != 1!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_decryptCipher.ctx);
		_decryptCipher.ctx = NULL;
		return false;
	}

	return true;
}

bool TCPConnection::SetupEncryptCipher(CipherMethod method) {
	_encryptCipher.ctx = EVP_CIPHER_CTX_new();
	if (_encryptCipher.ctx == NULL) {
		cout << format("[TCPConnection] Failed to setup encrypt cipher for client ({}): EVP_CIPHER_CTX_new() for _encryptCipher.ctx failed!\n", _ipAddress);
		return false;
	}

	int rc = RAND_bytes(_encryptCipher.key, KEY_SIZE);
	if (rc != 1) {
		cout << format("[TCPConnection] Failed to setup encrypt cipher for client ({}): RAND_bytes() for _encryptCipher.key failed!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_encryptCipher.ctx);
		_encryptCipher.ctx = NULL;
		return false;
	}

	rc = RAND_bytes(_encryptCipher.iv, BLOCK_SIZE);
	if (rc != 1) {
		cout << format("[TCPConnection] Failed to setup encrypt cipher for client ({}): RAND_bytes() for _encryptCipher.iv failed!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_encryptCipher.ctx);
		_encryptCipher.ctx = NULL;
		return false;
	}

	_encryptCipher.method = method;

	if (EVP_EncryptInit(_encryptCipher.ctx, _encryptCipher.method == CipherMethod::RC4_40 ? EVP_rc4_40() : (_encryptCipher.method == CipherMethod::RC4 ? EVP_rc4() : EVP_enc_null()), _encryptCipher.key, _encryptCipher.iv) != 1) {
		cout << format("[TCPConnection] Failed to setup encrypt cipher for client ({}): EVP_EncryptInit() != 1!\n", _ipAddress);
		EVP_CIPHER_CTX_free(_encryptCipher.ctx);
		_encryptCipher.ctx = NULL;
		return false;
	}

	return true;
}

bool TCPConnection::decrypt(vector<unsigned char>& buffer) {
	int outLen = 0;
	if (EVP_DecryptUpdate(_decryptCipher.ctx, buffer.data(), &outLen, buffer.data(), (int)buffer.size()) != 1) {
		cout << format("[TCPConnection] Failed to decrypt packet from client ({}): EVP_DecryptUpdate() != 1!\n", _ipAddress);
		return false;
	}

	return true;
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
		userManager.RemoveUserByConnection(shared_from_this());
		DisconnectClient(ec);
		return;
	}

	vector<unsigned char> buffer(bytesTransferred);
	buffer_copy(boost::asio::buffer(buffer), _streamBuf.data());
	_streamBuf.consume(bytesTransferred);

	if (_decryptCipher.ctx != NULL) {
		if (_decrypt) {
			if (!decrypt(buffer)) {
				userManager.RemoveUserByConnection(shared_from_this());
				DisconnectClient();
				return;
			}
		}
	}

	auto packet = TCPConnection::Packet::Create(PacketSource::Client, shared_from_this(), buffer);

	if (!packet->IsValid()) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent TCP Packet with invalid signature!\n", _ipAddress);
#endif

		userManager.RemoveUserByConnection(shared_from_this());
		DisconnectClient();
		return;
	}
	if (packet->GetSequence() != _incomingSequence) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent TCP Packet with incorrect sequence! Expected {}, got {}\n", _ipAddress, _incomingSequence, packet->GetSequence());
#endif

		userManager.RemoveUserByConnection(shared_from_this());
		DisconnectClient();
		return;
	}
	if (!packet->GetLength()) {
#ifdef _DEBUG
		cout << format("[TCPConnection] Client ({}) sent TCP Packet with length 0!\n", _ipAddress);
#endif

		userManager.RemoveUserByConnection(shared_from_this());
		DisconnectClient();
		return;
	}

#ifdef NO_SSL
	boost::asio::async_read(_sslStream.next_layer(), _streamBuf, boost::asio::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		if (ec) {
			userManager.RemoveUserByConnection(self);
			self->DisconnectClient(ec);
			return;
		}
#else
	boost::asio::async_read(_sslStream, _streamBuf, boost::asio::transfer_exactly(packet->GetLength()), [packet, self = shared_from_this()]
	(boost::system::error_code ec, size_t bytesTransferred) {
		if (ec) {
			userManager.RemoveUserByConnection(self);
			self->DisconnectClient(ec);
			return;
		}
#endif

		vector<unsigned char> buffer(bytesTransferred);
		buffer_copy(boost::asio::buffer(buffer), self->_streamBuf.data());
		self->_streamBuf.consume(bytesTransferred);

		if (self->_decryptCipher.ctx != NULL) {
			if (self->_decrypt) {
				if (!self->decrypt(buffer)) {
					userManager.RemoveUserByConnection(self);
					self->DisconnectClient();
					return;
				}
			}
			else if (buffer[0] == PacketID::RecvCrypt) {
				self->_decrypt = true;
			}
		}

		vector<unsigned char> header = packet->GetBuffer();
		buffer.insert(buffer.begin(), header.begin(), header.end());
		packet->SetBuffer(buffer);

#ifdef _DEBUG
		string bufferStr;
		for (auto& c : buffer) {
			bufferStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
		}

		cout << format("[TCPConnection] Received packet from client ({}):{}\n", self->GetIPAddress(), bufferStr);
#endif

		self->_packetHandler(packet);
		self->asyncRead();
	});
}

bool TCPConnection::encrypt(vector<unsigned char>& buffer) {
	int outLen = 0;
	if (EVP_EncryptUpdate(_encryptCipher.ctx, buffer.data(), &outLen, buffer.data(), (int)buffer.size()) != 1) {
		cout << format("[TCPConnection] Failed to encrypt packet for client ({}): EVP_EncryptUpdate() != 1!\n", _ipAddress);
		return false;
	}

	return true;
}

void TCPConnection::asyncWrite(bool noSSL) {
	vector<unsigned char> buffer(_outgoingPackets.front());
	if (_encryptCipher.ctx != NULL) {
		if (_encrypt) {
			if (!encrypt(_outgoingPackets.front())) {
				userManager.RemoveUserByConnection(shared_from_this());
				DisconnectClient();
				return;
			}
		}
		else if (buffer[4] == PacketID::Crypt) {
			_encrypt = true;
		}
	}

	if (noSSL) {
		boost::asio::async_write(_sslStream.next_layer(), boost::asio::buffer(_outgoingPackets.front()), [self = shared_from_this(), buffer]
		(boost::system::error_code ec, size_t bytesTransferred) {
			self->onWrite(ec, bytesTransferred, buffer);
		});
	}
	else {
		boost::asio::async_write(_sslStream, boost::asio::buffer(_outgoingPackets.front()), [self = shared_from_this(), buffer]
		(boost::system::error_code ec, size_t bytesTransferred) {
			self->onWrite(ec, bytesTransferred, buffer);
		});
	}
}

void TCPConnection::onWrite(boost::system::error_code ec, size_t bytesTransferred, vector<unsigned char> buffer) {
	if (ec) {
		userManager.RemoveUserByConnection(shared_from_this());
		DisconnectClient(ec);
		return;
	}

#ifdef _DEBUG
	string bufferStr;
	for (auto& c : buffer) {
		bufferStr += format(" {}{:X}", c < 0x10 ? "0x0" : "0x", c);
	}

	cout << format("[TCPConnection] Sent TCP Packet to client ({}):{}\n", GetIPAddress(), bufferStr);
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