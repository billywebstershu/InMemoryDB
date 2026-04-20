#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include "PerformanceMonitor.h"
#include "SnapshotManager.h"

class AdminUI {
public:
    AdminUI(PerformanceMonitor& perfMonitor, SnapshotManager& snapshotManager);
    ~AdminUI();

    bool login();
    void start();
    void stop();
    void showSplash();
    void showSnapshotLoader(int keysLoaded, int totalKeys);
    void addLog(const std::string& message);

private:
    PerformanceMonitor& perfMonitor;
    SnapshotManager& snapshotManager;
    std::atomic<bool> running;

    std::deque<std::string> logMessages;
    std::mutex logMutex;

    std::vector<double> cpuHistory;
    std::vector<double> memHistory;
    static const int GRAPH_WIDTH = 40;

    void updateGraphHistory();
    std::string getCurrentTime();
    std::string centerText(const std::string& text, int width);
};