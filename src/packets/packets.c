#include <string.h>
#include <sys/socket.h>
#include <uuid/uuid.h>
#include <mstd/common.h>
#include <mstd/types/stack.h>
#include "src/logging.h"
#include "src/network.h"

#include "src/packets/handshake.h"
#include "src/packets/status.h"
#include "src/packets/login.h"

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

const packet_format PACKET_FORMATS_LOGIN[] = {
    {.id = PACKET_ID_SB_L_START, .size = sizeof(packet_sb_l_start), .handler = sb_l_start_handler, .format = "s16 u128"},
    {.id = PACKET_ID_SB_L_ACK, .size = sizeof(packet_sb_l_ack), .handler = sb_l_ack_handler, .format = ""},
    {.id = PACKET_ID_CB_L_SUCCESS, .size = sizeof(packet_cb_l_success), .handler = null, .format = "u128 s16 v32 [s64 s ?s1024]"},
};

const packet_format PACKET_FORMATS_CONFIG[] = {};

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

byte* decode_packet_format(byte* in, byte* out, string format, uint formatLength, stack* allocStack) {
    int previousValue = 0;
    bool nextPresent = true;
    for (uint i = 0; i < formatLength; i++) {
        if (format[i] == '?') {
            nextPresent = *(in++);
            *(out++) = nextPresent;
            continue;
        }
        if (format[i] == '[') {

            uint length = 0;
            uint size = 0;
            uint depth = 1;
            while (true) {
                length++;
                switch (format[i + length]) {
                case '[':
                    depth++;
                    break;
                case ']':
                    depth--;
                    if (depth == 0)
                        goto done;
                    break;
                case 's':
                    size += sizeof(void*);
                    break;
                case '?':
                case 'b':
                    size += 1;
                    break;
                case 'u':
                case 'i':
                case 'v':;
                    char* start = format + i + length;
                    char* end = start;
                    while (*end != ' ')
                        end++;
                    size += strtol(start, &end, 10) / 8;
                    break;
                }
            }
        done:
            if (previousValue == 0) {
                for (uint j = 0; j < sizeof(void*); i++)
                    *(out++) = 0;
            } else {
                byte* buffer = malloc(size * previousValue);
                stack_push(allocStack, buffer);
                for (uint j = 0; j < previousValue; j++) {
                    in = decode_packet_format(in, buffer + (j * size), format + i + 1, length, allocStack);
                    if (in == null)
                        return null;
                }
            }
            i += length + 1;
        }

        char type = format[i];
        uint size = 0;
        if (format[i++] > '0' && format[i] < '9') {
            char* start = format + i;
            char* end = start;
            while (*end != ' ')
                end++;
            size = strtol(start, &end, 10);
            i += end - start;
        }
        previousValue = 0;

        if (!nextPresent)
            continue;

        switch (type) {
        case 'u':
        case 'i':
            out += size / 8;
            for (uint j = 0; j < size / 8; j++)
                *(out - j - 1) = *(in++);
            previousValue = *((int*)out - size / 8);
            break;
        case 'b':
            *(out++) = *(in++);
            break;
        case 'v':
            if (size == 32) {
                int value;
                in += decode_varint(in, &value);
                previousValue = value;
                for (uint j = 0; j < sizeof(int); j++) {
                    *(out++) = ((byte*)&value)[j];
                }
            } else if (size == 128) {
                long value;
                in += decode_varlong(in, &value);
                for (uint j = 0; j < sizeof(long); j++)
                    *(out++) = ((byte*)&value)[j];
            }
            break;
        case 's':
            int length;
            in += decode_varint(in, &length);
            if ((size > 0 && length > size) || (size == 0 && length > 32767)) {
                log_error("cesium:network", "packet string exceeded max length");
                return null;
            }
            string buffer = malloc(length + 1);
            stack_push(allocStack, buffer);
            for (uint j = 0; j < length; j++)
                buffer[j] = *(in++);
            buffer[length] = '\0';
            for (uint j = 0; j < sizeof(string); j++)
                *(out++) = ((byte*)&buffer)[j];
            break;
        }
    }

    return in;
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
    case STATE_CONFIGURATION:
        search_sb(PACKET_FORMATS_CONFIG);
        log_warning("cesium:network", "unknown packet with id configuration:%i", id);
        return HANDLE_UNKNOWN;
    }

found:
    byte* packet = malloc(format.size);
    byte* packetWrite = packet;
    for (uint j = 0; j < sizeof(enum packet_id); j++)
        *(packetWrite++) = ((byte*)&id)[j];

    stack* allocStack = stack_new(1024);

    if (decode_packet_format(data, packetWrite, format.format, strlen(format.format), allocStack) != null) {
        if (format.handler(connection, packet))
            status = HANDLE_SUCCESS;
    }

    for (uint i = 0; i < allocStack->count; i++)
        free(stack_pop(allocStack));
    stack_cleanup(allocStack);
    free(packet);

    return status;
}

typedef struct {
    byte* data;
    uint size;
    uint n;
} packet_buffer;

packet_buffer encode_packet_format(byte* in, packet_buffer out, string format, uint formatLength) {
    int previousValue = 0;
    bool nextPresent = true;
    for (uint i = 0; i < formatLength; i++) {
        if (format[i] == '?') {
            nextPresent = *(in++);
            if (out.n + 1 > out.size) {
                out.size += 32768;
                out.data = realloc(out.data, out.size);
            }
            out.data[out.size] = nextPresent;
            out.n += 1;
            continue;
        }
        if (format[i] == '[') {
            uint length = 0;
            uint depth = 1;
            while (true) {
                length++;
                switch (format[i + length]) {
                case '[':
                    depth++;
                    break;
                case ']':
                    depth--;
                    if (depth == 0)
                        goto done;
                    break;
                }
            }
        done:
            if (previousValue != 0) {
                for (uint j = 0; j < previousValue; j++) {
                    out = encode_packet_format(in, out, format + i + 1, length);
                }
            }
            i += length + 1;
        }

        char type = format[i];
        uint size = 0;
        if (format[i++] > '0' && format[i] < '9') {
            char* start = format + i;
            char* end = start;
            while (*end != ' ')
                end++;
            size = strtol(start, &end, 10);
            i += end - start;
        }
        previousValue = 0;

        if (!nextPresent)
            continue;

        switch (type) {
        case 'u':
        case 'i':
            if (out.n + size / 8 > out.size) {
                out.size += 32768;
                out.data = realloc(out.data, out.size);
            }
            out.n += size / 8;
            for (uint j = 0; j < size / 8; j++)
                out.data[out.n - j - 1] = *(in++);
            previousValue = *((int*)out.data + out.n - size / 8);
            break;
        case 'b':
            if (out.n + 1 > out.size) {
                out.size += 32768;
                out.data = realloc(out.data, out.size);
            }
            *(out.data) = *(in++);
            break;
        case 'v':
            if (out.n + 10 >= out.size) {
                out.size += 32768;
                out.data = realloc(out.data, out.size);
            }
            if (size == 32) {
                previousValue = *(int*)in;
                out.n += encode_varint(out.data + out.n, *(int*)in);
            } else if (size == 64) {
                out.n += encode_varlong(out.data + out.n, *(long*)in);
            }
            break;
        case 's':
            string buffer = *(string*)in;
            in += sizeof(string);
            uint length = strlen(buffer);
            if (out.n + length + 10 >= out.size) {
                out.size += 32768;
                out.data = realloc(out.data, out.size);
            }
            out.n += encode_varint(out.data + out.n, length);
            for (uint j = 0; j < length; j++)
                out.data[out.n + j] = buffer[j];
            out.n += length;
            break;
        }
    }

    return out;
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
    case STATE_CONFIGURATION:
        search_cb(PACKET_FORMATS_CONFIG);
        return;
    }

found:;

    packet_buffer buffer = {.data = malloc(4096), .size = 4096, .n = 0};
    buffer = encode_packet_format(bytes, buffer, format.format, strlen(format.format));

    byte lengthBuffer[5];
    byte idBuffer[5];
    uint idBufferSize = encode_varint(idBuffer, id);
    uint lengthBufferSize = encode_varint(lengthBuffer, buffer.n + idBufferSize);

    send(connection->sock, lengthBuffer, lengthBufferSize, 0);
    send(connection->sock, idBuffer, idBufferSize, 0);
    send(connection->sock, buffer.data, buffer.n, 0);

    free(buffer.data);
}