#pragma once
#include <string>
#include <unordered_map>

class AuthManager {
public:
    bool loadUsers(const std::string& filepath);
    bool authenticate(const std::string& username, const std::string& password);

private:
    std::unordered_map<std::string, std::string> users;
};