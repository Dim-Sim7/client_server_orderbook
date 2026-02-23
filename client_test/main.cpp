// client.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "../packet.h"
#include <map>

int main() {
    //create a UDP socket - same as server side
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    //describe the server's address we want to send to
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9000); //must match server bound port
    

    // convert "127.0.0.1" human-readable string -> binary IP stored in server.sin_addr
    // AF_INET tells it to expect an IPv4 address string
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    // send a registration packet to the server
    // server's recvfrom will wake up and learn our address from this
    sendto(fd, "sub", 3, 0, (struct sockaddr*)&server, sizeof(server));

    // seperate address for sending retransmist requests
    struct sockaddr_in retrans_server;
    retrans_server.sin_family = AF_INET;
    retrans_server.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &retrans_server.sin_addr);
    
    // reorder buffer — holds retransmitted packets until we can apply them
    std::map<int, Packet> reorder_buffer;
    Packet pkt;
    int expected_seq = 0;
    // block waiting for the server's response
    // NULL for the last two args because we don't need to know who sent it
    while (true) {
        recvfrom(fd, &pkt, sizeof(pkt), 0, NULL, NULL);
        
        if (pkt.msg_type == MSG_RETRANSMIT) {
            // dont process immediately - store in reorder buffer
            reorder_buffer[pkt.seq] = pkt;
            printf("retransmit packet receive: seq=%d store in buffer\n", pkt.seq);

            // check if buffer fills the gap - apply in contiguous sequences
            // waits for packets to arrive in sequence (expecting 5, pkt 6 arrives - wait. 5 arrives, 5 is here!)
            while (reorder_buffer.count(expected_seq)) {
                Packet& buffered = reorder_buffer[expected_seq];
                printf("applying from buffer: %s\n", buffered.data);
                reorder_buffer.erase(expected_seq);
                expected_seq++;
            }
        } else {
            // live packet - check for gap
            if (pkt.seq != expected_seq) {
                printf("GAP! expected=%d got=%d -- requesting retransmit\n", expected_seq, pkt.seq);

                ClientRequest req;
                req.msg_type = MSG_RETRANSMIT;
                req.from_seq = expected_seq;
                req.to_seq = pkt.seq - 1;
                sendto(fd, &req, sizeof(req), 0, (struct sockaddr*)&retrans_server, sizeof(retrans_server));
            }

            expected_seq = pkt.seq + 1;
            printf("got: %s\n", pkt.data);
        }

    }
    
    return 0;
}