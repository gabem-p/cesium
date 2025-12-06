#include <mstd/common.h>
#include "src/logging.h"
#include "src/network.h"

#include "configuration.h"

bool sb_c_information_handler(net_connection* connection, void* arg) {
    packet_sb_c_information* packet = arg;

    log_info("cesium:network", "locale=%s renderDistance=%i chatMode=%i chatColors=%i skinParts=%X mainHand=%i filtering=%i anonymous=%i particles=%i",
             packet->locale, packet->renderDistance, packet->chatMode, packet->chatColors,
             packet->skinParts, packet->mainHand, packet->textFiltering, packet->anonymous, packet->particles);

    return true;
}