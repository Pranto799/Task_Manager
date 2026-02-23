/**
 * @file tm_platform.h
 * @brief Platform adapter vtable -- abstracts all OS-specific calls.
 *
 * Business logic in core/ and ui/ calls exclusively through this interface.
 * The concrete implementations live in src/platform/platform_posix.c and
 * src/platform/platform_win32.c. No #ifdef blocks appear outside platform/.
 */

#ifndef TM_PLATFORM_H
#define TM_PLATFORM_H

#include <stdio.h>
#include "tm_types.h"

/**
 * OS-abstraction vtable.  One instance is selected at startup in main.c
 * and exposed via g_platform.
 */
typedef struct {
    /** Open the OS process list and return a FILE* for line-by-line reading. */
    FILE *(*open_process_list)(void);

    /**
     * Parse the next line from @p fp into @p out.
     * @return true if a valid entry was read; false at EOF or on error.
     */
    bool (*parse_process_line)(FILE *fp, TmProcess *out);

    /** Terminate process @p pid. Returns TM_OK or TM_ERR_PLATFORM. */
    tm_result_t (*kill_process)(uint32_t pid);

    /** Sample current overall CPU usage, 0–100. */
    float (*sample_cpu)(void);

    /** Fill @p used_kb and @p total_kb with current physical memory figures. */
    void (*query_memory)(uint64_t *used_kb, uint64_t *total_kb);
} TmPlatform;

/** Pointer set once in main() before any other call. Never NULL at runtime. */
extern const TmPlatform *g_platform;

/* Declarations for the two concrete adapters */
extern const TmPlatform k_platform_posix;
extern const TmPlatform k_platform_win32;

#endif /* TM_PLATFORM_H */
