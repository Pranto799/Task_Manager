/**
 * @file tm_process.h
 * @brief Public API for the process list subsystem.
 *
 * Business logic only -- no Raylib symbols.
 */

#ifndef TM_PROCESS_H
#define TM_PROCESS_H

#include "tm_types.h"

/**
 * Free all nodes in the process list and reset process_count.
 * @param s  Application state. Must not be NULL.
 */
void tm_process_list_free(TmAppState *s);

/**
 * Refresh the process list from the OS and notify observers.
 * @param s  Application state. Must not be NULL.
 * @return   TM_OK, TM_ERR_IO, TM_ERR_ALLOC, or TM_ERR_INVALID_ARG.
 */
tm_result_t tm_process_list_refresh(TmAppState *s);

/**
 * Terminate the process with the given PID.
 * @param pid  Process ID to kill.
 * @return     TM_OK or TM_ERR_PLATFORM.
 */
tm_result_t tm_process_kill(uint32_t pid);

/**
 * Return a pointer to the currently selected process, or NULL.
 * @param s  Application state. Must not be NULL.
 */
TmProcess *tm_process_get_selected(const TmAppState *s);

/**
 * Register an observer callback fired after each list refresh.
 * @param fn    Callback; must not be NULL.
 * @param user  Opaque user data passed to the callback.
 * @return      TM_OK or TM_ERR_INVALID_ARG (too many observers or NULL fn).
 */
tm_result_t tm_process_observer_add(TmProcessChangedFn fn, void *user);

#endif /* TM_PROCESS_H */
