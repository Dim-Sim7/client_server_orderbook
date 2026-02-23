#include "server.h"


Server::Server(OrderBook& book) : book_(book), seq_(0) {
    initSockets();
    initHistory();
    waitForClient();
}

Server::~Server() {
    if (fd_ != -1)         close(fd_);
    if (retrans_fd_ != -1) close(retrans_fd_);
}

Server::Server(Server&& other) : 
            book_(other.book_),
            fd_(other.fd_),
            retrans_fd_(other.retrans_fd_),
            addr_(other.addr_),
            retrans_addr_(other.retrans_addr_),
            client_(other.client_),
            client_len_(other.client_len_),
            seq_(other.seq_)
{   
    memcpy(buf_, other.buf_, sizeof(buf_));
    memcpy(history_, other.history_, sizeof(history_));

    other.fd_ = -1;
    other.retrans_fd_ = -1;
}

void Server::run() {
    while (true) {
        // --- check for retransmit requests (non-blocking) ---
        fd_set read_fds;
        FD_ZERO(&read_fds);     //clear the set
        FD_SET(retrans_fd_, &read_fds); //add retrans_fd to watch list

        //timeout of 0 = dont block, just check right now
        struct timeval timeout = {0, 0};
        int ready = select(retrans_fd_ + 1, &read_fds, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(retrans_fd_, &read_fds)) {
            // a retransmit request came in
            ClientRequest req;
            recvfrom(retrans_fd_, &req, sizeof(req), 0, NULL, NULL);

            if (req.msg_type == MSG_RETRANSMIT) {
                //replay those sequence numbers from history
                for (int i = req.from_seq; i <= req.to_seq; i++) {
                    Packet* old_pkt = (Packet*)history_[i % HISTORY_SIZE];
                    old_pkt->msg_type = MSG_RETRANSMIT; //stamp as retransmit
                    sendto(fd_, old_pkt, sizeof(Packet), 0,
                        (struct sockaddr*)&client_, client_len_);
                }
            }
            else if (req.msg_type == MSG_SNAPSHOT_REQUEST) {
                std::vector<SnapshotOrder> allOrders = book_.getAllOrders();
                //send all orders to client
                int total = allOrders.size();

                SnapshotPacket pkt;
                pkt.msg_type = MSG_SNAPSHOT;
                pkt.snapshot_seq = seq_;

                for (int i = 0; i < total; ++i) {
                    pkt.orders[i % SNAPSHOT_BATCH_SIZE] = allOrders[i];
                    if (i % SNAPSHOT_BATCH_SIZE == SNAPSHOT_BATCH_SIZE - 1 || i == total - 1) {
                        // send
                        pkt.orders_in_batch = (i % SNAPSHOT_BATCH_SIZE) + 1;
                        pkt.batches_remaining = (total - i - 1) / SNAPSHOT_BATCH_SIZE;
                        sendto(fd_, &pkt, sizeof(pkt), 0, (struct sockaddr*)&client_, client_len_);
                    }

                }
            }

        }

        // send next live update
        Packet pkt;
        pkt.msg_type = MSG_LIVE;
        pkt.seq      = seq_;
        snprintf(pkt.data, sizeof(pkt.data), "update seq=%d", seq_);

        // store in history before sending
        memcpy(history_[seq_ % HISTORY_SIZE], &pkt, sizeof(pkt));

        sendto(fd_, &pkt, sizeof(pkt), 0, (struct sockaddr*)&client_, client_len_);

        seq_++;
        //usleep(500);
    }
}

void Server::initSockets() {
    bindPort(9000, addr_, fd_);
    bindPort(9001, retrans_addr_, retrans_fd_);
}

void Server::bindPort(int port, sockaddr_in& addr, int& fd) {
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