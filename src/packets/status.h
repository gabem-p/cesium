#pragma once

#include <mstd/common.h>
#include "src/network.h"
#include "src/packets/packets.h"

typedef struct packed {
    enum packet_id id;
    string json;
} packet_cb_s_response;

typedef struct packed {
    enum packet_id id;
    long timestamp;
} packet_cb_s_pong;

typedef struct packed {
    enum packet_id id;
} packet_sb_s_request;

typedef struct packed {
    enum packet_id id;
    long timestamp;
} packet_sb_s_ping;

bool sb_s_request_handler(net_connection* connection, void* packet);
bool sb_s_ping_handler(net_connection* connection, void* packet);