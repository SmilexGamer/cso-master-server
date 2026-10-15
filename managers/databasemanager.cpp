#include "databasemanager.h"
#include "serverconfig.h"
#include "serverconsole.h"
#include "mysql/mysqld_error.h"
#include "libbcrypt/bcrypt.h"
#include <tuple>
#include <ctime>
#include "udp_server.h"

DatabaseManager databaseManager;

DatabaseManager::DatabaseManager() {
    _connection = NULL;
}

DatabaseManager::~DatabaseManager() {
    Shutdown();
}

bool DatabaseManager::Init(const string& server, const string& user, const string& password, const string& database) {
    _connection = mysql_init(NULL);
    bool success = true;

    if (!mysql_real_connect(_connection, server.c_str(), user.c_str(), password.c_str(), database.c_str(), 0, NULL, 0)) {
        success = false;
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Connection error on Init: {}\n", mysql_error(_connection)));
    }

    tie(success, _connection) = make_tuple(success, _connection);

    return success;
}

void DatabaseManager::Shutdown() {
    if (_connection) {
        RemoveAllUserSessions();
        RemoveAllUserTransfers();
        RemoveAllRooms();

        mysql_close(_connection);
        _connection = NULL;
    }
}

const LoginResult DatabaseManager::Login(const string& userName, const string& password) {
    if (userName.empty()) {
        return { 0, Packet_ReplyType::InvalidName };
    }

    if (password.empty()) {
        return { 0, Packet_ReplyType::InvalidPassword };
    }

    char* userNameEscaped = new char[userName.size() * 2 + 1];
    mysql_real_escape_string(_connection, userNameEscaped, userName.c_str(), (unsigned long)userName.size());

    string query = format("SELECT userID, password FROM users WHERE userName = '{}';", userNameEscaped);
    delete[] userNameEscaped;

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on Login: {}\n", mysql_error(_connection)));
        return { 0, Packet_ReplyType::SysError };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    LoginResult loginResult;

    if (row == NULL) {
        loginResult.reply = Packet_ReplyType::NotExist;
    }
    else {
        int ret = bcrypt_checkpw(password.c_str(), row[1]);
        switch (ret) {
            case -1: {
                loginResult.reply = Packet_ReplyType::SysError;
                break;
            }
            case 0: {
                loginResult.userID = atoi(row[0]);
                break;
            }
            default: {
                loginResult.reply = Packet_ReplyType::WrongPassword;
                break;
            }
        }
    }

    mysql_free_result(res);

    return loginResult;
}

char DatabaseManager::CreateUserCharacter(unsigned long userID, const string& nickName) {
    string query = format("SELECT 1 FROM user_characters WHERE nickName = '{}';", nickName);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserCharacter: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res)) {
        mysql_free_result(res);
        return 0;
    }
    else {
        mysql_free_result(res);

        query = format("INSERT INTO user_characters (userID, nickName) VALUES ({}, '{}');", userID, nickName);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserCharacter: {}\n", mysql_error(_connection)));
            return -1;
        }
    }

    return 1;
}

const UserCharacterResult DatabaseManager::GetUserCharacter(unsigned long userID, unsigned short flag) {
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
        if (flag & USERCHARACTER_FLAG_CLAN) {
            info += " clanID, clanMarkID, clanName, clanUnk4, clanUnk5, clanUnk6,";
        }
        info[info.size() - 1] = ' ';
    }

    string query = format("SELECT{}FROM user_characters WHERE userID = {};", info, userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserCharacter: {}\n", mysql_error(_connection)));
        return { userCharacter, -1 };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);
        return { userCharacter, 0 };
    }

    if (flag) {
        char index = 0;
        if (flag & USERCHARACTER_FLAG_UNK1) {
            userCharacter.unk1 = (unsigned char)atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_NICKNAME) {
            userCharacter.nickName = row[index++];
        }
        if (flag & USERCHARACTER_FLAG_UNK4) {
            userCharacter.unk4_1 = row[index++];
            userCharacter.unk4_2 = (unsigned char)atoi(row[index++]);
            userCharacter.unk4_3 = (unsigned char)atoi(row[index++]);
            userCharacter.unk4_4 = (unsigned char)atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_LEVEL) {
            userCharacter.level = (unsigned char)atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_UNK10) {
            userCharacter.unk10 = (unsigned char)atoi(row[index++]);
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
            userCharacter.city = (unsigned short)atoi(row[index++]);
            userCharacter.county = (unsigned short)atoi(row[index++]);
            userCharacter.neighborhood = (unsigned short)atoi(row[index++]);
            userCharacter.unk200_5 = row[index++];
        }
        if (flag & USERCHARACTER_FLAG_UNK400) {
            userCharacter.unk400 = atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_UNK800) {
            userCharacter.unk800 = (unsigned char)atoi(row[index++]);
        }
        if (flag & USERCHARACTER_FLAG_CLAN) {
            userCharacter.clanID = atoi(row[index++]);
            userCharacter.clanMarkID = atoi(row[index++]);
            userCharacter.clanName = row[index++];
            userCharacter.clanUnk4 = (unsigned char)atoi(row[index++]);
            userCharacter.clanUnk5 = (unsigned char)atoi(row[index++]);
            userCharacter.clanUnk6 = (unsigned char)atoi(row[index++]);
        }
    }

    mysql_free_result(res);

    return { userCharacter, 1 };
}

bool DatabaseManager::CreateUserBuymenus(unsigned long userID) {
    string query = format("SELECT 1 FROM user_buymenus WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserBuymenus: {}\n", mysql_error(_connection)));
        return false;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res)) {
        mysql_free_result(res);
    }
    else {
        mysql_free_result(res);

        query = format("INSERT INTO user_buymenus (userID, categoryID, slot1, slot2, slot3, slot4, slot5, slot6, slot7, slot8, slot9) VALUES");
        for (auto& buyMenu : serverConfig.defaultBuyMenus) {
            query += format(" ({}, {}", userID, buyMenu.categoryID);
            for (unsigned char slotID = 0; slotID < BUYMENU_MAX_SLOT; slotID++) {
                query += format(", {}", buyMenu.items[slotID]);
            }
            query += "),";
        }
        query[query.size() - 1] = ';';

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserBuymenus: {}\n", mysql_error(_connection)));
            return false;
        }
    }

    return true;
}

bool DatabaseManager::CreateUserBookmarks(unsigned long userID) {
    string query = format("SELECT 1 FROM user_bookmarks WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserBookmarks: {}\n", mysql_error(_connection)));
        return false;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res)) {
        mysql_free_result(res);
    }
    else {
        mysql_free_result(res);

        string query = format("INSERT INTO user_bookmarks (userID, slotID, name, primaryItemID, primaryAmmo, secondaryItemID, secondaryAmmo, flashbang, hegrenade, smokegrenade, defusekit, nightvision, kevlar, unk1) VALUES");
        for (unsigned char slotID = 0; slotID < BOOKMARK_MAX_SLOT; slotID++) {
            query += format(" ({}, {}, '', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),", userID, slotID);
        }
        query[query.size() - 1] = ';';

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserBookmarks: {}\n", mysql_error(_connection)));
            return false;
        }
    }

    return true;
}

bool DatabaseManager::CreateUserInventory(unsigned long userID) {
    string query = format("SELECT 1 FROM user_inventory WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserInventory: {}\n", mysql_error(_connection)));
        return false;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res)) {
        mysql_free_result(res);
    }
    else {
        mysql_free_result(res);

        time_t now;
        time(&now);
        unsigned char slotID = 1;

        string query = format("INSERT INTO user_inventory (userID, slotID, itemID, count, inUse, obtainTime, expirationTime) VALUES");
        for (auto& itemID : serverConfig.defaultInventory) {
            query += format(" ({}, {}, {}, 1, 1, {}, 0),", userID, slotID++, itemID, now);
        }
        query[query.size() - 1] = ';';

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on CreateUserInventory: {}\n", mysql_error(_connection)));
            return false;
        }
    }

    return true;
}

char DatabaseManager::AddUserSession(unsigned long userID) {
    string query = format("SELECT serverID, channelID FROM user_sessions WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);

        query = format("INSERT INTO user_sessions (userID, serverID, channelID) VALUES ({}, {}, {});", userID, serverConfig.serverID, serverConfig.channelID);

        if (mysql_query(_connection, query.c_str())) {
            if (mysql_errno(_connection) == ER_DUP_ENTRY) {
                return 0;
            }

            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
            return -1;
        }
    }
    else {
        unsigned char serverID = (unsigned char)atoi(row[0]);
        unsigned char channelID = (unsigned char)atoi(row[1]);

        mysql_free_result(res);

        if (!serverID || serverID > serverConfig.serverList.size()) {
            return 0;
        }

        if (!channelID || channelID > serverConfig.serverList[(unsigned char)(serverID - 1)].channels.size()) {
            return 0;
        }

        if (serverConfig.serverList[(unsigned char)(serverID - 1)].channels[(unsigned char)(channelID - 1)].isOnline) {
            return 0;
        }

        query = format("DELETE FROM user_sessions WHERE userID = {};", userID);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
            return -1;
        }

        query = format("INSERT INTO user_sessions (userID, serverID, channelID) VALUES ({}, {}, {});", userID, serverConfig.serverID, serverConfig.channelID);

        if (mysql_query(_connection, query.c_str())) {
            if (mysql_errno(_connection) == ER_DUP_ENTRY) {
                return 0;
            }

            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddUserSession: {}\n", mysql_error(_connection)));
            return -1;
        }
    }

    return 1;
}

void DatabaseManager::RemoveUserSession(unsigned long userID) {
    string query = format("DELETE FROM user_sessions WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveUserSession: {}\n", mysql_error(_connection)));
        return;
    }
}

char DatabaseManager::IsUserSessionAdded(unsigned long userID) {
    string query = format("SELECT serverID, channelID FROM user_sessions WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on IsUserSessionAdded: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        mysql_free_result(res);

        return 0;
    }
    else {
        unsigned char serverID = (unsigned char)atoi(row[0]);
        unsigned char channelID = (unsigned char)atoi(row[1]);

        mysql_free_result(res);

        if (!serverID || serverID > serverConfig.serverList.size()) {
            return 2;
        }

        if (!channelID || channelID > serverConfig.serverList[(unsigned char)(serverID - 1)].channels.size()) {
            return 2;
        }

        if (serverID != serverConfig.serverID || channelID != serverConfig.channelID) {
            return 2;
        }
    }

    return 1;
}

void DatabaseManager::RemoveAllUserSessions() {
    string query = format("DELETE FROM user_sessions WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveAllUserSessions: {}\n", mysql_error(_connection)));
        return;
    }
}

const TransferLoginResult DatabaseManager::TransferLogin(const string& authToken) {
    if (authToken.empty()) {
        return { 0, "", Packet_ReplyType::INVALID_USERINFO };
    }

    string query = format("SELECT userID FROM user_transfers WHERE authToken = '{}' AND serverID = {} AND channelID = {};", authToken, serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on TransferLogin: {}\n", mysql_error(_connection)));
        return { 0, "", Packet_ReplyType::SysError };
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    TransferLoginResult transferLoginResult;

    if (row == NULL) {
        transferLoginResult.reply = Packet_ReplyType::TRANSFER_ERR;
    }
    else {
        transferLoginResult.userID = atoi(row[0]);

        mysql_free_result(res);

        query = format("SELECT userName FROM users WHERE userID = {};", transferLoginResult.userID);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on TransferLogin: {}\n", mysql_error(_connection)));
            return { 0, "", Packet_ReplyType::SysError };
        }

        res = mysql_use_result(_connection);
        row = mysql_fetch_row(res);

        if (row == NULL) {
            transferLoginResult.reply = Packet_ReplyType::NotExist;
        }
        else {
            transferLoginResult.userName = row[0];
        }
    }

    mysql_free_result(res);

    return transferLoginResult;
}

char DatabaseManager::AddUserTransfer(unsigned long userID, const string& authToken, unsigned char serverID, unsigned char channelID) {
    string query = format("INSERT INTO user_transfers (userID, authToken, serverID, channelID, transferTime) VALUES ({}, '{}', {}, {}, {});", userID, authToken, serverID, channelID, serverConsole.GetCurrentTime());

    if (mysql_query(_connection, query.c_str())) {
        if (mysql_errno(_connection) == ER_DUP_ENTRY) {
            return 0;
        }

        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddUserTransfer: {}\n", mysql_error(_connection)));
        return -1;
    }

    return 1;
}

void DatabaseManager::RemoveUserTransfer(unsigned long userID) {
    string query = format("DELETE FROM user_transfers WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveUserTransfer: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveOldUserTransfers() {
    string query = format("DELETE FROM user_transfers WHERE serverID = {} AND channelID = {} AND transferTime < {};", serverConfig.serverID, serverConfig.channelID, serverConsole.GetCurrentTime() - 60);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveOldUserTransfers: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveAllUserTransfers() {
    string query = format("DELETE FROM user_transfers WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveAllUserTransfers: {}\n", mysql_error(_connection)));
        return;
    }
}

bool DatabaseManager::SaveUserConfig(unsigned long userID, const vector<unsigned char>& userConfig) {
    string userConfigHex;
    for (auto& c : userConfig) {
        userConfigHex += format("{:02X}", c);
    }

    string query = format("INSERT INTO user_configs (userID, userConfig) VALUES ({}, 0x{}) ON DUPLICATE KEY UPDATE userConfig = 0x{};", userID, userConfigHex, userConfigHex);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on SaveUserConfig: {}\n", mysql_error(_connection)));
        return false;
    }

    return true;
}

const vector<unsigned char> DatabaseManager::GetUserConfig(unsigned long userID) {
    string query = format("SELECT userConfig FROM user_configs WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserConfig: {}\n", mysql_error(_connection)));
        return {};
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = mysql_fetch_row(res);

    vector<unsigned char> userConfig;

    if (row) {
        unsigned long* lengths = mysql_fetch_lengths(res);

        if (lengths == NULL) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Fetch lengths error on GetUserConfig: {}\n", mysql_error(_connection)));
        }
        else {
            userConfig.insert(userConfig.begin(), row[0], row[0] + lengths[0]);
        }
    }

    mysql_free_result(res);

    return userConfig;
}

bool DatabaseManager::SaveUserQuickStart(unsigned long userID, unsigned long userQuickStart) {
    unsigned char gameModeID = (unsigned char)(userQuickStart / 10000);
    unsigned char mapID = (unsigned char)((userQuickStart - (gameModeID * 10000)) / 100);
    unsigned char levelLimit = (unsigned char)((userQuickStart - (gameModeID * 10000 + mapID * 100)) / 10);
    unsigned char clanBattle = (unsigned char)(userQuickStart - (gameModeID * 10000 + mapID * 100 + levelLimit * 10));

    string query = format("INSERT INTO user_quickstarts (userID, gameModeID, mapID, levelLimit, clanBattle) VALUES ({}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE gameModeID = {}, mapID = {}, levelLimit = {}, clanBattle = {};", userID, gameModeID, mapID, levelLimit, clanBattle, gameModeID, mapID, levelLimit, clanBattle);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on SaveUserQuickStart: {}\n", mysql_error(_connection)));
        return false;
    }

    return true;
}

unsigned long DatabaseManager::GetUserQuickStart(unsigned long userID) {
    string query = format("SELECT gameModeID, mapID, levelLimit, clanBattle FROM user_quickstarts WHERE userID = {};", userID);

    unsigned long userQuickStart = DEFAULT_USER_QUICKSTART;

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserQuickStart: {}\n", mysql_error(_connection)));
    }
    else {
        MYSQL_RES* res = mysql_use_result(_connection);
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row) {
            userQuickStart = atoi(row[0]) * 10000 + atoi(row[1]) * 100 + atoi(row[2]) * 10 + atoi(row[3]);
        }

        mysql_free_result(res);
    }

    return userQuickStart;
}

bool DatabaseManager::SaveUserBuyMenu(unsigned long userID, unsigned char categoryID, unsigned char slotID, unsigned char itemID) {
    string query = format("UPDATE user_buymenus SET slot{} = {} WHERE userID = {} AND categoryID = {};", slotID + 1, itemID, userID, categoryID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on SaveUserBuyMenu: {}\n", mysql_error(_connection)));
        return false;
    }

    return true;
}

const vector<BuyMenu> DatabaseManager::GetUserBuyMenus(unsigned long userID) {
    string query = format("SELECT categoryID, slot1, slot2, slot3, slot4, slot5, slot6, slot7, slot8, slot9 FROM user_buymenus WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserBuyMenus: {}\n", mysql_error(_connection)));
        return {};
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = NULL;
    vector<BuyMenu> userBuyMenus;

    while ((row = mysql_fetch_row(res)))
    {
        BuyMenu buyMenu;
        buyMenu.categoryID = (unsigned char)atoi(row[0]);

        for (unsigned char slotID = 0; slotID < BUYMENU_MAX_SLOT; slotID++) {
            buyMenu.items.push_back((unsigned char)atoi(row[slotID + 1]));
        }

        userBuyMenus.push_back(buyMenu);
    }

    mysql_free_result(res);

	return userBuyMenus;
}

bool DatabaseManager::SaveUserBookMark(unsigned long userID, const BookMark& userBookMark) {
    char* bookMarkName = new char[userBookMark.name.size() * 2 + 1];
    mysql_real_escape_string(_connection, bookMarkName, userBookMark.name.c_str(), (unsigned long)userBookMark.name.size());

    string query = format("UPDATE user_bookmarks SET name = '{}', primaryItemID = {}, primaryAmmo = {}, secondaryItemID = {}, secondaryAmmo = {}, flashbang = {}, hegrenade = {}, smokegrenade = {}, defusekit = {}, nightvision = {}, kevlar = {}, unk1 = {} WHERE userID = {} AND slotID = {};",
        bookMarkName, userBookMark.primaryItemID, userBookMark.primaryAmmo, userBookMark.secondaryItemID, userBookMark.secondaryAmmo, userBookMark.flashbang,
        userBookMark.hegrenade, userBookMark.smokegrenade, userBookMark.defusekit, userBookMark.nightvision, userBookMark.kevlar, userBookMark.unk1, userID, userBookMark.slotID);
    delete[] bookMarkName;

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on SaveUserBookMark: {}\n", mysql_error(_connection)));
        return false;
    }

    return true;
}

const vector<BookMark> DatabaseManager::GetUserBookMarks(unsigned long userID) {
    string query = format("SELECT slotID, name, primaryItemID, primaryAmmo, secondaryItemID, secondaryAmmo, flashbang, hegrenade, smokegrenade, defusekit, nightvision, kevlar, unk1 FROM user_bookmarks WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserBookMarks: {}\n", mysql_error(_connection)));
        return {};
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = NULL;
    vector<BookMark> userBookMarks;

    while ((row = mysql_fetch_row(res)))
    {
        BookMark bookMark { (unsigned char)atoi(row[0]), row[1], (unsigned char)atoi(row[2]), (bool)atoi(row[3]), (unsigned char)atoi(row[4]), (bool)atoi(row[5]), (unsigned char)atoi(row[6]), 
                            (bool)atoi(row[7]), (bool)atoi(row[8]), (bool)atoi(row[9]), (bool)atoi(row[10]), (unsigned char)atoi(row[11]), (unsigned char)atoi(row[12]) };

        userBookMarks.push_back(bookMark);
    }

    mysql_free_result(res);

    return userBookMarks;
}

const vector<InventoryItem> DatabaseManager::GetUserInventory(unsigned long userID) {
    string query = format("SELECT slotID, itemID, count, inUse, obtainTime, expirationTime FROM user_inventory WHERE userID = {};", userID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on GetUserInventory: {}\n", mysql_error(_connection)));
        return {};
    }

    MYSQL_RES* res = mysql_use_result(_connection);
    MYSQL_ROW row = NULL;
    vector<InventoryItem> userInventory;

    while ((row = mysql_fetch_row(res)))
    {
        InventoryItem item{ (unsigned char)atoi(row[0]), (unsigned char)atoi(row[1]), (unsigned char)atoi(row[2]), (bool)atoi(row[3]), (unsigned long)atoi(row[4]), (unsigned long)atoi(row[5]) };

        userInventory.push_back(item);
    }

    mysql_free_result(res);

    return userInventory;
}

char DatabaseManager::AddRoom(unsigned short roomID, bool clanBattle, bool isPassworded, unsigned char levelLimit, unsigned char gameModeID, unsigned char mapID, unsigned char currentPlayers, unsigned char maxPlayers) {
    string query = format("SELECT 1 FROM rooms WHERE serverID = {} AND channelID = {} AND roomID = {};", serverConfig.serverID, serverConfig.channelID, roomID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddRoom: {}\n", mysql_error(_connection)));
        return -1;
    }

    MYSQL_RES* res = mysql_use_result(_connection);

    if (mysql_fetch_row(res)) {
        mysql_free_result(res);
        return 0;
    }
    else {
        mysql_free_result(res);

        query = format("INSERT INTO rooms (serverID, channelID, roomID, clanBattle, isPassworded, levelLimit, gameModeID, mapID, currentPlayers, maxPlayers) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {});", serverConfig.serverID, serverConfig.channelID, roomID, clanBattle, isPassworded, levelLimit, gameModeID, mapID, currentPlayers, maxPlayers);

        if (mysql_query(_connection, query.c_str())) {
            serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on AddRoom: {}\n", mysql_error(_connection)));
            return -1;
        }
    }

    return 1;
}

bool DatabaseManager::UpdateRoom(unsigned short roomID, bool clanBattle, bool isPassworded, unsigned char levelLimit, unsigned char gameModeID, unsigned char mapID, unsigned char currentPlayers, unsigned char maxPlayers) {
    string query = format("UPDATE rooms SET clanBattle = {}, isPassworded = {}, levelLimit = {}, gameModeID = {}, mapID = {}, currentPlayers = {}, maxPlayers = {} WHERE serverID = {} AND channelID = {} AND roomID = {};",
        clanBattle, isPassworded, levelLimit, gameModeID, mapID, currentPlayers, maxPlayers, serverConfig.serverID, serverConfig.channelID, roomID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on UpdateRoom: {}\n", mysql_error(_connection)));
        return false;
    }
    
    return true;
}

SearchRoomResult DatabaseManager::SearchRoom(unsigned char level, unsigned char gameModeID, unsigned char mapID, unsigned char levelLimit, unsigned char clanBattle) {
    string query = format("SELECT serverID, channelID, roomID FROM rooms WHERE NOT (serverID = {} AND channelID = {}) AND isPassworded = 0 AND currentPlayers < maxPlayers AND levelLimit = {} AND clanBattle = {} ", serverConfig.serverID, serverConfig.channelID, levelLimit, clanBattle);
    if (gameModeID != 99) {
        query += format("AND gameModeID = {} ", gameModeID);
    }
    if (mapID != 99) {
        query += format("AND mapID = {} ", mapID);
    }
    query[query.size() - 1] = ';';

    SearchRoomResult searchRoomResult;

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on SearchRoom: {}\n", mysql_error(_connection)));
    }
    else {
        MYSQL_RES* res = mysql_use_result(_connection);
        MYSQL_ROW row = NULL;

        while ((row = mysql_fetch_row(res)))
        {
            if (!serverConfig.serverList[(unsigned char)(atoi(row[0]) - 1)].channels[(unsigned char)(atoi(row[1]) - 1)].isOnline || serverConfig.serverList[(unsigned char)(atoi(row[0]) - 1)].type == ServerType::Newbie && level >= LIMIT_NEWBIE_LEVEL) {
                continue;
            }

            searchRoomResult.serverID = (unsigned char)atoi(row[0]);
            searchRoomResult.channelID = (unsigned char)atoi(row[1]);
            searchRoomResult.roomID = (unsigned short)atoi(row[2]);
        }

        mysql_free_result(res);
    }

    return searchRoomResult;
}

void DatabaseManager::RemoveRoom(unsigned short roomID) {
    string query = format("DELETE FROM rooms WHERE serverID = {} AND channelID = {} AND roomID = {};", serverConfig.serverID, serverConfig.channelID, roomID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveRoom: {}\n", mysql_error(_connection)));
        return;
    }
}

void DatabaseManager::RemoveAllRooms() {
    string query = format("DELETE FROM rooms WHERE serverID = {} AND channelID = {};", serverConfig.serverID, serverConfig.channelID);

    if (mysql_query(_connection, query.c_str())) {
        serverConsole.Print(PrefixType::Error, format("[ DatabaseManager ] Query error on RemoveAllRooms: {}\n", mysql_error(_connection)));
        return;
    }
}