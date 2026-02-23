#include <sys/socket.h>   // socket, bind, sendto, recvfrom
#include <netinet/in.h>   // sockaddr_in, INADDR_ANY
#include <string.h>       // strlen
#include <stdio.h>        // printf
#include <unistd.h>

#include "../packet.h"
#include "../orderbook.h"

const int HISTORY_SIZE = 10000;
char history[HISTORY_SIZE][64];

int main() {
    // ask the OS to create a UDP socket, returns a file descriptor (just an int)
    // AF_INET = IPv4, SOCK_DGRAM = UDP
    // --- socket 1: live updates + client registration ---
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET; //IPv4
    addr.sin_port        = htons(9000); // htons converts to big-endian
    addr.sin_addr.s_addr = INADDR_ANY; //accept packets on any network interface
    // claim port 9000 on this machine - incoming UDP packets to :9000 now route to fd
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));


    // --- socket 2: listens for retransmit requests ---
    int retrans_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in retrans_addr;
    retrans_addr.sin_family = AF_INET;
    retrans_addr.sin_port = htons(9001); //different port
    retrans_addr.sin_addr.s_addr = INADDR_ANY;
    bind(retrans_fd, (struct sockaddr*)&retrans_addr, sizeof(retrans_addr));


    //wait for client to register
    char buf[1024];
    // sockaddr_storage is a generic container big enough to hold any addr type
    struct sockaddr_storage client;
    socklen_t client_len = sizeof(client);
    //block until a packet arrives, when it does, buf gets the data
    // and client gets the sender's
    recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&client, &client_len);
    printf("client registered\n");

    int seq = 0;

    //live send loop
    while (true) {
        // --- check for retransmit requests (non-blocking) ---
        fd_set read_fds;
        FD_ZERO(&read_fds);     //clear the set
        FD_SET(retrans_fd, &read_fds); //add retrans_fd to watch list

        //timeout of 0 = dont block, just check right now
        struct timeval timeout = {0, 0};
        int ready = select(retrans_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(retrans_fd, &read_fds)) {
            // a retransmit request came in
            ClientRequest req;
            recvfrom(retrans_fd, &req, sizeof(req), 0, NULL, NULL);

            if (req.msg_type == MSG_RETRANSMIT) {
                //replay those sequence numbers from history
                for (int i = req.from_seq; i <= req.to_seq; i++) {
                    Packet* old_pkt = (Packet*)history[i % HISTORY_SIZE];
                    old_pkt->msg_type = MSG_RETRANSMIT; //stamp as retransmit
                    sendto(fd, old_pkt, sizeof(Packet), 0,
                        (struct sockaddr*)&client, client_len);
                }
            }
            else if (req.msg_type == MSG_SNAPSHOT) {
                std::vector<SnapshotOrder> allOrders = 
            }

        }

        // send next live update
        Packet pkt;
        pkt.msg_type = MSG_LIVE;
        pkt.seq      = seq;
        snprintf(pkt.data, sizeof(pkt.data), "update seq=%d", seq);

        // store in history before sending
        memcpy(history[seq % HISTORY_SIZE], &pkt, sizeof(pkt));

        sendto(fd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&client, client_len);

        seq++;
        //usleep(500);
    }

    return 0;
}