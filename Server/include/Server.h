#pragma once
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "DataStore.h"
#include "AuthManager.h"
#include "SnapshotManager.h"
#include "PerformanceMonitor.h"
#include "AdminUI.h"

#pragma comment(lib, "ws2_32.lib")

class Server {
public:
    Server(int port);
    ~Server();

    bool start();
    void stop();

    PerformanceMonitor& getPerformanceMonitor();

private:
    int port;
    SOCKET serverSocket;
    bool running;

    DataStore dataStore;
    AuthManager authManager;
    SnapshotManager snapshotManager;
    PerformanceMonitor perfMonitor;
    AdminUI adminUI;

    std::vector<std::thread> clientThreads;

    void acceptConnections();
};