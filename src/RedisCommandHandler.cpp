
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <algorithm>
#include <vector>
#include <sstream>

// RESP parser:
// *2\r\n$4\r\n\PING\r\n$4\r\nTEST\r\n
// *2 -> array has 2 elementos
// $4 -> next string has 4 characters
// PING
// TEST

std::vector<std::string> parseRespCommand(const std::string& input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    // if not start with *, fallback to splitting by whitespace
    if (input[0] != '*') {
        std::istringstream iss(input); // anotar isso auqi no STD tambem OK
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos = 0;
    // Expect '*' followed by the number of elementos
    if (input[pos] != '*') return tokens;
    pos++; // skip *

    // crlf = carriage return (quebra de linha)
    size_t crlf = input.find("\r\n", pos); // anotar isso em string OK
    if (crlf == std::string::npos) return tokens; // anotar isso em string: npos -> eh uma constante que significa no position pra ser usada junto de find. OK

    int numElements = std::stoi(input.substr(pos, crlf-pos));
    pos = crlf + 2; // skip \r\n

    for (int i = 0; i < numElements; i++) {
        if (pos >= input.size() || input[pos] != '$') break; // format error;
        pos++; // skip $
        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        int len = std::stoi(input.substr(pos, crlf-pos));
        pos = crlf + 2; // skip \r\n
        if (pos + len > input.size()) break;
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len + 2; // skip \r\n
    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler() {}

std::string RedisCommandHandler::processCommand(const std::string &commandLine) {
    // use RESP parser:
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "ERR: empty command\r\n";
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    // anotar transform de STD e ::toupper de Algorithm
    std::ostringstream response;

    RedisDatabase& db = RedisDatabase::getInstance();

    // check commands
    if (cmd == "PING") {
        response <<  "+PONG\r\n";
    } else if (cmd == "ECHO") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'echo' command\r\n";
        } else {
            response << "+" << tokens[1] << "\r\n";
        }
    } else if (cmd == "FLUSHALL") {
        db.flushAll();
        response << "+OK\r\n";
    } else if (cmd == "SET") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'set' command\r\n";
        } else {
            db.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    //kv operations
    } else if (cmd == "GET") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'get' command\r\n";
        } else {
            std::string value;
            if (db.get(tokens[1], value)) {
                response << "$" << value.size() << "\r\n" << value << "\r\n";
            } else {
                response << "$-1\r\n";  // nothing to get
            }
        }
    } else if (cmd == "KEYS") {
        std::vector<std::string> allKeys = db.keys();
        response << "*" << allKeys.size() << "\r\n";
        for (const auto& key : allKeys) {
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    } else if (cmd == "TYPE") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'type' command\r\n";
        } else {
            response << "+" << db.type(tokens[1]) << "\r\n";
        }
    } else if (cmd == "DEL" || cmd == "UNLINK") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for " << cmd << "  command\r\n";
        } else {
            bool res = db.del(tokens[1]);
            response << ":" << (res ? 1 : 0) << "\r\n";
        }
    } else if (cmd == "EXPIRE") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'expire' command\r\n";
        } else {
            if (bool res = db.expire(tokens[1], tokens[2])) response << "+OK\r\n";
        }
    } else if (cmd == "RENAME") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'rename' command\r\n";
        } else {
            if (bool res = db.rename(tokens[1], tokens[2])) response << "+OK\r\n";
        }
    }
    //list operations
    else if (cmd == "LLEN") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'llen' command\r\n";
        } else {
            ssize_t len = db.llen(tokens[1]);
            response << ":" + std::to_string(len) + "\r\n";
        }
    } else if (cmd == "LPUSH") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'lpush' command\r\n";
        } else {
            db.lpush(tokens[1], tokens[2]);
            ssize_t len = db.llen(tokens[1]);
            response << ":" + std::to_string(len) + "\r\n";
        }
    } else if (cmd == "RPUSH") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'rpush' command\r\n";
        } else {
            db.rpush(tokens[1], tokens[2]);
            ssize_t len = db.llen(tokens[1]);
            response << ":" + std::to_string(len) + "\r\n";
        }
    } else if (cmd == "LPOP") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'lpop' command\r\n";
        } else {
            std::string value;
            if (db.lpop(tokens[1], value)) {
                response << "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
            } else {
                response << "$-1\r\n";  // nothing to get
            }
        }
    } else if (cmd == "RPOP") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'lpop' command\r\n";
        } else {
            std::string value;
            if (db.rpop(tokens[1], value)) {
                response << "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
            } else {
                response << "$-1\r\n";  // nothing to get
            }
        }
    } else if (cmd == "LREM") {
        if (tokens.size() < 4) {
            response << "-ERR wrong number of arguments for 'lrem' command\r\n";
        } else {
            try {
                int count = std::stoi(tokens[2]);
                int removed = db.lrem(tokens[1], count, tokens[3]);
                response << ":" + std::to_string(removed) + "\r\n";
            } catch (const std::exception&) {
                response << "-ERR invalid count\r\n";
            }
        }
    } else if (cmd == "LINDEX") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'lindex' command\r\n";
        } else {
            try {
                int index = std::stoi(tokens[2]);
                std::string value;
                if (db.lindex(tokens[1], index, value)) {
                    response << "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
                } else {
                    response << "$-1\r\n";  // nothing to get
                }
            } catch (const std::exception&) {
                response << "-ERR invalid index\r\n";
            }
        }
    } else if (cmd == "LSET") {
        if (tokens.size() < 4) {
            response << "-ERR wrong number of arguments for 'lset' command\r\n";
        } else {
            try {
                int index = std::stoi(tokens[2]);
                std::string value;
                if (db.lset(tokens[1], index, tokens[3])) {
                    response << "+OK\r\n";
                } else {
                    response << "-ERR index out of rangeSAPR4\r\n";  // nothing to get
                }
            } catch (const std::exception&) {
                response << "-ERR invalid index\r\n";
            }
        }
    }
    //hash operations
    else if (cmd == "HSET") {
        if (tokens.size() < 4) {
            response << "-ERR wrong number of arguments for 'hset' command\r\n";
        } else {
            db.hset(tokens[1], tokens[2], tokens[3]);
            response << ":1\r\n";
        }
    } else if (cmd == "HGET") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'hget' command\r\n";
        } else {
            std::string value;
            if (db.hget(tokens[1], tokens[2], value)) {
                response << "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
            } else {
                response << "$-1\r\n";  // nothing to get
            }
        }
    } else if (cmd == "HEXISTS") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'hexists' command\r\n";
        } else {
            bool exists = db.hexists(tokens[1], tokens[2]);
            response << ":" + std::to_string(exists ? 1 : 0) + "\r\n";
        }
    } else if (cmd == "HDEL") {
        if (tokens.size() < 3) {
            response << "-ERR wrong number of arguments for 'hdel' command\r\n";
        } else {
            bool res = db.hdel(tokens[1], tokens[2]);
            response << ":" + std::to_string(res ? 1 : 0) + "\r\n";
        }
    } else if (cmd == "HGETALL") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'hgetall' command\r\n";
        } else {
            auto hash = db.hgetall(tokens[1]);
            response << "*" << hash.size() << "\r\n";
            for (const auto& pair :hash) {
                response << "$" << pair.first.size() << "\r\n" << pair.first << "\r\n";
                response << "$" << pair.second.size() << "\r\n" << pair.second << "\r\n";
            }
        }
    } else if (cmd == "HKEYS" ) {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'hkeys' command\r\n";
        } else {
            auto keys = db.hkeys(tokens[1]);
            response << "*" << keys.size() << "\r\n";
            for (const auto& key : keys) {
                response << "$" << key.size() << "\r\n" << key << "\r\n";
            }
        }
    }  else if ( cmd == "HVALS") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'hvals' command\r\n";
        } else {
            auto vals = db.hvals(tokens[1]);
            response << "*" << vals.size() << "\r\n";
            for (const auto& val : vals) {
                response << "$" << val.size() << "\r\n" << val << "\r\n";
            }
        }
    } else if (cmd == "HLEN") {
        if (tokens.size() < 2) {
            response << "-ERR wrong number of arguments for 'hlen' command\r\n";
        } else {
            ssize_t len = db.hlen(tokens[1]);
            response << ":" + std::to_string(len) + "\r\n";
        }
    } else if (cmd == "HMSET") {
        if (tokens.size() < 4 || (tokens.size()%2) == 1) {
            response << "-ERR wrong number of arguments for 'hmset' command\r\n";
        } else {
            std::vector<std::pair<std::string, std::string>> fieldValues;
            for (size_t i = 2; i <tokens.size(); i+=2) {
                fieldValues.emplace_back(tokens[i], tokens[i+1]);
            }
            db.hmset(tokens[1], fieldValues);
            response << "+OK\r\n";
        }
    }
    else {
        response << "-ERR unknown command\r\n";
    }
    return response.str();
}
