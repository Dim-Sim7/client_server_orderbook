#include <stdio.h>
#include "orderbook.h"
#include "server.h"

#include <thread>

int main() {
    Server server{};

    // run in separate threads
    std::thread server_thread(&Server::run, &server);
    
    server_thread.join();
    return 0;
}