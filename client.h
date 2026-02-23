// client.h
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "packet.h"
#include <map>

class Client {
private:
    int fd_;
    struct sockaddr_in server_;
    struct sockaddr_in retrans_server_;
    std::map<int, Packet> reorder_buffer_;
    Packet pkt_;
    int expected_seq_;
public:

};