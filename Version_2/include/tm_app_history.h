/**
 * @file tm_app_history.h
 * @brief Public API for the application history subsystem.
 */

#ifndef TM_APP_HISTORY_H
#define TM_APP_HISTORY_H

#include "tm_types.h"

/** Free all history list nodes. */
void tm_history_list_free(TmAppState *s);

/**
 * Allocate and populate the history list with initial demo data.
 * @return TM_OK or TM_ERR_ALLOC.
 */
tm_result_t tm_history_init(TmAppState *s);

/**
 * Update history ring buffers for entries whose interval has elapsed.
 * Call once per performance update cycle.
 * @return TM_OK or TM_ERR_INVALID_ARG.
 */
tm_result_t tm_history_tick(TmAppState *s);

#endif /* TM_APP_HISTORY_H */
