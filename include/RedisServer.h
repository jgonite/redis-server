
#ifndef REDISSERVER_H
#define REDISSERVER_H
#include <atomic>
#include <unistd.h>
#include <string>

class RedisServer {

public:
    RedisServer(int port);
    void run();
    void shutdown();

private:
    int port;
    int server_socket;
    std::atomic<bool> running;

    // isso aqui eh pra fazer o que se chama de
    // graceful shutdown
    void setupSignalHandler();

};

#endif