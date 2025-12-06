#pragma once

#include <mstd/common.h>
#include "src/network.h"
#include "src/packets/packets.h"

typedef struct packed {
    enum packet_id id;

    string locale;
    sbyte renderDistance;
    enum : int {
        CHAT_ENABLED,
        CHAT_COMMANDS_ONLY,
        CHAT_HIDDEN
    } chatMode;
    bool chatColors;
    byte skinParts;
    enum : int {
        MAIN_HAND_LEFT,
        MAIN_HAND_RIGHT
    } mainHand;
    bool textFiltering;
    bool anonymous;
    enum : int {
        PARTICLES_ALL,
        PARTICLES_DECREASED,
        PARTICLES_MINIMAL
    } particles;
} packet_sb_c_information;

bool sb_c_information_handler(net_connection* connection, void* packet);