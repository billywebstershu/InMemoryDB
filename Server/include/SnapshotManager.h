#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "DataStore.h"

class SnapshotManager {
public:
    SnapshotManager(const std::string& filepath,
        DataStore& dataStore,
        int intervalSeconds);
    ~SnapshotManager();

    bool save();
    bool restore();
    int  restoreWithCount();
    void startAutoSnapshot();
    void stopAutoSnapshot();
    std::string getLastSnapshotTime();

private:
    std::string filepath;
    DataStore& dataStore;
    int intervalSeconds;
    std::atomic<bool> running;
    std::thread snapshotThread;
    std::string lastSnapshotTime;

    void autoSnapshotLoop();
    std::string getCurrentTimestamp();
};