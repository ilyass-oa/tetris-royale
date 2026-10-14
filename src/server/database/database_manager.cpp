#include "database_manager.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <sys/stat.h>

namespace {
constexpr int iterations = 210000;
std::string hex(const unsigned char *data, std::size_t length) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (std::size_t i = 0; i < length; ++i) {
        result += digits[data[i] >> 4];
        result += digits[data[i] & 15];
    }
    return result;
}
std::vector<unsigned char> unhex(const std::string &text) {
    if (text.size() % 2)
        throw std::runtime_error("Invalid password hash encoding");
    std::vector<unsigned char> result;
    auto digit = [](char c) -> unsigned char {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        throw std::runtime_error("Invalid password hash encoding");
    };
    for (std::size_t i = 0; i < text.size(); i += 2)
        result.push_back((digit(text[i]) << 4) | digit(text[i + 1]));
    return result;
}
std::array<unsigned char, 32> derive(const std::string &password,
                                     const std::vector<unsigned char> &salt) {
    std::array<unsigned char, 32> result{};
    if (!PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt.data(),
                           static_cast<int>(salt.size()), iterations, EVP_sha256(), result.size(),
                           result.data()))
        throw std::runtime_error("Password hashing failed");
    return result;
}
bool validName(const std::string &name) {
    return !name.empty() && name.size() <= 32 &&
           std::all_of(name.begin(), name.end(), [](unsigned char c) {
               return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                      c == '_' || c == '-';
           });
}
void eraseValue(nlohmann::json &values, const std::string &value) {
    if (!values.is_array())
        values = nlohmann::json::array();
    values.erase(std::remove(values.begin(), values.end(), value), values.end());
}
bool contains(const nlohmann::json &values, const std::string &value) {
    return values.is_array() && std::find(values.begin(), values.end(), value) != values.end();
}
} // namespace
DatabaseManager::DatabaseManager(const std::string &usersPath) : usersFile(usersPath) {
    if (usersFile.has_parent_path())
        std::filesystem::create_directories(usersFile.parent_path());
    if (std::filesystem::exists(usersFile)) {
        std::ifstream input(usersFile);
        if (!input)
            throw std::runtime_error("Cannot read account database");
        input >> usersData;
        if (!usersData.is_object())
            throw std::runtime_error("Account database must be a JSON object");
        for (auto &[name, user] : usersData.items()) {
            if (!validName(name) || !user.is_object() || !user.contains("password_hash"))
                throw std::runtime_error(
                    "Unsupported account database; keep a backup and use a new data path");
            user["amis"] = user.value("amis", nlohmann::json::array());
            user["requests"] = user.value("requests", nlohmann::json::array());
        }
    } else {
        usersData = nlohmann::json::object();
        saveUnlocked();
    }
    if (::chmod(usersFile.c_str(), 0600) != 0)
        throw std::runtime_error("Cannot restrict account database permissions");
}
void DatabaseManager::saveUnlocked() {
    // Restrict permissions before writing; replace the old file only after a complete write.
    auto temporary = usersFile;
    temporary += ".tmp";
    {
        std::ofstream create(temporary, std::ios::trunc);
        if (!create)
            throw std::runtime_error("Cannot create database temporary file");
    }
    if (::chmod(temporary.c_str(), 0600) != 0)
        throw std::runtime_error("Cannot restrict database temporary file permissions");
    std::ofstream output(temporary, std::ios::trunc);
    output << usersData.dump(2) << '\n';
    output.flush();
    if (!output)
        throw std::runtime_error("Cannot save account database");
    output.close();
    std::filesystem::rename(temporary, usersFile);
}
bool DatabaseManager::addUser(const std::string &pseudo, const std::string &password) {
    if (!validName(pseudo) || password.empty() || password.size() > 128)
        return false;
    std::lock_guard lock(mutex);
    if (usersData.contains(pseudo))
        return false;
    std::vector<unsigned char> salt(16);
    if (!RAND_bytes(salt.data(), salt.size()))
        throw std::runtime_error("Cannot generate account salt");
    auto hash = derive(password, salt);
    usersData[pseudo] = {{"password_hash", hex(hash.data(), hash.size())},
                         {"salt", hex(salt.data(), salt.size())},
                         {"meilleur_score", 0},
                         {"amis", nlohmann::json::array()},
                         {"requests", nlohmann::json::array()}};
    saveUnlocked();
    return true;
}
bool DatabaseManager::verifyUser(const std::string &pseudo, const std::string &password) {
    if (password.empty() || password.size() > 128)
        return false;
    std::lock_guard lock(mutex);
    if (!usersData.contains(pseudo))
        return false;
    auto &user = usersData.at(pseudo);
    auto salt = unhex(user.at("salt").get<std::string>());
    auto stored = unhex(user.at("password_hash").get<std::string>());
    if (salt.size() != 16 || stored.size() != 32)
        throw std::runtime_error("Invalid password hash");
    auto actual = derive(password, salt);
    return CRYPTO_memcmp(stored.data(), actual.data(), actual.size()) == 0;
}
void DatabaseManager::sendFriendRequest(const std::string &user, std::string &friendName) {
    std::lock_guard lock(mutex);
    if (user == friendName || !usersData.contains(user) || !usersData.contains(friendName))
        return;
    auto &receiver = usersData.at(friendName);
    if (!contains(receiver["requests"], user) && !contains(receiver["amis"], user)) {
        receiver["requests"].push_back(user);
        saveUnlocked();
    }
}
void DatabaseManager::acceptFriendRequest(const std::string &user, const std::string &sender) {
    std::lock_guard lock(mutex);
    if (user == sender || !usersData.contains(user) || !usersData.contains(sender))
        return;
    auto &receiver = usersData.at(user);
    if (!contains(receiver["requests"], sender))
        return;
    eraseValue(receiver["requests"], sender);
    eraseValue(usersData.at(sender)["requests"], user);
    if (!contains(receiver["amis"], sender))
        receiver["amis"].push_back(sender);
    if (!contains(usersData.at(sender)["amis"], user))
        usersData.at(sender)["amis"].push_back(user);
    saveUnlocked();
}
void DatabaseManager::removeFriend(const std::string &user, const std::string &friendName) {
    std::lock_guard lock(mutex);
    if (!usersData.contains(user) || !usersData.contains(friendName))
        return;
    eraseValue(usersData.at(user)["requests"], friendName);
    eraseValue(usersData.at(user)["amis"], friendName);
    eraseValue(usersData.at(friendName)["amis"], user);
    saveUnlocked();
}
std::vector<std::string> DatabaseManager::getFriendsList(const std::string &pseudo) {
    std::lock_guard lock(mutex);
    return usersData.contains(pseudo)
               ? usersData.at(pseudo).at("amis").get<std::vector<std::string>>()
               : std::vector<std::string>{};
}
std::vector<std::string> DatabaseManager::getFriendRequests(const std::string &pseudo) {
    std::lock_guard lock(mutex);
    return usersData.contains(pseudo)
               ? usersData.at(pseudo).at("requests").get<std::vector<std::string>>()
               : std::vector<std::string>{};
}
void DatabaseManager::saveScore(const std::string &pseudo, int score) {
    std::lock_guard lock(mutex);
    if (score < 0 || !usersData.contains(pseudo))
        return;
    auto &user = usersData.at(pseudo);
    if (score > user.value("meilleur_score", 0)) {
        user["meilleur_score"] = score;
        saveUnlocked();
    }
}
int DatabaseManager::getUserScore(const std::string &pseudo) {
    std::lock_guard lock(mutex);
    return usersData.contains(pseudo) ? usersData.at(pseudo).value("meilleur_score", 0) : 0;
}
std::vector<std::string> DatabaseManager::getTopScores() {
    std::lock_guard lock(mutex);
    std::vector<std::pair<int, std::string>> scores;
    for (auto &[name, user] : usersData.items())
        scores.emplace_back(user.value("meilleur_score", 0), name);
    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        return a.first != b.first ? a.first > b.first : a.second < b.second;
    });
    std::vector<std::string> result;
    for (std::size_t i = 0; i < std::min<std::size_t>(10, scores.size()); ++i)
        result.push_back(scores[i].second + " : " + std::to_string(scores[i].first));
    return result;
}
