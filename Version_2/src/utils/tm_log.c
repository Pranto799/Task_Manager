/**
 * @file tm_log.c
 * @brief Timestamped logger implementation.
 */

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../../include/tm_log.h"

static const char *level_label(TmLogLevel level) {
    switch (level) {
        case TM_LOG_DEBUG: return "DEBUG";
        case TM_LOG_INFO:  return "INFO ";
        case TM_LOG_WARN:  return "WARN ";
        case TM_LOG_ERROR: return "ERROR";
        default:           return "?????";
    }
}

void tm_log(TmLogLevel level, const char *fmt, ...) {
    if (!fmt) return;

    time_t now   = time(NULL);
    struct tm *t = localtime(&now);
    char ts[20];
    strftime(ts, sizeof(ts), "%H:%M:%S", t);

    FILE *sink = (level >= TM_LOG_WARN) ? stderr : stdout;
    fprintf(sink, "[%s][%s] ", ts, level_label(level));

    va_list args;
    va_start(args, fmt);
    vfprintf(sink, fmt, args);
    va_end(args);

    fprintf(sink, "\n");
}
