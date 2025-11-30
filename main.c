#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <mstd/common.h>
#include "src/logging.h"

#define PORT "25565"

int main(int argc, string argv[]) {
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* results;
    bool error = getaddrinfo(null, PORT, &hints, &results);
    if (error) {
        ccm_log_error("cesium:network", "getaddrinfo failed");
    }
}