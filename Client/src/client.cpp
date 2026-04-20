#include "Client.h"
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>

#define COLOUR_WHITE  7
#define COLOUR_GREEN  10
#define COLOUR_RED    12
#define COLOUR_CYAN   11
#define COLOUR_YELLOW 14

Client::Client(const std::string& host, int port)
    : host(host), port(port),
    clientSocket(INVALID_SOCKET),
    authenticated(false), username("") {
}

Client::~Client() {
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    WSACleanup();
}

void Client::setColour(int colour) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colour);
}

void Client::resetColour() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), COLOUR_WHITE);
}

bool Client::connectToServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server: "
            << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }

    return true;
}

void Client::printWelcome() {
    setColour(COLOUR_CYAN);
    std::cout << "=========================================" << std::endl;
    std::cout << "     InMemoryDB Client v1.0" << std::endl;
    std::cout << "=========================================" << std::endl;
    resetColour();
    std::cout << "Connected to " << host << ":" << port << std::endl;
    setColour(COLOUR_CYAN);
    std::cout << "=========================================" << std::endl;
    resetColour();
}

void Client::printHelp() {
    setColour(COLOUR_CYAN);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << " AVAILABLE COMMANDS" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    resetColour();
    setColour(COLOUR_YELLOW);
    std::cout << " AUTH <username> <password>" << std::endl;
    resetColour();
    std::cout << "   Authenticate with the server" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " SET <key> <value>" << std::endl;
    resetColour();
    std::cout << "   Store a value" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " GET <key>" << std::endl;
    resetColour();
    std::cout << "   Retrieve a value" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " DEL <key>" << std::endl;
    resetColour();
    std::cout << "   Delete a key" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " KEYS" << std::endl;
    resetColour();
    std::cout << "   List all keys" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " HELP" << std::endl;
    resetColour();
    std::cout << "   Show this help message" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " LOGOUT" << std::endl;
    resetColour();
    std::cout << "   Log out and return to login prompt" << std::endl;
    setColour(COLOUR_YELLOW);
    std::cout << " QUIT" << std::endl;
    resetColour();
    std::cout << "   Disconnect from server" << std::endl;
    setColour(COLOUR_CYAN);
    std::cout << "-----------------------------------------" << std::endl;
    resetColour();
}

void Client::printResponse(const std::string& response) {
    if (response.empty()) return;

    if (response == "OK" ||
        response.substr(0, 5) == "VALUE" ||
        response.substr(0, 4) == "KEYS" ||
        response == "BYE" ||
        response == "EMPTY") {
        setColour(COLOUR_GREEN);
        std::cout << response << std::endl;
        resetColour();
    }
    else if (response.substr(0, 3) == "ERR" ||
        response == "NULL") {
        setColour(COLOUR_RED);
        std::cout << response << std::endl;
        resetColour();
    }
    else {
        std::cout << response << std::endl;
    }
}

void Client::run() {
    printWelcome();

    setColour(COLOUR_YELLOW);
    std::cout << receiveMessage() << std::endl;
    resetColour();

    std::cout << "Type HELP for a list of commands.\n\n";

    std::string input;

    while (true) {
        if (authenticated) {
            setColour(COLOUR_GREEN);
            std::cout << username;
            resetColour();
            std::cout << "@db> ";
        }
        else {
            std::cout << "db> ";
        }

        std::getline(std::cin, input);
        if (input.empty()) continue;

        std::string cmd = input;
        for (char& c : cmd) c = toupper(c);

        if (cmd == "HELP") {
            printHelp();
            continue;
        }
     
        if (cmd == "LOGOUT") {
            if (!authenticated) {
                setColour(COLOUR_RED);
                std::cout << "ERR not logged in\n";
                resetColour();
                continue;
            }

            system("cls"); 

            authenticated = false;
            username.clear();

            printWelcome();
            setColour(COLOUR_YELLOW);
            std::cout << "Logged out. Please authenticate again.\n";
            resetColour();
            std::cout << "Type HELP for a list of commands.\n\n";
            continue;
        }

        if (cmd.rfind("AUTH", 0) == 0) {
            sendMessage(input);
            std::string res = receiveMessage();
            printResponse(res);

            if (res == "OK") {
                std::istringstream iss(input);
                std::string tmp;
                iss >> tmp >> username;
                authenticated = true;
            }
            continue;
        }

        sendMessage(input);
        std::string res = receiveMessage();
        printResponse(res);

        if (cmd == "QUIT") break;
    }

    std::cout << "Disconnected.\n";
}

void Client::sendMessage(const std::string& message) {
    std::string msg = message + "\n";
    send(clientSocket, msg.c_str(), (int)msg.length(), 0);
}

std::string Client::receiveMessage() {
    std::string result;
    char ch;
    while (true) {
        int bytesReceived = recv(clientSocket, &ch, 1, 0);
        if (bytesReceived <= 0) return "";

        if (ch == '\n') break;
        result += ch;
    }
    if (!result.empty() && result.back() == '\r') {
        result.pop_back();
    }
    return result;
}