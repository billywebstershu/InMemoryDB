#pragma once
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

struct ConnectedUser {
    std::string username;
    std::string address;
    std::string connectedAt;
};

class PerformanceMonitor {
public:
    PerformanceMonitor();

    void addUser(const std::string& username, const std::string& address);
    void removeUser(const std::string& address);
    std::vector<ConnectedUser> getConnectedUsers();
    int getConnectedUserCount();

    double getCPUUsage();
    double getMemoryUsageMB();
    std::string getUptime();

    void addEvent(const std::string& event);
    std::vector<std::string> getRecentEvents();

private:
    std::vector<ConnectedUser> connectedUsers;
    std::deque<std::string> events;
    std::mutex usersMutex;
    std::chrono::steady_clock::time_point startTime;

    ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    int numProcessors;
    HANDLE self;

    std::string getCurrentTimestamp();
};