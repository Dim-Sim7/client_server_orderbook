// client.h
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "packet.h"
#include "orderbook.h"
#include <map>
#include "orderbook.h"
#include "packet.h"
#include <unistd.h>

class Client {
private:
    OrderBook book_;
    uint16_t fd_;
    struct sockaddr_in server_;
    struct sockaddr_in retrans_server_;
    std::map<int, Order> reorder_buffer_;
    OrderMsg pkt_; // container for packet data, passed around
    int expected_seq_;

    struct timespec gap_start_;
    bool gap_active_;
    bool waiting_for_snapshot_;

    void initSockets();
    void registerToServer();
    void receiveRetransmit();
    void receiveSnapshot();
    void receiveLivePacket();
    void bindToServer(uint16_t port, sockaddr_in& addr);

public:
    Client(OrderBook& book);
    ~Client();
    Client(Client&& other) = delete;
    Client& operator=(Client&& other) = delete;
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;


    void run();

};