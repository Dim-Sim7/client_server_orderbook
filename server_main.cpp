#include <stdio.h>
#include "orderbook.h"
#include "server.h"

#include <thread>

int main() {
    OrderBook server_book;

    
    Server server(server_book);

    // run in separate threads
    std::thread server_thread(&Server::run, &server);
    
    server_thread.join();
    return 0;
}