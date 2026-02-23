/**
 * @file platform_posix.c
 * @brief POSIX (Linux / macOS) platform adapter implementation.
 *
 * This is the ONLY file in the project that may use POSIX-specific
 * system calls. All #ifdef _WIN32 blocks are absent from every other file.
 */

#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../include/tm_platform.h"
#include "../../include/tm_log.h"

/* -------------------------------------------------------------------------
 * Process list
 * ---------------------------------------------------------------------- */

static FILE *posix_open_process_list(void) {
    return popen("ps -eo pid,comm --no-headers", "r");
}

static bool posix_parse_process_line(FILE *fp, TmProcess *out) {
    if (!fp || !out) return false;
    unsigned int pid = 0;
    char name[TM_NAME_MAX] = {0};
    int n = fscanf(fp, "%u %255s", &pid, name);
    if (n != 2) return false;
    out->pid = (uint32_t)pid;
    strncpy(out->name, name, TM_NAME_MAX - 1);
    out->name[TM_NAME_MAX - 1] = '\0';
    /* Simulated metrics -- replace with /proc parsing for real data */
    out->memory_bytes = (uint64_t)(1000 + rand() % 10000) * 1024;
    out->cpu_percent  = (float)(rand() % 1000) / 10.0f;
    out->is_selected  = false;
    out->next         = NULL;
    return true;
}

static tm_result_t posix_kill_process(uint32_t pid) {
    if (kill((pid_t)pid, SIGKILL) == 0) return TM_OK;
    tm_log_error("kill(%u, SIGKILL) failed: %s", pid, strerror(errno));
    return TM_ERR_PLATFORM;
}

static float posix_sample_cpu(void) {
    /* Demo: return a plausible random value.
     * Real impl: parse /proc/stat deltas between calls. */
    return 5.0f + (float)(rand() % 60);
}

static void posix_query_memory(uint64_t *used_kb, uint64_t *total_kb) {
    /* Demo values; replace with sysinfo() or /proc/meminfo parsing. */
    *total_kb = (uint64_t)16 * 1024 * 1024;
    *used_kb  = (uint64_t)(4000 + rand() % 4000) * 1024;
}

/* -------------------------------------------------------------------------
 * Exported adapter
 * ---------------------------------------------------------------------- */

const TmPlatform k_platform_posix = {
    .open_process_list  = posix_open_process_list,
    .parse_process_line = posix_parse_process_line,
    .kill_process       = posix_kill_process,
    .sample_cpu         = posix_sample_cpu,
    .query_memory       = posix_query_memory,
};
