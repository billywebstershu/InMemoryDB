#pragma once
#include <string>
#include <winsock2.h>
#include "DataStore.h"
#include "AuthManager.h"
#include "Command.h"
#include "PerformanceMonitor.h"

class ClientHandler {
public:
    ClientHandler(SOCKET socket, const std::string& address,
        DataStore& dataStore, AuthManager& authManager,
        PerformanceMonitor& perfMonitor);

    void run();

private:
    SOCKET clientSocket;
    std::string clientAddress;
    bool authenticated;
    std::string username;

    DataStore& dataStore;
    AuthManager& authManager;
    PerformanceMonitor& perfMonitor;

    void sendResponse(const std::string& response);
    std::string receiveMessage();
    std::string processCommand(const Command& cmd);
};