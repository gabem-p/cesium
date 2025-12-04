#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <mstd/common.h>
#include "src/logging.h"
#include "src/packets/packets.h"

#include "network.h"

#define PORT "25565"

void* thread_network_connection(void* arg) {
    net_connection* connection = arg;
    byte lengthBuffer[3];

    while (true) {
        for (uint bytes = 0; bytes < 3;) {
            bytes += recv(connection->sock, lengthBuffer + bytes, 1, 0);
            if (bytes <= 0)
                goto terminate;
            if ((lengthBuffer[bytes - 1] & 0x80) == 0)
                break;
        }

        int length;
        decode_varint(lengthBuffer, &length);

        byte* data = malloc(length);
        for (uint bytes = 0; bytes < length;)
            bytes += recv(connection->sock, data + bytes, length, 0);

        packet_handle(connection, data);

        free(data);
    }

terminate:
    close(connection->sock);
    free(connection);
    pthread_exit(EXIT_SUCCESS);
}

void* thread_network_main() {
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* results;
    bool error = getaddrinfo(null, PORT, &hints, &results);
    if (error) {
        log_error("cesium:network", "getaddrinfo() failed with message: %s", gai_strerror(error));
        pthread_exit(EXIT_FAILURE);
    }

    int sock = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){true}, sizeof(int));
    if (sock == -1) {
        log_error("cesium:network", "socket() failed with message: %s", strerror(errno));
        pthread_exit(EXIT_FAILURE);
    }
    if (bind(sock, results->ai_addr, results->ai_addrlen) == -1) {
        log_error("cesium:network", "bind() failed with message: %s", strerror(errno));
        close(sock);
        pthread_exit(EXIT_FAILURE);
    }
    if (listen(sock, 10) == -1) {
        log_error("cesium:network", "listen() failed with message: %s", strerror(errno));
        close(sock);
        pthread_exit(EXIT_FAILURE);
    }

    while (true) {
        int newsock = accept(sock, null, null);
        if (newsock == -1) {
            log_error("cesium:network", "accept() failed with message: %s", strerror(errno));
            close(sock);
            pthread_exit(EXIT_FAILURE);
        }

        net_connection* connection = malloc(sizeof(net_connection));
        connection->sock = newsock;
        connection->state = STATE_HANDSHAKING;

        pthread_create(&connection->thread, null, thread_network_connection, connection);
        pthread_detach(connection->thread);
    }

    close(sock);
    pthread_exit(EXIT_SUCCESS);
}