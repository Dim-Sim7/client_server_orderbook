#pragma once

#include <sys/socket.h>   // socket, bind, sendto, recvfrom
#include <netinet/in.h>   // sockaddr_in, INADDR_ANY
#include <string.h>       // strlen
#include <stdio.h>        // printf
#include <unistd.h>

#include "packet.h"
#include "orderbook.h"



class Server {
private:

    const static int HISTORY_SIZE = 100;
    
    OrderBook& book_;
    int fd_;
    int retrans_fd_;
    struct sockaddr_in addr_;
    struct sockaddr_in retrans_addr_;
    struct sockaddr_storage client_;
    socklen_t client_len_;
    OrderMsg buf_[1024];
    uint64_t seq_;
    std::array<OrderMsg, HISTORY_SIZE> history_;

public:

    Server(OrderBook& book);
    ~Server();                  // close socket fd
    Server(Server&& other) = delete;
    Server& operator=(Server&& other) = delete ;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run(); //main loop
    std::vector<OrderMsg> makeSampleOrders();
private:

    void initSockets();
    void bindPort(uint16_t port, sockaddr_in& addr, int& fd);
    void initHistory();
    void waitForClient();

    void retransmitPackets(const uint64_t from_seq, const uint64_t to_seq);
    void sendSnapshot();
    void sendPacket(OrderMsg& order);
    OrderMsg makeTestOrder();
};