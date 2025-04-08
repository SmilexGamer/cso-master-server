#include "databasemanager.h"
#include "serverconfig.h"
#include "servertick.h"
#include "serverconsole.h"
#include "mysql/mysqld_error.h"
#include "libbcrypt/BCrypt.hpp"
#include <tuple>

DatabaseManager databaseManager;

DatabaseManager::DatabaseManager() {
    _connection = NULL;
    _addedServerChannel = false;
}

DatabaseManager::~DatabaseManager() {
    Shutdown();
}

bool DatabaseManager::Init(const string& server, const string& user, const string& password, const string& database) {
    _connection = mysql_init(NULL);
    bool success = true;

    if (!mysql_real_connect(_connection, server.c_str(), user.c_str(), password.c_str(), database.c_str(), 0, NULL, 0)) {
        success = false;
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Connection error on Init: {}\n", mysql_error(_connection)));
    }

    tie(success, _connection) = make_tuple(success, _connection);

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "cso-master-server");
    DWORD dwWaitResult = WaitForSingleObject(hMutex, 0);
    if (!dwWaitResult || dwWaitResult == WAIT_ABANDONED) {
        RemoveAllUserSessions();
        RemoveAllUserTransfers();
        RemoveServerChannel();
    }

    return success;
}

void DatabaseManager::Shutdown() {
    if (_addedServerChannel) {
        _addedServerChannel = false;

        RemoveAllUserSessions();
        RemoveAllUserTransfers();
        RemoveServerChannel();
    }

    if (_connection) {
        mysql_close(_connection);
        _connection = NULL;
    }
}

bool DatabaseManager::AddServerChannel() {
    string query = format("SELECT 1 FROM server_channels WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddServerChannel: {}\n", mysql_error(_connection)));
        return false;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res) == NULL) {
        mysql_free_result(res);

        query = format("INSERT INTO server_channels (serverID, channelID, numPlayers) VALUES ({}, {}, 0);", serverConfig.serverID, serverConfig.channelID, 0);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddServerChannel: {}\n", mysql_error(_connection)));
            return false;
        }
    }
    else {
        mysql_free_result(res);

        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Server channel already present in database!\n"));
        return false;
    }

    _addedServerChannel = true;
    return true;
}

void DatabaseManager::RemoveServerChannel() {
    string query = format("DELETE FROM server_channels WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveServerChannel: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::UpdateChannelNumPlayers(unsigned short numPlayers) {
    string query = format("UPDATE server_channels SET numPlayers = {} WHERE serverID = {} AND channelID = {};", numPlayers, serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on UpdateChannelNumPlayers: {}\n", mysql_error(_connection)));
        return;
    }
}

char DatabaseManager::GetChannelNumPlayers(unsigned char serverID, unsigned char channelID) {
    string query = format("SELECT numPlayers FROM server_channels WHERE serverID = {} AND channelID = {};", serverID, channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetChannelNumPlayers: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);

        return 0;
    }

    serverConfig.serverList[serverID - 1].channels[channelID - 1].numPlayers = atoi(row[0]);

    mysql_free_result(res);

    return 1;
}

void DatabaseManager::GetAllChannelsNumPlayers() {
    string query;
    MYSQL_RES* res = NULL;
    MYSQL_ROW row = NULL;

    for (auto& server : serverConfig.serverList) {
        if (server.id == serverConfig.serverID) {
            query = format("SELECT channelID, numPlayers FROM server_channels WHERE serverID = {};", server.id);

            if (mysql_query(_connection, query.c_str())) {
                serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetAllChannelsNumPlayers: {}\n", mysql_error(_connection)));
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
                serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetAllChannelsNumPlayers: {}\n", mysql_error(_connection)));
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
                    serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetAllChannelsNumPlayers: {}\n", mysql_error(_connection)));
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

LoginResult DatabaseManager::Login(const string& userName, const string& password) {
    if (userName.empty()) {
        return { 0, Packet_ReplyType::InvalidName };
    }

    if (password.empty()) {
        return { 0, Packet_ReplyType::InvalidPassword };
    }

    string query = format("SELECT userID, password FROM users WHERE userName = '{}';", userName);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on Login: {}\n", mysql_error(_connection)));
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

char DatabaseManager::CreateCharacter(unsigned long userID, const string& nickName) {
    string query = format("SELECT 1 FROM user_characters WHERE nickName = '{}';", nickName);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on CreateCharacter: {}\n", mysql_error(_connection)));
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
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on CreateCharacter: {}\n", mysql_error(_connection)));
        return -1;
    }

    return 1;
}

UserCharacterResult DatabaseManager::GetUserCharacter(unsigned long userID, unsigned short flag) {
    UserCharacter userCharacter;
    userCharacter.flag = flag;

    string info;

    if (flag == NULL) {
        info += " 1 ";
    }
    else {
        if (flag & USERCHARACTER_FLAG_UNK1) {
            info += " unk1,";
        }
        if (flag & USERCHARACTER_FLAG_NICKNAME) {
            info += " nickName,";
        }
        if (flag & USERCHARACTER_FLAG_UNK4) {
            info += " unk4_1, unk4_2, unk4_3, unk4_4,";
        }
        if (flag & USERCHARACTER_FLAG_LEVEL) {
            info += " level,";
        }
        if (flag & USERCHARACTER_FLAG_UNK10) {
            info += " unk10,";
        }
        if (flag & USERCHARACTER_FLAG_EXP) {
            info += " exp,";
        }
        if (flag & USERCHARACTER_FLAG_CASH) {
            info += " cash,";
        }
        if (flag & USERCHARACTER_FLAG_POINTS) {
            info += " points,";
        }
        if (flag & USERCHARACTER_FLAG_BATTLESTATS) {
            info += " battles, wins, frags, deaths,";
        }
        if (flag & USERCHARACTER_FLAG_LOCATION) {
            info += " city, county, neighborhood, unk200_5,";
        }
        if (flag & USERCHARACTER_FLAG_UNK400) {
            info += " unk400,";
        }
        if (flag & USERCHARACTER_FLAG_UNK800) {
            info += " unk800,";
        }
        if (flag & USERCHARACTER_FLAG_UNK1000) {
            info += " unk1000_1, unk1000_2, unk1000_3, unk1000_4, unk1000_5, unk1000_6,";
        }
        info[info.size() - 1] = ' ';
    }

    string query = format("SELECT{}FROM user_characters WHERE userID = {};", info, userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetUserCharacter: {}\n", mysql_error(_connection)));
        return { userCharacter, -1 };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);
        return { userCharacter, 0 };
    }

    if (flag != NULL) {
        char index = 0;
        if (flag & USERCHARACTER_FLAG_UNK1) {
            userCharacter.unk1 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_NICKNAME) {
            userCharacter.nickName = row[index++];
        }
        if (flag & USERCHARACTER_FLAG_UNK4) {
            userCharacter.unk4_1 = row[index++];
            userCharacter.unk4_2 = atoi(row[index++]);
            userCharacter.unk4_3 = atoi(row[index++]);
            userCharacter.unk4_4 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_LEVEL) {
            userCharacter.level = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_UNK10) {
            userCharacter.unk10 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_EXP) {
            userCharacter.exp = atoll(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_CASH) {
            userCharacter.cash = atoll(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_POINTS) {
            userCharacter.points = atoll(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_BATTLESTATS) {
            userCharacter.battles = atoi(row[index++]);
            userCharacter.wins = atoi(row[index++]);
            userCharacter.frags = atoi(row[index++]);
            userCharacter.deaths = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_LOCATION) {
            userCharacter.city = atoi(row[index++]);
            userCharacter.county = atoi(row[index++]);
            userCharacter.neighborhood = atoi(row[index++]);
            userCharacter.unk200_5 = row[index++];
        }
        if (flag & USERCHARACTER_FLAG_UNK400) {
            userCharacter.unk400 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_UNK800) {
            userCharacter.unk800 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_UNK1000) {
            userCharacter.unk1000_1 = atoi(row[index++]);
            userCharacter.unk1000_2 = atoi(row[index++]);
            userCharacter.unk1000_3 = row[index++];
            userCharacter.unk1000_4 = atoi(row[index++]);
            userCharacter.unk1000_5 = atoi(row[index++]);
            userCharacter.unk1000_6 = atoi(row[index++]);
        }
    }

    mysql_free_result(res);

    return { userCharacter, 1 };
}

char DatabaseManager::AddUserSession(unsigned long userID) {
    string query = format("SELECT 1 FROM user_sessions WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res) == NULL) {
        mysql_free_result(res);

        query = format("INSERT INTO user_sessions (userID, serverID, channelID) VALUES ({}, {}, {});", userID, serverConfig.serverID, serverConfig.channelID);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
            return -1;
        }
    }
    else {
        mysql_free_result(res);

        return 0;
    }

    return 1;
}

void DatabaseManager::RemoveUserSession(unsigned long userID) {
    string query = format("DELETE FROM user_sessions WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveUserSession: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveAllUserSessions() {
    string query = format("DELETE FROM user_sessions WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveAllUserSessions: {}\n", mysql_error(_connection)));
        return;
    }
}

LoginResult DatabaseManager::TransferLogin(const string& userName, const string& userIP) {
    if (userName.empty()) {
        return { 0, Packet_ReplyType::INVALID_USERINFO };
    }

    if (userIP.empty()) {
        return { 0, Packet_ReplyType::INVALID_USERINFO };
    }

    string query = format("SELECT 1 FROM user_transfers WHERE userName = '{}' AND userIP = '{}' AND serverID = {} AND channelID = {};", userName, userIP, serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on TransferLogin: {}\n", mysql_error(_connection)));
        return { 0, Packet_ReplyType::SysError };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    unsigned long userID = 0;
    Packet_ReplyType reply = Packet_ReplyType::LoginSuccess;

    if (row == NULL) {
        reply = Packet_ReplyType::TRANSFER_ERR;
    }
    else {
        mysql_free_result(res);

        query = format("SELECT userID FROM users WHERE userName = '{}';", userName);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on TransferLogin: {}\n", mysql_error(_connection)));
            return { 0, Packet_ReplyType::SysError };
        }

        res = mysql_use_result(_connection);
        row = mysql_fetch_row(res);

        if (row != NULL) {
            userID = atoi(row[0]);
        }
        else {
            reply = Packet_ReplyType::NotExist;
        }
    }

    mysql_free_result(res);

    return { userID, reply };
}

char DatabaseManager::AddUserTransfer(const string& userName, const string& userIP, unsigned char serverID, unsigned char channelID) {
    string query = format("SELECT 1 FROM user_transfers WHERE userName = '{}';", userName);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddUserTransfer: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res) == NULL) {
        mysql_free_result(res);

        query = format("INSERT INTO user_transfers (userName, userIP, serverID, channelID, transferTime) VALUES ('{}', '{}', {}, {}, {});", userName, userIP, serverID, channelID, serverTick.GetCurrentTime());

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on AddUserTransfer: {}\n", mysql_error(_connection)));
            return -1;
        }
    }
    else {
        mysql_free_result(res);

        return 0;
    }

    return 1;
}

void DatabaseManager::RemoveUserTransfer(const string& userName) {
    string query = format("DELETE FROM user_transfers WHERE userName = '{}';", userName);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveUserTransfer: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveOldUserTransfers() {
    string query = format("DELETE FROM user_transfers WHERE serverID = {} AND channelID = {} AND transferTime < {};", serverConfig.serverID, serverConfig.channelID, serverTick.GetCurrentTime() - 60);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveOldUserTransfers: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveAllUserTransfers() {
    string query = format("DELETE FROM user_transfers WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on RemoveAllUserTransfers: {}\n", mysql_error(_connection)));
        return;
    }
}

bool DatabaseManager::SaveUserOptionData(int userID, std::vector<unsigned char>& data) {
    string checkQuery = format("SELECT COUNT(*) FROM user_options WHERE userID = {};", userID);
    if (mysql_query(_connection, checkQuery.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on checking user option: {}\n", mysql_error(_connection)));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);
    bool exists = (row && atoi(row[0]) > 0);
    mysql_free_result(res);

    string hexData = "";
    char hex_buf[3];
    for (size_t i = 0; i < data.size(); ++i) {
        snprintf(hex_buf, sizeof(hex_buf), "%02X", data[i]);
        hexData += hex_buf;
    }

    string query;
    if (exists) {
        query = format("UPDATE user_options SET optionData = 0x{} WHERE userID = {};", hexData, userID);
    }
    else {
        query = format("INSERT INTO user_options (userID, optionData) VALUES ({}, 0x{});", userID, hexData);
    }

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on SaveUserOptionData: {}\n", mysql_error(_connection)));
        return false;
    }

    return true;
}

std::vector<unsigned char> DatabaseManager::GetUserOptionData(int userID) {
    std::vector<unsigned char> data;
    string query = format("SELECT optionData FROM user_options WHERE userID = {};", userID);
    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Query error on GetUserOptionData: {}\n", mysql_error(_connection)));
        return data;
    }

    MYSQL_RES* res = mysql_store_result(_connection);
    if (res == NULL) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Could not store result on GetUserOptionData: {}\n", mysql_error(_connection)));
        return data;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        serverConsole.Print(PrintType::Info, format("[ DatabaseManager ] No option data found for userID: {}\n", userID));
        mysql_free_result(res);
        return data;
    }

    unsigned long* lengths = mysql_fetch_lengths(res);
    if (lengths == NULL) {
        serverConsole.Print(PrintType::Error, format("[ DatabaseManager ] Could not fetch data lengths on GetUserOptionData: {}\n", mysql_error(_connection)));
        mysql_free_result(res);
        return data;
    }

    if (lengths[0] > 0 && row[0] != NULL) {
        data.assign(row[0], row[0] + lengths[0]);
    }

    mysql_free_result(res);

    return data;
}