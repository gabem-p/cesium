#include <string.h>
#include <sys/socket.h>
#include <mstd/common.h>
#include <mstd/types/stack.h>
#include "src/logging.h"
#include "src/packets/handshake.h"
#include "src/packets/status.h"
#include "src/network.h"

#include "packets.h"

const packet_format PACKET_FORMATS_HANDSHAKING[] = {
    {.id = PACKET_ID_SB_H_HANDSHAKE, .size = sizeof(packet_sb_h_handshake), .handler = sb_h_handshake_handler, .format = "v32 s255 u16 v32"},
};

const packet_format PACKET_FORMATS_STATUS[] = {
    {.id = PACKET_ID_SB_S_REQUEST, .size = sizeof(packet_sb_s_request), .handler = sb_s_request_handler, .format = ""},
    {.id = PACKET_ID_SB_S_PING, .size = sizeof(packet_sb_s_ping), .handler = sb_s_ping_handler, .format = "i64"},
    {.id = PACKET_ID_CB_S_RESPONSE, .size = sizeof(packet_cb_s_response), .handler = null, .format = "s"},
    {.id = PACKET_ID_CB_S_PONG, .size = sizeof(packet_cb_s_pong), .handler = null, .format = "i64"},
};

const packet_format PACKET_FORMATS_LOGIN[] = {};

byte decode_varint(byte* start, int* out) {
    int value = 0;
    byte i;
    for (i = 0; i < 5; i++) {
        value |= (*(start + i) & 0x7F) << (7 * i);
        if ((*(start + i) & 0x80) == 0)
            break;
    }
    *out = value;
    return i + 1;
}

byte decode_varlong(byte* start, long* out) {
    long value = 0;
    byte i;
    for (i = 0; i < 10; i++) {
        value |= (*(start + i) & 0x7F) << (7 * i);
        if ((*(start + i) & 0x80) == 0)
            break;
    }
    *out = value;
    return i + 1;
}

byte encode_varint(byte* start, int value) {
    byte i;
    for (i = 0; i < 5; i++) {
        if ((value & ~0x7F) == 0) {
            *(start + i++) = value;
            break;
        }
        *(start + i) = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    return i;
}

byte encode_varlong(byte* start, long value) {
    byte i;
    for (i = 0; i < 10; i++) {
        if ((value & ~0x7F) == 0) {
            *(start + i++) = value;
            break;
        }
        *(start + i) = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    return i;
}

enum packet_handle_status packet_handle(net_connection* connection, byte* data) {
    enum packet_handle_status status = HANDLE_ERROR;

    packet_format format;
    enum packet_id id;
    data += decode_varint(data, &id);

#define search_sb(array) { \
    for (uint i = 0; i < sizeof(array) / sizeof(packet_format); i++) { \
        if (array[i].id != id || array[i].handler == null) \
            continue; \
        format = array[i]; \
        goto found; \
    } \
}

    switch (connection->state) {
    case STATE_HANDSHAKING:
        search_sb(PACKET_FORMATS_HANDSHAKING);
        log_warning("cesium:network", "unknown packet with id handshaking:%i", id);
        return HANDLE_UNKNOWN;
    case STATE_STATUS:
        search_sb(PACKET_FORMATS_STATUS);
        log_warning("cesium:network", "unknown packet with id status:%i", id);
        return HANDLE_UNKNOWN;
    case STATE_LOGIN:
        search_sb(PACKET_FORMATS_LOGIN);
        log_warning("cesium:network", "unknown packet with id login:%i", id);
        return HANDLE_UNKNOWN;
    }

found:
    uint n = 0;
    byte* packet = malloc(format.size);
    for (uint j = 0; j < sizeof(enum packet_id); j++)
        packet[n++] = ((byte*)&id)[j];

    stack* allocStack = stack_new(256);

    uint formatLength = strlen(format.format);
    for (uint i = 0; i < formatLength; i++) {
        char type = format.format[i];
        long size = 0;
        if (format.format[i++] != ' ') {
            uint start = i;
            while (format.format[i + 1] != ' ')
                i++;
            char* end = format.format + i + 1;
            size = strtol(format.format + start, &end, 10);
        }

        switch (type) {
        case 'u':
        case 'i':
            n += size / 8;
            for (uint j = 0; j < size / 8; j++)
                packet[n - j - 1] = *(data++);
            break;
        case 'v':
            if (size == 32) {
                int value;
                data += decode_varint(data, &value);
                for (uint j = 0; j < sizeof(int); j++)
                    packet[n++] = ((byte*)&value)[j];
            } else if (size == 64) {
                long value;
                data += decode_varlong(data, &value);
                for (uint j = 0; j < sizeof(long); j++)
                    packet[n++] = ((byte*)&value)[j];
            }
            break;
        case 'b':
            packet[n++] = *(data++);
            break;
        case 's':
            int length;
            data += decode_varint(data, &length);
            if ((size != 0 && length > size) || (size == 0 && length >= 32768)) {
                log_error("cesium:network", "packet string exceeded max character count");
                goto error;
            }
            string str = malloc(length + 1);
            for (uint j = 0; j < length; j++)
                str[j] = *(data++);
            str[length] = '\0';
            for (uint j = 0; j < sizeof(string); j++)
                packet[n++] = ((byte*)&str)[j];
            stack_push(allocStack, str);
            break;
        }

        i++;
    }

    if (format.handler(connection, packet))
        status = HANDLE_SUCCESS;

error:
    for (uint i = 0; i < allocStack->count; i++)
        free(stack_pop(allocStack));
    stack_cleanup(allocStack);
    free(packet);
    return status;
}

void packet_send(net_connection* connection, void* packet) {
    byte* bytes = packet;

    packet_format format;
    enum packet_id id;
    for (uint j = 0; j < sizeof(enum packet_id); j++)
        ((byte*)&id)[j] = *(bytes++);

#define search_cb(array) { \
    for (uint i = 0; i < sizeof(array) / sizeof(packet_format); i++) { \
        if (array[i].id != id || array[i].handler != null) \
            continue; \
        format = array[i]; \
        goto found; \
    } \
}

    switch (connection->state) {
    case STATE_HANDSHAKING:
        search_cb(PACKET_FORMATS_HANDSHAKING);
        return;
    case STATE_STATUS:
        search_cb(PACKET_FORMATS_STATUS);
        return;
    case STATE_LOGIN:
        search_cb(PACKET_FORMATS_LOGIN);
        return;
    }

found:
    uint n = 0;
    uint size = 32768;
    byte* buffer = malloc(size);

    uint formatLength = strlen(format.format);
    for (uint i = 0; i < formatLength; i++) {
        char type = format.format[i];
        long size = 0;
        if (format.format[i++] != ' ') {
            uint start = i;
            while (format.format[i + 1] != ' ')
                i++;
            char* end = format.format + i + 1;
            size = strtol(format.format + start, &end, 10);
        }

        switch (type) {
        case 'u':
        case 'i':
            if (n + size >= 32768) {
                size += 32768;
                buffer = realloc(buffer, size);
            }
            for (uint j = 0; j < size / 8; j++)
                buffer[n++] = *(bytes++);
            break;
        case 'v':
            if (n + 10 >= 32768) {
                size += 32768;
                buffer = realloc(buffer, size);
            }
            if (size == 32) {
                n += encode_varint(buffer + n, *(int*)bytes);
            } else if (size == 64) {
                n += encode_varlong(buffer + n, *(long*)bytes);
            }
            break;
        case 'b':
            if (n + 1 >= 32768) {
                size += 32768;
                buffer = realloc(buffer, size);
            }
            buffer[n++] = *(bytes++);
            break;
        case 's':
            string str = *(string*)bytes;
            bytes += sizeof(string);
            uint length = strlen(str);
            if (n + length + 10 >= 32768) {
                size += 32768;
                buffer = realloc(buffer, size);
            }
            n += encode_varint(buffer + n, length);
            for (uint j = 0; j < length; j++)
                buffer[n + j] = str[j];
            n += length;
            break;
        }

        i++;
        continue;
    }
    byte lengthBuffer[5];
    byte idBuffer[5];
    uint idBufferSize = encode_varint(idBuffer, id);
    uint lengthBufferSize = encode_varint(lengthBuffer, n + idBufferSize);

    send(connection->sock, lengthBuffer, lengthBufferSize, 0);
    send(connection->sock, idBuffer, idBufferSize, 0);
    send(connection->sock, buffer, n, 0);

    free(buffer);
}