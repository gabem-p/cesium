#include <mstd/common.h>
#include "src/network.h"
#include "src/logging.h"
#include "src/packets/packets.h"

#include "status.h"

bool sb_s_request_handler(net_connection* connection, void* arg) {
    packet_sb_s_request* packet = arg;

    FILE* file = fopen("response.json", "r");
    fseek(file, 0, SEEK_END);
    uint length = ftell(file);
    fseek(file, 0, SEEK_SET);

    string json = alloca(length + 1);
    fread(json, length, 1, file);
    json[length] = '\0';

    packet_cb_s_response response = {
        .id = PACKET_ID_CB_S_RESPONSE,
        .json = json};
    packet_send(connection, &response);

    fclose(file);

    return true;
}
bool sb_s_ping_handler(net_connection* connection, void* arg) {
    packet_sb_s_ping* packet = arg;

    packet_cb_s_pong pong = {
        .id = PACKET_ID_CB_S_PONG,
        .timestamp = packet->timestamp};
    packet_send(connection, &pong);

    return true;
}