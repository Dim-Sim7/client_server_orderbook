#include "client.h"

#include "packet.h"


Client::Client(OrderBook& book) : book_(book), expected_seq_(0)
{
    initSockets();
    registerToServer();
}

Client::~Client() {
    close(fd_);
}


void Client::initSockets() {
    // create a UDP socket
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    bindToServer(SERVER_PORT, server_);
    bindToServer(RETRANS_PORT, retrans_server_);
}
void Client::registerToServer() {
    sendto(fd_, "sub", 3, 0, (struct sockaddr*)&server_, sizeof(server_));
}

void Client::bindToServer(uint16_t port, sockaddr_in& addr) {
    
    // describe server's address we want to send to
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
}

void Client::receiveRetransmit() {
    // dont process immediately - store in reorder buffer
    reorder_buffer_.at(pkt_.seq) = pkt_;
    printf("retransmit packet receive: seq=%d store in buffer\n", pkt_.seq);

    // check if buffer fills the gap - apply in contiguous sequences
    // waits for packets to arrive in sequence (expecting 5, pkt 6 arrives - wait. 5 arrives, 5 is here!)
    while (reorder_buffer_.count(expected_seq_)) {
        Order& buffered = reorder_buffer_[expected_seq_];
        printf("applying from buffer: %s\n", buffered.data);
        reorder_buffer_.erase(expected_seq_);
        expected_seq_++;
    }
}

void Client::receiveLivePacket() {
    if (pkt_.seq != expected_seq_) {
        printf("GAP! expected=%d got=%d -- requesting retransmit\n", expected_seq_, pkt_.seq);
        ClientRequest req;
        req.msg_type = MSG_RETRANSMIT;
        req.from_seq = expected_seq_;
        req.to_seq = pkt_.seq - 1;
        sendto(fd_, &req, sizeof(req), 0, (struct sockaddr*)&retrans_server_, sizeof(retrans_server_));
    }

    expected_seq_ = pkt_.seq + 1;
    printf("got: %s\n", pkt_.seq);
    book_.addOrder(pkt_.order_id, pkt_.side == 'B' ? Side::BUY : Side::SELL, pkt_.price, 
                pkt_.qty, pkt_.order_type == 'L' ? OrderType::LIMIT : OrderType::MARKET);
}

void Client::run() {
    while(true) { 
        // block waiting for the server's response
        // NULL for the last two args because we don't need to know who sent it
        recvfrom(fd_, &pkt_, sizeof(pkt_), 0, NULL, NULL);
            //retransmit packet
            if (pkt_.msg_type == MSG_RETRANSMIT) {
                receiveRetransmit();
            } else {
                // live packet - check for gap
                receiveLivePacket();
            }
    }
}
