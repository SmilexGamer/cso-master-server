#include "databasemanager.h"
#include <tuple>
#include "mysql/mysqld_error.h"
#include <iostream>
#include <format>
#include "serverconfig.h"
#include "libbcrypt/BCrypt.hpp"

DatabaseManager databaseManager;

DatabaseManager::DatabaseManager() {
    _connection = NULL;
    _addedServerChannel = false;
}

DatabaseManager::~DatabaseManager() {
    databaseManager.RemoveServerChannel();

    mysql_close(_connection);
    _connection = NULL;
}

bool DatabaseManager::Init(string server, string user, string password, string database) {
    _connection = mysql_init(NULL);
    bool success = true;

    if (!mysql_real_connect(_connection, server.c_str(), user.c_str(), password.c_str(), database.c_str(), 0, NULL, 0)) {
        success = false;
        cout << format("[DatabaseManager] Connection error on Init: {}\n", mysql_error(_connection));
    }

    tie(success, _connection) = make_tuple(success, _connection);

    return success;
}

bool DatabaseManager::AddServerChannel() {
    string query;
    query = format("SELECT 1 FROM server_channels WHERE channelID = {} AND serverID = {};", serverConfig.channelID, serverConfig.serverID);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on AddServerChannel: {}\n", mysql_error(_connection));
        return false;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res) == NULL) {
        mysql_free_result(res);

        query = format("INSERT INTO server_channels (channelID, serverID, numPlayers) VALUES ({}, {}, 0);", serverConfig.channelID, serverConfig.serverID, 0);

        if (mysql_query(_connection, query.c_str())) {
            cout << format("[DatabaseManager] Query error on AddServerChannel: {}\n", mysql_error(_connection));
            return false;
        }
    }
    else {
        mysql_free_result(res);

        cout << format("[DatabaseManager] Server channel already present in database!\n");
        return false;
    }

    _addedServerChannel = true;
    return true;
}

void DatabaseManager::RemoveServerChannel() {
    if (_addedServerChannel) {
        string query;
        query = format("DELETE FROM server_channels WHERE channelID = {} AND serverID = {};", serverConfig.channelID, serverConfig.serverID);

        if (mysql_query(_connection, query.c_str())) {
            cout << format("[DatabaseManager] Query error on RemoveServerChannel: {}\n", mysql_error(_connection));
            return;
        }

        _addedServerChannel = false;
    }
}

void DatabaseManager::UpdateChannelNumPlayers(unsigned short numPlayers) {
    string query;
    query = format("UPDATE server_channels SET numPlayers = {} WHERE channelID = {} and serverID = {};", numPlayers, serverConfig.channelID, serverConfig.serverID);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on UpdateChannelNumPlayers: {}\n", mysql_error(_connection));
        return;
    }
}

void DatabaseManager::GetChannelsNumPlayers() {
    string query;
    MYSQL_RES* res = NULL;
    MYSQL_ROW row = NULL;

    for (auto& server : serverConfig.serverList) {
        if (server.id == serverConfig.serverID) {
            query = format("SELECT channelID, numPlayers FROM server_channels WHERE serverID = {};", server.id);

            if (mysql_query(_connection, query.c_str())) {
                cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                return;
            }

            res = mysql_use_result(_connection);

            while ((row = mysql_fetch_row(res)) != NULL)
            {
                unsigned char channelID = (unsigned char)atoi(row[0]);

                if (channelID == serverConfig.channelID) {
                    continue;
                }

                serverConfig.serverList[server.id - 1].channels[channelID - 1].numPlayers = (unsigned short)atoi(row[1]);
            }

            mysql_free_result(res);
        }
        else {
            query = format("SELECT 1 FROM server_channels WHERE serverID = {};", server.id);

            if (mysql_query(_connection, query.c_str())) {
                cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                return;
            }

            res = mysql_use_result(_connection);

            if (mysql_fetch_row(res) == NULL) {
                mysql_free_result(res);

                server.status = ServerStatus::NotReady;
            }
            else {
                mysql_free_result(res);

                server.status = ServerStatus::Ready;

                query = format("SELECT channelID, numPlayers FROM server_channels WHERE serverID = {};", server.id);

                if (mysql_query(_connection, query.c_str())) {
                    cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                    return;
                }

                res = mysql_use_result(_connection);

                while ((row = mysql_fetch_row(res)) != NULL)
                {
                    unsigned char channelID = (unsigned char)atoi(row[0]);

                    serverConfig.serverList[server.id - 1].channels[channelID - 1].numPlayers = (unsigned short)atoi(row[1]);
                }

                mysql_free_result(res);
            }
        }
    }
}

Packet_ReplyType DatabaseManager::Login(string userName, string password) {
    if (userName.empty()) {
        return Packet_ReplyType::InvalidName;
    }

    if (password.empty()) {
        return Packet_ReplyType::InvalidPassword;
    }

    string query;
    query = format("SELECT userID, password FROM users WHERE userName = '{}';", userName);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on Login: {}\n", mysql_error(_connection));
        return Packet_ReplyType::SysError;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    Packet_ReplyType result = Packet_ReplyType::LoginSuccess;

    if (row != NULL) {
        cout << format("password: {}, hash: {}\n", password.c_str(), row[1]);

        if (!BCrypt::validatePassword(password, row[1])) {
            mysql_free_result(res);

            return Packet_ReplyType::WrongPassword;
        }

        query = format("SELECT 1 FROM game_users WHERE userID = {};", row[0]);

        mysql_free_result(res);

        if (mysql_query(_connection, query.c_str())) {
            cout << format("[DatabaseManager] Query error on Login: {}\n", mysql_error(_connection));
            return Packet_ReplyType::SysError;
        }

        res = mysql_use_result(_connection);
        row = mysql_fetch_row(res);

        if (row == NULL) {
            result = Packet_ReplyType::CreateCharacter;
        }
    }
    else {
        result = Packet_ReplyType::NotExist;
    }

    mysql_free_result(res);

    return result;
}