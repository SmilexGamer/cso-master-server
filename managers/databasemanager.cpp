#include "databasemanager.h"
#include <tuple>
#include "mysql/mysqld_error.h"
#include <iostream>
#include <format>
#include "serverconfig.h"

DatabaseManager databaseManager;

DatabaseManager::DatabaseManager() {
    _connection = NULL;
}

DatabaseManager::~DatabaseManager() {
    mysql_close(_connection);
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

    return true;
}

void DatabaseManager::RemoveServerChannel() {
    string query;
    query = format("DELETE FROM server_channels WHERE channelID = {} AND serverID = {};", serverConfig.channelID, serverConfig.serverID);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on RemoveServerChannel: {}\n", mysql_error(_connection));
        return;
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
    for (auto& server : serverConfig.serverList) {
        if (server.id == serverConfig.serverID) {
            string query;
            query = format("SELECT channelID, numPlayers FROM server_channels WHERE serverID = {};", server.id);

            if (mysql_query(_connection, query.c_str())) {
                cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                return;
            }

            MYSQL_RES* res = mysql_use_result(_connection);
            MYSQL_ROW row;

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
            string query;
            query = format("SELECT 1 FROM server_channels WHERE serverID = {};", server.id);

            if (mysql_query(_connection, query.c_str())) {
                cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                return;
            }

            MYSQL_RES* res = mysql_use_result(_connection);

            if (mysql_fetch_row(res) == NULL) {
                mysql_free_result(res);

                server.status = ServerStatus::NotReady;
            }
            else {
                mysql_free_result(res);

                server.status = ServerStatus::Ready;

                string query;
                query = format("SELECT channelID, numPlayers FROM server_channels WHERE serverID = {};", server.id);

                if (mysql_query(_connection, query.c_str())) {
                    cout << format("[DatabaseManager] Query error on GetChannelsNumPlayers: {}\n", mysql_error(_connection));
                    return;
                }

                MYSQL_RES* res = mysql_use_result(_connection);
                MYSQL_ROW row;

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

bool DatabaseManager::Login(string userName, string password) {
    return true;
}