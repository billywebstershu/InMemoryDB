#include "Server.h"
#include "ClientHandler.h"
#include <iostream>
#include <thread>

Server::Server(int port)
    : port(port), serverSocket(INVALID_SOCKET), running(false),
    snapshotManager("data/snapshot.dat", dataStore, 60),
    adminUI(perfMonitor, snapshotManager) {
}

Server::~Server() {
    stop();
}

PerformanceMonitor& Server::getPerformanceMonitor() {
    return perfMonitor;
}

bool Server::start() {
    // show splash screen first
    adminUI.showSplash();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    if (!authManager.loadUsers("data/users.txt")) {
        std::cerr << "Failed to load users file" << std::endl;
        WSACleanup();
        return false;
    }

    // restore snapshot and show loader screen
    int keysRestored = snapshotManager.restoreWithCount();
    adminUI.showSnapshotLoader(keysRestored, keysRestored);

    // admin login loop until successful
    bool loggedIn = false;
    while (!loggedIn) {
        loggedIn = adminUI.login();
    }

    // create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    snapshotManager.startAutoSnapshot();
    running = true;

    adminUI.addLog("Server started on port " + std::to_string(port));

    // accept connections on background thread
    std::thread acceptThread(&Server::acceptConnections, this);
    acceptThread.detach();

    // admin UI blocks main thread
    adminUI.start();

    return true;
}

void Server::stop() {
    running = false;
    snapshotManager.stopAutoSnapshot();
    snapshotManager.save();
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

void Server::acceptConnections() {
    while (running) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket,
            (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            if (running) {
                std::cerr << "Accept failed: "
                    << WSAGetLastError() << std::endl;
            }
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::string clientAddress = std::string(clientIP) + ":" +
            std::to_string(ntohs(clientAddr.sin_port));

        adminUI.addLog("Client connected from " + clientAddress);

        clientThreads.push_back(std::thread([this, clientSocket, clientAddress]() {
            ClientHandler handler(clientSocket, clientAddress,
                dataStore, authManager, perfMonitor);
            handler.run();
            }));
        clientThreads.back().detach();
    }
}