#include <uuid/uuid.h>
#include <mstd/common.h>
#include "src/logging.h"
#include "src/network.h"

#include "login.h"

bool sb_l_start_handler(net_connection* connection, void* arg) {
    packet_sb_l_start* packet = arg;

    char uuid[36];
    uuid_unparse(packet->uuid, uuid);
    log_info("cesium:network", "<%s> logged in (UUID %s)", packet->name, uuid);

    packet_cb_l_success response = {
        .id = PACKET_ID_CB_L_SUCCESS,
        .uuid = *packet->uuid,
        .username = packet->name,
        .propertyCount = 0};
    packet_send(connection, &response);

    return true;
}

bool sb_l_ack_handler(net_connection* connection, void* arg) {
    packet_sb_l_ack* packet = arg;

    connection->state = STATE_CONFIGURATION;

    return true;
}