#pragma once
#include <cstdint>

#define MSG_LIVE             1
#define MSG_RETRANSMIT       2
#define MSG_SNAPSHOT         3
#define MSG_SNAPSHOT_REQUEST 4

#define SNAPSHOT_BATCH_SIZE  50

typedef struct {
    uint8_t msg_type;
    int     seq;
    char    data[64];
} Packet;

typedef struct {
    uint8_t msg_type;
    int     from_seq;
    int     to_seq;
} ClientRequest;

typedef struct {
    int  price;
    int  qty;
    char side;
    char type;
    int  order_id;
} SnapshotOrder;

typedef struct {
    uint8_t       msg_type;
    int           snapshot_seq;
    int           orders_in_batch;
    int           batches_remaining;
    SnapshotOrder orders[SNAPSHOT_BATCH_SIZE];
} SnapshotPacket;