#include "Command.h"
#include <sstream>
#include <algorithm>

Command Command::parse(const std::string& raw) {
    Command cmd;
    cmd.type = CommandType::UNKNOWN;

    std::istringstream stream(raw);
    std::string token;
    stream >> token;

    std::transform(token.begin(), token.end(), token.begin(), ::toupper);

    if (token == "AUTH") {
        cmd.type = CommandType::AUTH;
        stream >> cmd.username >> cmd.password;
    }
    else if (token == "SET") {
        cmd.type = CommandType::SET;
        stream >> cmd.key >> cmd.value;
    }
    else if (token == "GET") {
        cmd.type = CommandType::GET;
        stream >> cmd.key;
    }
    else if (token == "DEL") {
        cmd.type = CommandType::DEL;
        stream >> cmd.key;
    }
    else if (token == "KEYS") {
        cmd.type = CommandType::KEYS;
    }
    else if (token == "QUIT") {
        cmd.type = CommandType::QUIT;
    }

    return cmd;
}