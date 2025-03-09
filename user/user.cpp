#include "user.h"

User::User(TCPConnection::pointer connection, unsigned long userID) : _connection(connection), _userID(userID) {

}