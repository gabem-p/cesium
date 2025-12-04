#include <stdarg.h>
#include <mstd/common.h>

#include "logging.h"

#define COLOR_SOURCE "\033[37m"
#define COLOR_INFO "\033[94m"
#define COLOR_ERROR "\033[91m"
#define COLOR_WARN "\033[33m"
#define COLOR_RESET "\033[0m"

void log_info(string source, string message, ...) {
    va_list args;
    va_start(args, message);

    printf("[%sINFO%s] [%s%s%s] ",
           COLOR_INFO, COLOR_RESET, COLOR_SOURCE, source, COLOR_RESET);
    vprintf(message, args);
    printf("\n");

    va_end(args);
}

void log_warning(string source, string message, ...) {
    va_list args;
    va_start(args, message);

    printf("[%sWARN%s] [%s%s%s] %s",
           COLOR_WARN, COLOR_RESET, COLOR_SOURCE, source, COLOR_RESET, COLOR_WARN);
    vprintf(message, args);
    printf("%s\n", COLOR_RESET);

    va_end(args);
}

void log_error(string source, string message, ...) {
    va_list args;
    va_start(args, message);

    printf("[%sERRR%s] [%s%s%s] %s",
           COLOR_ERROR, COLOR_RESET, COLOR_SOURCE, source, COLOR_RESET, COLOR_ERROR);
    vprintf(message, args);
    printf("%s\n", COLOR_RESET);

    va_end(args);
}