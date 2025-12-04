#pragma once

#include <mstd/common.h>

void* thread_network_main();

enum connection_state {
    STATE_HANDSHAKING,
    STATE_STATUS,
    STATE_LOGIN
};

typedef struct {
    int sock;
    ulong thread;
    enum connection_state state;
} net_connection;