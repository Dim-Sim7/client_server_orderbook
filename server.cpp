#include "server.h"
#include <algorithm>

Server::Server(OrderBook& book) : book_(book), seq_(0) {
    initSockets();
    initHistory();
    waitForClient();
}

Server::~Server() {
    if (fd_ != -1)         close(fd_);
    if (retrans_fd_ != -1) close(retrans_fd_);
}


void Server::run() {
    while (true) {
        // --- check for retransmit requests (non-blocking) ---
        fd_set read_fds;
        FD_ZERO(&read_fds);     //clear the set
        FD_SET(retrans_fd_, &read_fds); //add retrans_fd to watch list

        //timeout of 0 = dont block, just check right now
        struct timeval timeout = {0, 0};
        int nfds = std::max(fd_, retrans_fd_) + 1;
        int ready = select(nfds, &read_fds, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(retrans_fd_, &read_fds)) {
            // a retransmit request came in
            ClientRequest req;
            recvfrom(retrans_fd_, &req, sizeof(req), 0, NULL, NULL);
            //replay those sequence numbers from history
            retransmitPackets(req.from_seq, req.to_seq);
        }
        sendPacket();
    }
}

void Server::retransmitPackets(const int from_seq, const int to_seq) {
    for (size_t i{static_cast<size_t>(from_seq)}; i <= static_cast<size_t>(to_seq); i++) {
        Packet* old_pkt = (Packet*)history_[i % HISTORY_SIZE];
        old_pkt->msg_type = MSG_RETRANSMIT; //stamp as retransmit
        sendto(fd_, old_pkt, sizeof(Packet), 0, (struct sockaddr*)&client_, client_len_);
    }
}

void Server::sendPacket() {
    // send next live update
    OrderMsg order = makeTestOrder();
    // store in history before sending
    memcpy(history_[seq_ % HISTORY_SIZE], &order, sizeof(order));

    sendto(fd_, &order, sizeof(order), 0, (struct sockaddr*)&client_, client_len_);

    seq_++;
    usleep(500);
}

OrderMsg Server::makeTestOrder() {
    OrderMsg msg;
    msg.msg_type   = MSG_ADD;
    msg.side       = 'B';
    msg.order_type = 'L';
    msg.seq        = seq_;
    msg.qty        = 100;
    msg.price      = 10000 + (rand() % 100);  // random price around 100.00
    msg.order_id   = seq_;
    return msg;
}

void Server::initSockets() {
    bindPort(SERVER_PORT, addr_, fd_);
    bindPort(RETRANS_PORT, retrans_addr_, retrans_fd_);
}

void Server::bindPort(uint16_t port, sockaddr_in& addr, int& fd) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);  // create socket first
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
}

void Server::initHistory() {
    memset(history_, 0, sizeof(history_));
}

void Server::waitForClient() {
    recvfrom(fd_, buf_, sizeof(buf_), 0, (struct sockaddr*)&client_, &client_len_);
    printf("client registered\n");
}