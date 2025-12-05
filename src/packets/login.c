#include <uuid/uuid.h>
#include <mstd/common.h>
#include "src/network.h"

#include "login.h"

bool sb_l_start_handler(net_connection* connection, void* arg) {
    packet_sb_l_start* packet = arg;

    packet_cb_l_success response = {
        .id = PACKET_ID_CB_L_SUCCESS,
        .uuid = *packet->uuid,
        .username = packet->name,
        .propertyCount = 0};
    packet_send(connection, &response);

    return true;
}