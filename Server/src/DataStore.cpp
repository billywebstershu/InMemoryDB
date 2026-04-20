#include "DataStore.h"

bool DataStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(storeMutex);
    store[key] = value;
    return true;
}

std::string DataStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = store.find(key);
    if (it != store.end()) {
        return it->second;
    }
    return "";
}

bool DataStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = store.find(key);
    if (it != store.end()) {
        store.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> DataStore::keys() {
    std::lock_guard<std::mutex> lock(storeMutex);
    std::vector<std::string> result;
    for (auto& pair : store) {
        result.push_back(pair.first);
    }
    return result;
}