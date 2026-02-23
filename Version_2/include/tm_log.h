/**
 * @file tm_log.h
 * @brief Simple timestamped logger with DEBUG/INFO/WARN/ERROR levels.
 */

#ifndef TM_LOG_H
#define TM_LOG_H

typedef enum {
    TM_LOG_DEBUG = 0,
    TM_LOG_INFO,
    TM_LOG_WARN,
    TM_LOG_ERROR,
} TmLogLevel;

/**
 * Emit a log entry.  Thread-unsafe; call from main thread only.
 * @param level   Severity level.
 * @param fmt     printf-style format string.
 * @param ...     Format arguments.
 */
void tm_log(TmLogLevel level, const char *fmt, ...);

/* Convenience macros */
#define tm_log_debug(...)  tm_log(TM_LOG_DEBUG, __VA_ARGS__)
#define tm_log_info(...)   tm_log(TM_LOG_INFO,  __VA_ARGS__)
#define tm_log_warn(...)   tm_log(TM_LOG_WARN,  __VA_ARGS__)
#define tm_log_error(...)  tm_log(TM_LOG_ERROR, __VA_ARGS__)

#endif /* TM_LOG_H */
