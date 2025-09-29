#include <iostream>
#include <thread>
#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"

int main(int argc, char* argv[]) {
    int port = 6371;
    if (argc >=2) port = std::stoi(argv[1]);
    if (RedisDatabase::getInstance().load("dump.my_rdb")) {
        std::cout << "Database loaded from dump.my_rdb" << std::endl;
    }
    RedisServer server(port);

    // Background persistance: dump the database every 300 seconds
    std::thread persistanceThread([]() { // construtor com callable com argumentos
        while  (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300)); // anotar std::this_thread
            if (!RedisDatabase::getInstance().dump("dump.my_rdb")) {
                std::cerr << "Error dumping database" << std::endl;
            }else {
                std::cout << "Database dumped to dump.my_rdb" << std::endl;
            }
        }
    }); // anotar tambem esse cara no resumo do namespace std.
    persistanceThread.detach(); // o que isso faz eh a thread ser 'desvinculada' do objeto
    // persistenceThread e passa a rodar sozinha em background independente do lifecycle de persistenceThread.
    server.run();

    return 0;
}
