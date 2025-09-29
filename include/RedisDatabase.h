#ifndef REDISDATABASE_H
#define REDISDATABASE_H
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

class RedisDatabase {
public:
    static RedisDatabase& getInstance();
    std::mutex db_mutex;
    // common commands
    bool flushAll();

    //kv
    void set(const std::string& key, const std::string& value);
    bool get(const std::string &key, std::string &value);
    std::vector<std::string> keys();
    std::string type(const std::string& key);
    bool del(const std::string& key);
    // expire
    bool expire(const std::string& key, std::string& seconds);
    // rename
    bool rename(const std::string& oldkey, const std::string& newkey);

    // list
    ssize_t llen(const std::string& key);
    void lpush(const std::string& key, const std::string& value);
    void rpush(const std::string& key, const std::string& value);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string &key, std::string &value);
    int lrem(const std::string& key, int count, std::string& value);
    bool lindex(const std::string& key, int index, std::string& value);
    bool lset(const std::string& key, int index, std::string& value);

    // hash operations
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    bool hget(const std::string& key, const std::string& field, std::string& value);
    bool hdel(const std::string& key, const std::string& field);
    bool hexists(const std::string& key, const std::string& field);
    std::vector<std::string> hkeys(const std::string& key);
    std::vector<std::string> hvals(const std::string& key);
    ssize_t hlen(const std::string& key);
    std::unordered_map<std::string, std::string> hgetall(const std::string& key);
    bool hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& values);

    // Persistance: Dump / load database from file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;


    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expire_store;
};

#endif
