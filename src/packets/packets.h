#pragma once

#include <mstd/common.h>
#include "src/network.h"

enum packet_id : int {
    PACKET_ID_SB_H_HANDSHAKE = 0x00,

    PACKET_ID_CB_S_RESPONSE = 0x00,
    PACKET_ID_CB_S_PONG = 0x01,
    PACKET_ID_SB_S_REQUEST = 0x00,
    PACKET_ID_SB_S_PING = 0x01
};

typedef bool (*packet_handler)(net_connection* connection, void* packet);

typedef struct {
    enum packet_id id;
    uint size;
    packet_handler handler;
    string format;
} packet_format;

enum packet_handle_status {
    HANDLE_SUCCESS,
    HANDLE_ERROR,
    HANDLE_UNKNOWN
};

byte decode_varint(byte* start, int* out);
byte decode_varlong(byte* start, long* out);
byte encode_varint(byte* start, int value);
byte encode_varlong(byte* start, long value);

enum packet_handle_status packet_handle(net_connection* connection, byte* data);
void packet_send(net_connection* connection, void* data);