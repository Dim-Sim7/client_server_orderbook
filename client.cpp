#include "client.h"
#include <thread>
#include <chrono>
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
    printf("retransmit packet receive: seq=%ld store in buffer\n", pkt_.seq);

    // check if buffer fills the gap - apply in contiguous sequences
    // waits for packets to arrive in sequence (expecting 5, pkt 6 arrives - wait. 5 arrives, 5 is here!)
    while (reorder_buffer_.count(expected_seq_)) {
        OrderMsg& buffered = reorder_buffer_[expected_seq_];
        printf("applying from buffer: %ld\n", buffered.seq);
        int32_t price = buffered.price;
        book_.addOrder(buffered.order_id,
                    buffered.side == 'B' ? Side::BUY : Side::SELL,
                    std::optional<int32_t>(price),
                    buffered.qty,
                    buffered.order_type == 'L' ? LIMIT : MARKET);
        reorder_buffer_.erase(expected_seq_);
        expected_seq_++;
    }
}

void Client::receiveLivePacket() {
    if (pkt_.seq != expected_seq_) {
        printf("GAP! expected=%ld got=%ld -- requesting retransmit\n", expected_seq_, pkt_.seq);
        ClientRequest req;
        req.msg_type = MSG_RETRANSMIT;
        req.from_seq = expected_seq_;
        req.to_seq = pkt_.seq - 1;
        sendto(fd_, &req, sizeof(req), 0, (struct sockaddr*)&retrans_server_, sizeof(retrans_server_));
    }

    expected_seq_ = pkt_.seq + 1;
    book_.addOrder(pkt_.order_id, pkt_.side == 'B' ? Side::BUY : Side::SELL, std::optional<int32_t>(pkt_.price), 
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        book_.print();

    }
}
