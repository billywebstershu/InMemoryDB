#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class Client {
public:
    Client(const std::string& host, int port);
    ~Client();

    bool connectToServer();
    void run();

private:
    std::string host;
    int port;
    SOCKET clientSocket;
    bool authenticated;
    std::string username;

    void sendMessage(const std::string& message);
    std::string receiveMessage();
    void printResponse(const std::string& response);
    void printHelp();
    void printWelcome();
    void setColour(int colour);
    void resetColour();
};