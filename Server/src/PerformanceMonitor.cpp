#include "PerformanceMonitor.h"
#include <sstream>
#include <iomanip>
#include <ctime>

PerformanceMonitor::PerformanceMonitor() {
    startTime = std::chrono::steady_clock::now();

    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

std::string PerformanceMonitor::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void PerformanceMonitor::addUser(const std::string& username,
    const std::string& address) {
    std::lock_guard<std::mutex> lock(usersMutex);
    ConnectedUser user;
    user.username = username;
    user.address = address;
    user.connectedAt = getCurrentTimestamp();
    connectedUsers.push_back(user);
}

void PerformanceMonitor::removeUser(const std::string& address) {
    std::lock_guard<std::mutex> lock(usersMutex);
    connectedUsers.erase(
        std::remove_if(connectedUsers.begin(), connectedUsers.end(),
            [&address](const ConnectedUser& u) {
                return u.address == address;
            }),
        connectedUsers.end()
    );
}

std::vector<ConnectedUser> PerformanceMonitor::getConnectedUsers() {
    std::lock_guard<std::mutex> lock(usersMutex);
    return connectedUsers;
}

int PerformanceMonitor::getConnectedUserCount() {
    std::lock_guard<std::mutex> lock(usersMutex);
    return (int)connectedUsers.size();
}

double PerformanceMonitor::getCPUUsage() {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    double percent = (double)((sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart));
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;
    percent *= 100;

    lastCPU = now;
    lastSysCPU = sys;
    lastUserCPU = user;

    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}

double PerformanceMonitor::getMemoryUsageMB() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(self, &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    return 0.0;
}

std::string PerformanceMonitor::getUptime() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - startTime).count();

    int hours = (int)(elapsed / 3600);
    int minutes = (int)((elapsed % 3600) / 60);
    int seconds = (int)(elapsed % 60);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << seconds;
    return oss.str();
}

void PerformanceMonitor::addEvent(const std::string& event) {
    std::lock_guard<std::mutex> lock(usersMutex);
    events.push_back(event);
    if (events.size() > 100) {
        events.pop_front();
    }
}

std::vector<std::string> PerformanceMonitor::getRecentEvents() {
    std::lock_guard<std::mutex> lock(usersMutex);
    return std::vector<std::string>(events.begin(), events.end());
}