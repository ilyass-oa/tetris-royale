#pragma once
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// One service is shared by every session. The mutex covers read-modify-save.
class DatabaseManager {
    std::filesystem::path usersFile;
    nlohmann::json usersData;
    std::mutex mutex;
    void saveUnlocked();

  public:
    explicit DatabaseManager(const std::string &usersPath);
    bool addUser(const std::string &pseudo, const std::string &password);
    bool verifyUser(const std::string &pseudo, const std::string &password);
    void removeFriend(const std::string &user, const std::string &friendName);
    std::vector<std::string> getFriendsList(const std::string &pseudo);
    std::vector<std::string> getFriendRequests(const std::string &pseudo);
    void acceptFriendRequest(const std::string &user, const std::string &senderName);
    void sendFriendRequest(const std::string &user, std::string &friendName);
    void saveScore(const std::string &pseudo, int score);
    int getUserScore(const std::string &pseudo);
    std::vector<std::string> getTopScores();
};
