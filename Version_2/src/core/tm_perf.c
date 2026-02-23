/**
 * @file tm_perf.c
 * @brief System performance monitoring -- business logic, no Raylib.
 *
 * Each metric has its own private update function (~12 lines each).
 * The orchestrator tm_perf_update() is flat and readable.
 */

#include <string.h>
#include <stdlib.h>

#include "../../include/tm_perf.h"
#include "../../include/tm_platform.h"
#include "../../include/tm_log.h"
#include "../../include/tm_app_history.h"

/* -------------------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------------- */

void tm_perf_data_init(TmPerfData *d) {
    if (!d) return;
    memset(d, 0, sizeof(*d));
    d->mem_total_kb  = (uint64_t)16 * 1024 * 1024; /* 16 GiB in KB */
    d->disk_total_kb = (uint64_t)500 * 1024 * 1024; /* 500 GiB in KB */
    d->last_update   = clock();
}

/* -------------------------------------------------------------------------
 * Per-metric updaters (each ~12 lines, independently testable)
 * ---------------------------------------------------------------------- */

static void update_cpu(TmPerfData *d) {
    d->cpu_percent = g_platform->sample_cpu();
    if (d->cpu_percent > 100.0f) d->cpu_percent = 100.0f;
    d->cpu_history[d->cpu_idx] = d->cpu_percent;
    d->cpu_idx = (d->cpu_idx + 1) % TM_HIST_LEN;
}

static void update_memory(TmPerfData *d) {
    g_platform->query_memory(&d->mem_used_kb, &d->mem_total_kb);
    if (d->mem_used_kb > d->mem_total_kb)
        d->mem_used_kb = d->mem_total_kb;
    d->mem_available_kb      = d->mem_total_kb - d->mem_used_kb;
    d->mem_history[d->mem_idx] = d->mem_used_kb;
    d->mem_idx = (d->mem_idx + 1) % TM_HIST_LEN;
}

static void update_disk(TmPerfData *d) {
    /* Demo: simulate disk usage fluctuation */
    d->disk_used_kb = 200000000ULL + (uint64_t)(rand() % 150000000);
    d->disk_history[d->disk_idx] = d->disk_used_kb;
    d->disk_idx = (d->disk_idx + 1) % TM_HIST_LEN;
}

static void update_gpu(TmPerfData *d) {
    d->gpu_percent = 8.0f + (float)(rand() % 50);
    if (d->gpu_percent > 100.0f) d->gpu_percent = 100.0f;
    d->gpu_history[d->gpu_idx] = d->gpu_percent;
    d->gpu_idx = (d->gpu_idx + 1) % TM_HIST_LEN;
}

static void update_threads(TmPerfData *d) {
    d->thread_count = d->process_count * 3 + (rand() % 100);
}

/* -------------------------------------------------------------------------
 * Delta / orchestrator
 * ---------------------------------------------------------------------- */

float tm_perf_delta_seconds(const TmPerfData *d) {
    if (!d) return 0.0f;
    return (float)(clock() - d->last_update) / (float)CLOCKS_PER_SEC;
}

tm_result_t tm_perf_update(TmAppState *s) {
    if (!s) return TM_ERR_INVALID_ARG;

    float delta = tm_perf_delta_seconds(&s->perf);
    if (delta < TM_PERF_UPDATE_INTERVAL_S) return TM_OK;

    s->perf.last_update    = clock();
    s->perf.process_count  = s->process_count;
    s->perf.uptime_s      += (uint32_t)delta;

    update_cpu(&s->perf);
    update_memory(&s->perf);
    update_disk(&s->perf);
    update_gpu(&s->perf);
    update_threads(&s->perf);

    TM_CHECK(tm_history_tick(s));
    return TM_OK;
}
