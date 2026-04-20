#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

class DataStore {
public:
    bool set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    std::vector<std::string> keys();

private:
    std::unordered_map<std::string, std::string> store;
    std::mutex storeMutex;
};