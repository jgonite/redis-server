#ifndef REDISCOMMANDHANDLER_H
#define REDISCOMMANDHANDLER_H
#include <string>

class RedisCommandHandler {

public:
    RedisCommandHandler();
    // process command from the client and return RESP (Redis Protocol)-formatted response
    std::string processCommand(const std::string& commandLine);
};

#endif //REDISCOMMANDHANDLER_H
