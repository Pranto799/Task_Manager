/**
 * @file platform_win32.c
 * @brief Windows (Win32) platform adapter implementation.
 *
 * This is the ONLY file in the project that uses Windows-specific APIs.
 * No other file contains #ifdef _WIN32 blocks.
 */

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/tm_platform.h"
#include "../../include/tm_log.h"

/* -------------------------------------------------------------------------
 * Process list
 * ---------------------------------------------------------------------- */

static FILE *win32_open_process_list(void) {
    return popen("tasklist /fo csv /nh", "r");
}

static bool win32_parse_process_line(FILE *fp, TmProcess *out) {
    if (!fp || !out) return false;

    char line[1024];
    if (!fgets(line, sizeof(line), fp)) return false;

    /* CSV format: "name.exe","pid","session","session#","mem K" */
    char *first_q = strchr(line, '"');
    if (!first_q) return false;
    char *second_q = strchr(first_q + 1, '"');
    if (!second_q) return false;

    size_t name_len = (size_t)(second_q - first_q - 1);
    if (name_len >= TM_NAME_MAX) name_len = TM_NAME_MAX - 1;
    strncpy(out->name, first_q + 1, name_len);
    out->name[name_len] = '\0';

    /* Extract PID from third field */
    char *pid_start = strchr(second_q + 1, '"');
    if (pid_start) pid_start++;
    char *pid_end = pid_start ? strchr(pid_start, '"') : NULL;
    out->pid = (pid_start && pid_end) ? (uint32_t)strtoul(pid_start, NULL, 10) : 0;

    /* Simulated metrics */
    out->memory_bytes = (uint64_t)(1000 + rand() % 10000) * 1024;
    out->cpu_percent  = (float)(rand() % 1000) / 10.0f;
    out->is_selected  = false;
    out->next         = NULL;
    return true;
}

static tm_result_t win32_kill_process(uint32_t pid) {
    char cmd[TM_CMD_MAX];
    snprintf(cmd, sizeof(cmd), "taskkill /PID %lu /F", (unsigned long)pid);
    int r = system(cmd);
    if (r == 0) return TM_OK;
    tm_log_error("taskkill /PID %lu /F failed (exit %d)", (unsigned long)pid, r);
    return TM_ERR_PLATFORM;
}

static float win32_sample_cpu(void) {
    /* Demo: replace with PdhCollectQueryData / GetSystemTimes. */
    return 5.0f + (float)(rand() % 60);
}

static void win32_query_memory(uint64_t *used_kb, uint64_t *total_kb) {
    /* Demo values; replace with GlobalMemoryStatusEx(). */
    *total_kb = (uint64_t)16 * 1024 * 1024;
    *used_kb  = (uint64_t)(4000 + rand() % 4000) * 1024;
}

/* -------------------------------------------------------------------------
 * Exported adapter
 * ---------------------------------------------------------------------- */

const TmPlatform k_platform_win32 = {
    .open_process_list  = win32_open_process_list,
    .parse_process_line = win32_parse_process_line,
    .kill_process       = win32_kill_process,
    .sample_cpu         = win32_sample_cpu,
    .query_memory       = win32_query_memory,
};

#endif /* _WIN32 */
