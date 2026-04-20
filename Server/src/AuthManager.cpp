#include "AuthManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool AuthManager::loadUsers(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Could not open users file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream stream(line);
        std::string username, password;

        if (std::getline(stream, username, ':') &&
            std::getline(stream, password)) {
            users[username] = password;
        }
    }

    std::cout << "Loaded " << users.size()
        << " user(s) from " << filepath << std::endl;
    return true;
}

bool AuthManager::authenticate(const std::string& username,
    const std::string& password) {
    auto it = users.find(username);
    if (it != users.end()) {
        return it->second == password;
    }
    return false;
}

