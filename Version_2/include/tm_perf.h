/**
 * @file tm_perf.h
 * @brief Public API for system performance monitoring.
 *
 * Business logic only -- no Raylib symbols.
 */

#ifndef TM_PERF_H
#define TM_PERF_H

#include "tm_types.h"

/**
 * Zero-initialise performance data and set fixed totals.
 * Must be called once before the first tm_perf_update().
 * @param d  Performance data struct. Must not be NULL.
 */
void tm_perf_data_init(TmPerfData *d);

/**
 * Sample all metrics (CPU, memory, disk, GPU) if the update interval
 * has elapsed. Internally throttled to TM_PERF_UPDATE_INTERVAL_S.
 * @param s  Application state. Must not be NULL.
 * @return   TM_OK or TM_ERR_INVALID_ARG.
 */
tm_result_t tm_perf_update(TmAppState *s);

/**
 * Return elapsed seconds since the last performance update.
 * @param d  Performance data struct. Must not be NULL.
 */
float tm_perf_delta_seconds(const TmPerfData *d);

#endif /* TM_PERF_H */
