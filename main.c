#include <pthread.h>
#include <mstd/common.h>
#include "src/logging.h"
#include "src/network.h"

#define PORT "25565"

int main(int argc, string argv[]) {
    pthread_t networkThread;
    pthread_create(&networkThread, null, thread_network_main, null);
    log_info("cesium:main", "started network thread");
    pthread_join(networkThread, null);
}