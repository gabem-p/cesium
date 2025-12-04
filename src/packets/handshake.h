#pragma once

#include <mstd/common.h>
#include "src/network.h"
#include "src/packets/packets.h"

typedef struct packed {
    enum packet_id id;

    int protocolVersion;
    string serverAddress;
    ushort serverPort;
    enum : uint {
        INTENT_STATUS = 1,
        INTENT_LOGIN = 2,
        INTENT_TRANSFER = 3
    } intent;
} packet_sb_h_handshake;

bool sb_h_handshake_handler(net_connection* connection, void* packet);