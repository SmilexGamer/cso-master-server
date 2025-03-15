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
    string query = format("SELECT 1 FROM server_channels WHERE channelID = {} AND serverID = {};", serverConfig.channelID, serverConfig.serverID);

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
        string query = format("DELETE FROM server_channels WHERE channelID = {} AND serverID = {};", serverConfig.channelID, serverConfig.serverID);

        if (mysql_query(_connection, query.c_str())) {
            cout << format("[DatabaseManager] Query error on RemoveServerChannel: {}\n", mysql_error(_connection));
            return;
        }

        _addedServerChannel = false;
    }
}

void DatabaseManager::UpdateChannelNumPlayers(unsigned short numPlayers) {
    string query = format("UPDATE server_channels SET numPlayers = {} WHERE channelID = {} and serverID = {};", numPlayers, serverConfig.channelID, serverConfig.serverID);

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
                unsigned char channelID = atoi(row[0]);

                if (channelID == serverConfig.channelID) {
                    continue;
                }

                serverConfig.serverList[server.id - 1].channels[channelID - 1].numPlayers = atoi(row[1]);
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
                    unsigned char channelID = atoi(row[0]);

                    serverConfig.serverList[server.id - 1].channels[channelID - 1].numPlayers = atoi(row[1]);
                }

                mysql_free_result(res);
            }
        }
    }
}

LoginResult DatabaseManager::Login(string userName, string password) {
    if (userName.empty()) {
        return { 0, Packet_ReplyType::InvalidName };
    }

    if (password.empty()) {
        return { 0, Packet_ReplyType::InvalidPassword };
    }

    string query = format("SELECT userID, password FROM users WHERE userName = '{}';", userName);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on Login: {}\n", mysql_error(_connection));
        return { 0, Packet_ReplyType::SysError };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    unsigned long userID = 0;
    Packet_ReplyType reply = Packet_ReplyType::LoginSuccess;

    if (row != NULL) {
        if (!BCrypt::validatePassword(password, row[1])) {
            mysql_free_result(res);

            return { 0, Packet_ReplyType::WrongPassword };
        }

        userID = atoi(row[0]);
    }
    else {
        reply = Packet_ReplyType::NotExist;
    }

    mysql_free_result(res);

    return { userID, reply };
}

int DatabaseManager::CreateCharacter(unsigned long userID, string nickName) {
    string query = format("SELECT 1 FROM user_characters WHERE nickName = '{}';", nickName);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on CreateCharacter: {}\n", mysql_error(_connection));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row != NULL) {
        mysql_free_result(res);
        return 0;
    }

    mysql_free_result(res);

    query = format("INSERT INTO user_characters (userID, nickName) VALUES ({}, '{}');", userID, nickName);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on CreateCharacter: {}\n", mysql_error(_connection));
        return -1;
    }

    return 1;
}

int DatabaseManager::GetUserCharacter(unsigned long userID, UserCharacter& userCharacter) {
    string info;

    if (userCharacter.flag & UserInfoFlag::Unk1) {
        info += " unk1,";
    }
    if (userCharacter.flag & UserInfoFlag::NickName) {
        info += " nickName,";
    }
    if (userCharacter.flag & UserInfoFlag::Unk4) {
        info += " unk4_1, unk4_2, unk4_3, unk4_4,";
    }
    if (userCharacter.flag & UserInfoFlag::Level) {
        info += " level,";
    }
    if (userCharacter.flag & UserInfoFlag::Unk10) {
        info += " unk10,";
    }
    if (userCharacter.flag & UserInfoFlag::Exp) {
        info += " exp,";
    }
    if (userCharacter.flag & UserInfoFlag::Cash) {
        info += " cash,";
    }
    if (userCharacter.flag & UserInfoFlag::Points) {
        info += " points,";
    }
    if (userCharacter.flag & UserInfoFlag::BattleStats) {
        info += " battles, wins, frags, deaths,";
    }
    if (userCharacter.flag & UserInfoFlag::Location) {
        info += " city, county, neighborhood, unk200_5,";
    }
    if (userCharacter.flag & UserInfoFlag::Unk400) {
        info += " unk400,";
    }
    if (userCharacter.flag & UserInfoFlag::Unk800) {
        info += " unk800,";
    }
    if (userCharacter.flag & UserInfoFlag::Unk1000) {
        info += " unk1000_1, unk1000_2, unk1000_3, unk1000_4, unk1000_5, unk1000_6,";
    }
    info[info.size() - 1] = ' ';

    string query = format("SELECT{} FROM user_characters WHERE userID = {};", info, userID);

    if (mysql_query(_connection, query.c_str())) {
        cout << format("[DatabaseManager] Query error on GetUserCharacter: {}\n", mysql_error(_connection));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);
        return 0;
    }

    char index = 0;
    if (userCharacter.flag & UserInfoFlag::Unk1) {
        userCharacter.unk1 = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::NickName) {
        userCharacter.nickName = row[index++];
    }
    if (userCharacter.flag & UserInfoFlag::Unk4) {
        userCharacter.unk4_1 = row[index++];
        userCharacter.unk4_2 = atoi(row[index++]);
        userCharacter.unk4_3 = atoi(row[index++]);
        userCharacter.unk4_4 = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Level) {
        userCharacter.level = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Unk10) {
        userCharacter.unk10 = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Exp) {
        userCharacter.exp = atoll(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Cash) {
        userCharacter.cash = atoll(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Points) {
        userCharacter.points = atoll(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::BattleStats) {
        userCharacter.battles = atoi(row[index++]);
        userCharacter.wins = atoi(row[index++]);
        userCharacter.frags = atoi(row[index++]);
        userCharacter.deaths = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Location) {
        userCharacter.city = atoi(row[index++]);
        userCharacter.county = atoi(row[index++]);
        userCharacter.neighborhood = atoi(row[index++]);
        userCharacter.unk200_5 = row[index++];
    }
    if (userCharacter.flag & UserInfoFlag::Unk400) {
        userCharacter.unk400 = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Unk800) {
        userCharacter.unk800 = atoi(row[index++]);
    }
    if (userCharacter.flag & UserInfoFlag::Unk1000) {
        userCharacter.unk1000_1 = atoi(row[index++]);
        userCharacter.unk1000_2 = atoi(row[index++]);
        userCharacter.unk1000_3 = row[index++];
        userCharacter.unk1000_4 = atoi(row[index++]);
        userCharacter.unk1000_5 = atoi(row[index++]);
        userCharacter.unk1000_6 = atoi(row[index++]);
    }

    mysql_free_result(res);

    return 1;
}