#include "SnapshotManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

SnapshotManager::SnapshotManager(const std::string& filepath,
    DataStore& dataStore,
    int intervalSeconds)
    : filepath(filepath), dataStore(dataStore),
    intervalSeconds(intervalSeconds), running(false),
    lastSnapshotTime("Never") {
}

SnapshotManager::~SnapshotManager() {
    stopAutoSnapshot();
}

std::string SnapshotManager::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

bool SnapshotManager::save() {
    std::ofstream file(filepath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open snapshot file for writing: "
            << filepath << std::endl;
        return false;
    }

    std::string timestamp = getCurrentTimestamp();
    file << "SNAPSHOT " << timestamp << "\n";

    std::vector<std::string> keys = dataStore.keys();
    for (const std::string& key : keys) {
        std::string value = dataStore.get(key);
        if (!value.empty()) {
            file << "KEY " << key << " VALUE " << value << "\n";
        }
    }

    file << "EOF\n";
    file.close();

    lastSnapshotTime = timestamp;
    std::cout << "[SNAPSHOT] Saved at " << timestamp
        << " (" << keys.size() << " keys)" << std::endl;
    return true;
}

bool SnapshotManager::restore() {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "[SNAPSHOT] No snapshot file found at "
            << filepath << " — starting fresh" << std::endl;
        return false;
    }

    std::string line;
    int keysRestored = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line == "EOF") break;

        if (line.substr(0, 8) == "SNAPSHOT") {
            std::cout << "[SNAPSHOT] Restoring from: "
                << line << std::endl;
            continue;
        }

        if (line.substr(0, 4) == "KEY ") {
            std::istringstream stream(line.substr(4));
            std::string key, valueKeyword, value;
            if (stream >> key >> valueKeyword >> value) {
                dataStore.set(key, value);
                keysRestored++;
            }
        }
    }

    file.close();
    lastSnapshotTime = getCurrentTimestamp();
    std::cout << "[SNAPSHOT] Restored " << keysRestored
        << " key(s) from snapshot" << std::endl;
    return true;
}

int SnapshotManager::restoreWithCount() {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return 0;
    }

    std::string line;
    int keysRestored = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line == "EOF") break;

        if (line.substr(0, 8) == "SNAPSHOT") {
            continue;
        }

        if (line.substr(0, 4) == "KEY ") {
            std::istringstream stream(line.substr(4));
            std::string key, valueKeyword, value;
            if (stream >> key >> valueKeyword >> value) {
                dataStore.set(key, value);
                keysRestored++;
            }
        }
    }

    file.close();

    if (keysRestored > 0) {
        lastSnapshotTime = getCurrentTimestamp();
        std::cout << "[SNAPSHOT] Restored " << keysRestored
            << " key(s)" << std::endl;
    }

    return keysRestored;
}

void SnapshotManager::startAutoSnapshot() {
    running = true;
    snapshotThread = std::thread(&SnapshotManager::autoSnapshotLoop, this);
    std::cout << "[SNAPSHOT] Auto-snapshot enabled every "
        << intervalSeconds << " seconds" << std::endl;
}

void SnapshotManager::stopAutoSnapshot() {
    running = false;
    if (snapshotThread.joinable()) {
        snapshotThread.join();
    }
}

void SnapshotManager::autoSnapshotLoop() {
    int elapsed = 0;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        elapsed++;
        if (elapsed >= intervalSeconds) {
            save();
            elapsed = 0;
        }
    }
}

std::string SnapshotManager::getLastSnapshotTime() {
    return lastSnapshotTime;
}