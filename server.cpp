#include "server.h"
#include <algorithm>
#include <thread>
#include <chrono>
Server::Server(OrderBook& book) : book_(book), seq_(0) {
    initSockets();
    initHistory();
    
}

Server::~Server() {
    if (fd_ != -1)         close(fd_);
    if (retrans_fd_ != -1) close(retrans_fd_);
}


void Server::run() {
    waitForClient();
    std::vector<OrderMsg> orders = makeSampleOrders();
    size_t idx = 0;
    std::cout << "Client connected\n";
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
            std::cout << "Retransmit request received\n";
            // a retransmit request came in
            ClientRequest req;
            recvfrom(retrans_fd_, &req, sizeof(req), 0, NULL, NULL);
            //replay those sequence numbers from history
            retransmitPackets(req.from_seq, req.to_seq);
        }
        if (idx < orders.size()) {
            std::cout << "Sending packet " << idx << "\n";
            sendPacket(orders[idx++]);
        } 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::vector<OrderMsg> Server::makeSampleOrders() {
    std::vector<OrderMsg> orders;

    auto makeOrder = [](uint64_t id,
                        uint8_t side,
                        int32_t price,
                        uint32_t qty,
                        uint8_t type,
                        uint32_t seq) {

        OrderMsg msg{};

        msg.msg_type   = MSG_LIVE;
        msg.side       = side;
        msg.order_type = type;
        msg.padding    = 0;
        msg.seq        = seq;
        msg.qty        = qty;
        msg.price      = price;
        msg.order_id   = id;

        return msg;
    };

    uint32_t seq = 0;

    // =========================
    // INITIAL BUY DEPTH
    // =========================

    orders.push_back(makeOrder(0,  'B', 10000, 100, 'L', seq++));
    orders.push_back(makeOrder(1,  'B', 10000, 200, 'L', seq++));
    orders.push_back(makeOrder(2,  'B',  9950, 150, 'L', seq++));
    orders.push_back(makeOrder(3,  'B',  9900, 300, 'L', seq++));
    orders.push_back(makeOrder(4,  'B',  9850,  50, 'L', seq++));
    orders.push_back(makeOrder(5,  'B',  9800, 500, 'L', seq++));
    orders.push_back(makeOrder(6,  'B',  9750, 250, 'L', seq++));

    // =========================
    // INITIAL SELL DEPTH
    // =========================

    orders.push_back(makeOrder(7,  'S', 10050, 100, 'L', seq++));
    orders.push_back(makeOrder(8,  'S', 10050, 200, 'L', seq++));
    orders.push_back(makeOrder(9,  'S', 10100, 150, 'L', seq++));
    orders.push_back(makeOrder(10, 'S', 10150, 300, 'L', seq++));
    orders.push_back(makeOrder(11, 'S', 10200,  50, 'L', seq++));
    orders.push_back(makeOrder(12, 'S', 10250, 400, 'L', seq++));
    orders.push_back(makeOrder(13, 'S', 10300, 250, 'L', seq++));

    // =========================
    // MARKET ORDERS
    // =========================

    // market buy sweeps asks
    orders.push_back(makeOrder(14, 'B', 0, 100, 'M', seq++));
    orders.push_back(makeOrder(15, 'B', 0, 250, 'M', seq++));
    orders.push_back(makeOrder(16, 'B', 0, 300, 'M', seq++));

    // market sell sweeps bids
    orders.push_back(makeOrder(17, 'S', 0, 100, 'M', seq++));
    orders.push_back(makeOrder(18, 'S', 0, 400, 'M', seq++));
    orders.push_back(makeOrder(19, 'S', 0, 150, 'M', seq++));

    // =========================
    // AGGRESSIVE LIMIT ORDERS
    // =========================

    // crosses spread immediately
    orders.push_back(makeOrder(20, 'B', 10150, 200, 'L', seq++));
    orders.push_back(makeOrder(21, 'S',  9950, 300, 'L', seq++));

    // =========================
    // PASSIVE LIMIT ORDERS
    // =========================

    orders.push_back(makeOrder(22, 'B',  9700, 1000, 'L', seq++));
    orders.push_back(makeOrder(23, 'S', 10400,  800, 'L', seq++));

    // =========================
    // SAME PRICE FIFO TESTS
    // =========================

    orders.push_back(makeOrder(24, 'B', 10000,  50, 'L', seq++));
    orders.push_back(makeOrder(25, 'B', 10000,  75, 'L', seq++));
    orders.push_back(makeOrder(26, 'S', 10100, 125, 'L', seq++));
    orders.push_back(makeOrder(27, 'S', 10100, 175, 'L', seq++));

    // =========================
    // LARGE SWEEP
    // =========================

    orders.push_back(makeOrder(28, 'B', 0, 1000, 'M', seq++));
    orders.push_back(makeOrder(29, 'S', 0, 1200, 'M', seq++));

    return orders;
}

void Server::retransmitPackets(const uint64_t from_seq, const uint64_t to_seq) {
    for (size_t i{static_cast<size_t>(from_seq)}; i <= static_cast<size_t>(to_seq); i++) {
        OrderMsg* old_pkt = &history_[i % HISTORY_SIZE];
        old_pkt->msg_type = MSG_RETRANSMIT; //stamp as retransmit
        sendto(fd_, old_pkt, sizeof(OrderMsg), 0, (struct sockaddr*)&client_, client_len_);
    }
}

void Server::sendPacket(OrderMsg& order) {
    // send next live update
    // store in history before sending
    memcpy((void*)&history_[seq_ % HISTORY_SIZE], &order, sizeof(order));

    sendto(fd_, &order, sizeof(order), 0, (struct sockaddr*)&client_, client_len_);

    seq_++;
    usleep(500);
}

OrderMsg Server::makeTestOrder() {
    OrderMsg msg;
    msg.msg_type   = MSG_LIVE;
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
    history_.fill(OrderMsg{});
}

void Server::waitForClient() {
    client_len_ = sizeof(client_);

    ssize_t n = recvfrom(fd_,
                         buf_,
                         sizeof(buf_),
                         0,
                         (struct sockaddr*)&client_,
                         &client_len_);

    if (n < 0) {
        perror("recvfrom failed");
        return;
    }
    auto* addr = (sockaddr_in*)&client_;

    printf("client registered\n");
    printf("client port=%d\n", ntohs(addr->sin_port));

}