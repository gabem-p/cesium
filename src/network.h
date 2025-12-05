#pragma once

#include <mstd/common.h>

enum connection_state {
    STATE_HANDSHAKING,
    STATE_STATUS,
    STATE_LOGIN,
    STATE_CONFIGURATION
};

typedef struct {
    int sock;
    ulong thread;
    enum connection_state state;
} net_connection;

void* thread_network_main();