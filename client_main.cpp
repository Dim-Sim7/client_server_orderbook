#include <stdio.h>
#include "orderbook.h"
#include "client.h"

#include <thread>

int main() {
    OrderBook client_book;

    
    Client client(client_book);

    // run in separate threads
    std::thread client_thread(&Client::run, &client);
    
    client_thread.join();
    return 0;
}