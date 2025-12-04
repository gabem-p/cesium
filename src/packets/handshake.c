#include <mstd/common.h>
#include "src/network.h"
#include "src/logging.h"

#include "handshake.h"

bool sb_h_handshake_handler(net_connection* connection, void* arg) {
    packet_sb_h_handshake* packet = arg;

    ccm_log_info_f("cesium:network", "<%i> handshake address=%s:%i intent=%i protocol=%i",
                   connection->sock, packet->serverAddress, packet->serverPort, packet->intent, packet->protocolVersion);

    switch (packet->intent) {
    case INTENT_STATUS:
        connection->state = STATE_STATUS;
        break;
    case INTENT_LOGIN:
        connection->state = STATE_LOGIN;
        break;
    case INTENT_TRANSFER:
        break;
    }

    return true;
}