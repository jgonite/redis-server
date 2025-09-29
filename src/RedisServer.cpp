
#include "../include/RedisServer.h"

#include <csignal>
#include <cstring>

#include "../include/RedisCommandHandler.h"

#include <iostream>
#include <sys/socket.h>
#include <ostream>
#include <thread>
#include <vector>
#include <netinet/in.h>

#include "../include/RedisDatabase.h"

static int REDIS_CONN_BACKLOG = 10;
static RedisServer* globalServer = nullptr;

void signalHandler(int signum) {
    if (globalServer) {
        std::cout << "Caught signal " << signum << ", shutting down server" << std::endl;
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler() {
    signal(SIGINT, signalHandler);
}

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true) {
    globalServer = this;
    setupSignalHandler();
};

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "Server shutdown completed" << std::endl;
}

void RedisServer::run() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror( "Error creating server socket");
        return;
    }
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)& serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error binding server socket");
        return;
    }

    if (listen(server_socket, REDIS_CONN_BACKLOG) < 0) { // backlog de conexoes = 10
        perror("Error listening on server socket");
    }

    std::cout << "Server started on port: " << port << std::endl;

    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    while (running) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Error accepting client connection" << std::endl;
            }
            break;
        }
        threads.emplace_back([client_socket, &cmdHandler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);
                if (bytes <= 0) break;
                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            close(client_socket);

        });
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // shutdown
    if (RedisDatabase::getInstance().dump("dump.my_rdb")) {
        std::cout << "Database dumped to dump.my_rdb" << std::endl;
    } else {
        std::cerr << "Error dumping database" << std::endl;
    }

}