
#include "../include/RedisDatabase.h"

#include <algorithm>
#include <fstream>
#include <ios>
#include <sstream>


RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::flushAll() {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

void RedisDatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
};
bool RedisDatabase::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if (it != kv_store.end()) {
        value = it->second;
        return true;
    }
    return false;
};
std::vector<std::string>RedisDatabase:: keys() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> result;
    for (const auto& kv : kv_store) {
        result.push_back(kv.first);
    }
    for (const auto& kv : list_store) {
        result.push_back(kv.first);
    }
    for (const auto& kv : hash_store) {
        result.push_back(kv.first);
    }
};
std::string RedisDatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.find(key) != kv_store.end()) {
        return "string";
    }
    if (list_store.find(key) != list_store.end()) {
        return "list";
    }
    if (hash_store.find(key) != hash_store.end()) {
        return "list";
    }
    return "none";
};
bool RedisDatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    erased |= kv_store.erase(key) > 0;
    erased |= list_store.erase(key) > 0;
    erased |= hash_store.erase(key) > 0;
    return erased;
};
// expire
bool RedisDatabase::expire(const std::string& key, std::string& seconds) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exists = (kv_store.find(key) != kv_store.end()) ||
        (list_store.find(key) != list_store.end()) ||
            (hash_store.find(key) != hash_store.end());
    if (!exists) return false;
    expire_store[key] = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoi(seconds));
    return true;
};
// rename
bool RedisDatabase::rename(const std::string& oldkey, const std::string& newkey) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool found = false;
    auto itKv = kv_store.find(oldkey);
    if (itKv != kv_store.end()) {
        kv_store[newkey] = itKv->second;
        found = true;
        kv_store.erase(itKv);
    }
    auto itList = list_store.find(oldkey);
    if (itList != list_store.end()) {
        list_store[newkey] = itList->second;
        found = true;
        list_store.erase(itList);
    }
    auto itHash = hash_store.find(oldkey);
    if (itHash != hash_store.end()) {
        hash_store[newkey] = itHash->second;
        found = true;
        hash_store.erase(itHash);
    }
    auto itExpire = expire_store.find(oldkey);
    if (itExpire != expire_store.end()) {
        expire_store[newkey] = itExpire->second;
        found = true;
        expire_store.erase(itExpire);
    }
    return found;
};

// list
ssize_t RedisDatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second.size();
    }
    return 0;
};

void RedisDatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].insert(list_store[key].begin(), value);
};

void RedisDatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].push_back(value);
};

bool RedisDatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    }
    return false;
};
bool RedisDatabase::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.pop_back();
        return true;
    }
    return false;
};
int RedisDatabase::lrem(const std::string& key, int count, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    int removed = 0;
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return 0;
    }
    auto& list = it->second;
    if (count == 0) {
        //remove all ocurrances
        auto new_end = std::remove(list.begin(), list.end(), value);
        removed = std::distance(new_end, list.end());
        list.erase(new_end, list.end());
    } else if (count >0) {
        // remover da da head ate o tail (contagem eh negativa)
        for (auto iter = list.begin(); iter != list.end() && removed < count; ) {
            if (*iter == value) {
                iter = list.erase(iter);
                ++removed;
            } else {
                ++iter;
            }
        }
    } else {
        // remover da tail ate o head (contagem eh negativa)
        for (auto riter = list.rbegin(); riter != list.rend() && removed < (-count) ; ) {
            if (*riter == value) {
                auto fwdIterator = riter.base();
                --fwdIterator;
                fwdIterator = list.erase(fwdIterator);
                ++removed;
                riter = std::reverse_iterator<std::vector<std::string>::iterator>(fwdIterator);
            } else {
                ++riter;
            }
        }
    }
}

bool RedisDatabase::lindex(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return false;
    }
    const auto& list = it->second;
    if (index < 0) {
        index = list.size() + index;
    }
    if (index < 0 || static_cast<ssize_t>(index) >= list.size()) {
        return false;
    }
    value = list[index];
    return true;
}

bool RedisDatabase::lset(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return false;
    }
    auto& list = it->second;
    if (index < 0) {
        index = list.size() + index;
    }
    if (index < 0 || static_cast<ssize_t>(index) >= list.size()) {
        return false;
    }
    return true;
}

bool RedisDatabase::hset(const std::string& key, const std::string& field, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    hash_store[key][field] = value;
    return true;
};
bool RedisDatabase::hget(const std::string& key, const std::string& field, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        auto f = it->second.find(field);
        if (f != it->second.end()) {
            value = f->second;
            return true;
        }
    }
    return false;
};
bool RedisDatabase::hdel(const std::string& key, const std::string& field){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        return it->second.erase(field) > 0;
    }
    return false;
};
bool RedisDatabase::hexists(const std::string& key, const std::string& field){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        return it->second.find(field) != it->second.end();
    }
    return false;
};
std::vector<std::string> RedisDatabase::hkeys(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> fields;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair : it->second) {
            fields.push_back(pair.first);
        }
    }
    return fields;
};
std::vector<std::string> RedisDatabase::hvals(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> values;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair : it->second) {
            values.push_back(pair.second);
        }
    }
    return values;
};
ssize_t RedisDatabase::hlen(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        return it->second.size();
    }
    return 0;
};
std::unordered_map<std::string, std::string> RedisDatabase::hgetall(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    if (hash_store.find(key) != hash_store.end()) {
        return hash_store[key];
    }
    return {};
};
bool RedisDatabase::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& values){
    std::lock_guard<std::mutex> lock(db_mutex);
    for (const auto& pair : values) {
        hash_store[key][pair.first] = pair.second;
    }
    return true;
};






/*
 * Memory -> File - dump()
 * File -> Memory - load()
 * k = kv, l = lists, h = hashes
*/
bool RedisDatabase::dump(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;

    for (const auto& kv : kv_store) {
        ofs << "K" << kv.first << " " << kv.second << "\n";
    }
    for (const auto& kv : list_store) {
        ofs << "L " << kv.first;
        for (const auto& item : kv.second) {
            ofs << " " << item;
        }
        ofs << "\n";
    }
    for (const auto& kv : hash_store) {
        ofs << "H " << kv.first;
        for (const auto& field_val : kv.second) {
            ofs << " " << field_val.first << ":" << field_val.second;
        }
        ofs << "\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K') {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        } else if (type == 'L') {
            std::string key;
            iss >> key;
            std::vector<std::string> list;
            std::string item;
            while (iss >> item) {
                list.push_back(item);
            }
            list_store[key] = list;
        } else if (type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;
            while (iss >> pair) {
                auto pos = pair.find(":");
                if (pos != std::string::npos) {
                    std::string field = pair.substr(0, pos);
                    std::string value = pair.substr(pos+1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }
    }
    return true;
}

