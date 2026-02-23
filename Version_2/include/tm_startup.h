/**
 * @file tm_startup.h
 * @brief Public API for the startup applications subsystem.
 *
 * Business logic only -- no Raylib symbols.
 */

#ifndef TM_STARTUP_H
#define TM_STARTUP_H

#include "tm_types.h"

/**
 * Free all nodes in the startup list.
 * @param s  Application state. Must not be NULL.
 */
void tm_startup_list_free(TmAppState *s);

/**
 * Populate the startup list with system data (or demo data on unsupported OS).
 * @param s  Application state. Must not be NULL.
 * @return   TM_OK or TM_ERR_ALLOC.
 */
tm_result_t tm_startup_list_load(TmAppState *s);

/**
 * Toggle the enabled state of the startup app at position @p index.
 * @param s      Application state. Must not be NULL.
 * @param index  Zero-based index into the startup list.
 * @return       TM_OK or TM_ERR_INVALID_ARG.
 */
tm_result_t tm_startup_toggle(TmAppState *s, int index);

/**
 * Return the startup app at @p index, or NULL if out of range.
 * @param s      Application state. Must not be NULL.
 * @param index  Zero-based index.
 */
TmStartupApp *tm_startup_get(const TmAppState *s, int index);

#endif /* TM_STARTUP_H */
