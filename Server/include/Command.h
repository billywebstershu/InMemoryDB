#pragma once
#include <string>

enum class CommandType {
    AUTH,
    SET,
    GET,
    DEL,
    KEYS,
    QUIT,
    UNKNOWN
};

struct Command {
    CommandType type;
    std::string key;
    std::string value;
    std::string username;
    std::string password;

    static Command parse(const std::string& raw);
};