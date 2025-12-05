#pragma once

#include <uuid/uuid.h>
#include <mstd/common.h>
#include "src/network.h"
#include "src/packets/packets.h"

typedef struct packed {
    enum packet_id id;

    string name;
    uuid_t uuid;
} packet_sb_l_start;

typedef struct packed {
    enum packet_id id;
} packet_sb_l_ack;

typedef struct packed {
    enum packet_id id;

    uuid_t uuid;
    string username;
    int propertyCount;
    struct packed {
        string name;
        string value;
        bool hasSignature;
        string signature;
    }* properties;
} packet_cb_l_success;

bool sb_l_start_handler(net_connection* connection, void* packet);
bool sb_l_ack_handler(net_connection* connection, void* packet);