#include "ClientHandler.h"
#include <iostream>
#include <sstream>

ClientHandler::ClientHandler(SOCKET socket, const std::string& address,
    DataStore& dataStore, AuthManager& authManager,
    PerformanceMonitor& perfMonitor)
    : clientSocket(socket), clientAddress(address),
    authenticated(false), username(""),
    dataStore(dataStore), authManager(authManager),
    perfMonitor(perfMonitor) {
}

void ClientHandler::run() {
    sendResponse("Welcome to InMemoryDB. Please authenticate with "
        "AUTH <username> <password>");

    while (true) {
        std::string message = receiveMessage();

        if (message.empty()) {
            if (!username.empty()) {
                perfMonitor.removeUser(clientAddress);
                perfMonitor.addEvent("Client disconnected: " +
                    username + " (" + clientAddress + ")");
            }
            else {
                perfMonitor.addEvent("Client disconnected: " +
                    clientAddress);
            }
            break;
        }

        Command cmd = Command::parse(message);
        std::string response = processCommand(cmd);
        sendResponse(response);

        if (cmd.type == CommandType::QUIT) {
            if (!username.empty()) {
                perfMonitor.removeUser(clientAddress);
                perfMonitor.addEvent("Client quit: " + username +
                    " (" + clientAddress + ")");
            }
            break;
        }
    }

    closesocket(clientSocket);
}

std::string ClientHandler::processCommand(const Command& cmd) {
    if (cmd.type == CommandType::AUTH) {
        if (authManager.authenticate(cmd.username, cmd.password)) {
            authenticated = true;
            username = cmd.username;
            perfMonitor.addUser(username, clientAddress);
            perfMonitor.addEvent("Authenticated: " + username +
                " from " + clientAddress);
            return "OK";
        }
        perfMonitor.addEvent("Failed auth attempt from " + clientAddress);
        return "ERR invalid credentials";
    }

    if (cmd.type == CommandType::QUIT) {
        return "BYE";
    }

    if (!authenticated) {
        return "ERR not authenticated";
    }

    switch (cmd.type) {
    case CommandType::SET: {
        if (cmd.key.empty() || cmd.value.empty())
            return "ERR usage: SET <key> <value>";
        dataStore.set(cmd.key, cmd.value);
        return "OK";
    }
    case CommandType::GET: {
        if (cmd.key.empty())
            return "ERR usage: GET <key>";
        std::string value = dataStore.get(cmd.key);
        return value.empty() ? "NULL" : "VALUE " + value;
    }
    case CommandType::DEL: {
        if (cmd.key.empty())
            return "ERR usage: DEL <key>";
        return dataStore.del(cmd.key) ? "OK" : "ERR key not found";
    }
    case CommandType::KEYS: {
        auto k = dataStore.keys();
        if (k.empty()) return "EMPTY";
        std::string result = "KEYS";
        for (auto& key : k) result += " " + key;
        return result;
    }
    default:
        return "ERR unknown command";
    }
}

void ClientHandler::sendResponse(const std::string& response) {
    std::string message = response + "\n";
    send(clientSocket, message.c_str(), (int)message.length(), 0);
}

std::string ClientHandler::receiveMessage() {
    char buffer[1024];
    std::string result;

    while (true) {
        int bytesReceived = recv(clientSocket, buffer,
            sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) return "";

        buffer[bytesReceived] = '\0';
        result += buffer;

        if (result.find('\n') != std::string::npos) {
            while (!result.empty() && (result.back() == '\n' ||
                result.back() == '\r' ||
                result.back() == ' ')) {
                result.pop_back();
            }
            return result;
        }
    }
}