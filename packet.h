#pragma once
#include <cstdint>

constexpr int SERVER_PORT  = 9000;
constexpr int RETRANS_PORT = 9001;

#define MSG_LIVE             1
#define MSG_RETRANSMIT       2
#define MSG_SNAPSHOT         3
#define MSG_SNAPSHOT_REQUEST 4

#define SNAPSHOT_BATCH_SIZE  50



typedef struct {
    uint8_t  msg_type;   // MSG_ADD, MSG_CANCEL
    uint8_t  side;       // 'B' or 'S'
    uint8_t  order_type; // 'L' or 'M'
    uint8_t  padding;
    uint64_t seq;
    uint32_t qty;
    int32_t  price;
    uint64_t order_id;
} OrderMsg;

typedef struct {
    uint8_t msg_type;
    uint64_t from_seq;
    uint64_t to_seq;
} ClientRequest;
