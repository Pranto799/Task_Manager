/**
 * @file tm_process.c
 * @brief Process list management -- business logic, no Raylib.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

#include "../../include/tm_process.h"
#include "../../include/tm_platform.h"
#include "../../include/tm_log.h"

/* -------------------------------------------------------------------------
 * Observer registry (module-private)
 * ---------------------------------------------------------------------- */

static TmObserver s_observers[TM_MAX_OBSERVERS];
static int        s_observer_count = 0;

static void notify_observers(const TmAppState *s) {
    for (int i = 0; i < s_observer_count; i++) {
        if (s_observers[i].on_process_changed) {
            s_observers[i].on_process_changed(s, s_observers[i].user_data);
        }
    }
}

tm_result_t tm_process_observer_add(TmProcessChangedFn fn, void *user) {
    if (!fn || s_observer_count >= TM_MAX_OBSERVERS)
        return TM_ERR_INVALID_ARG;
    s_observers[s_observer_count++] = (TmObserver){ fn, user };
    return TM_OK;
}

/* -------------------------------------------------------------------------
 * List management
 * ---------------------------------------------------------------------- */

void tm_process_list_free(TmAppState *s) {
    if (!s) return;
    TmProcess *cur = s->process_list;
    while (cur) {
        TmProcess *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    s->process_list  = NULL;
    s->process_count = 0;
}

/* Allocate and prepend a copy of @p entry to the process list. */
static tm_result_t prepend_process(TmAppState *s, const TmProcess *entry) {
    TmProcess *node = (TmProcess *)malloc(sizeof(TmProcess));
    if (!node) return TM_ERR_ALLOC;
    *node         = *entry;
    node->next    = s->process_list;
    s->process_list = node;
    s->process_count++;
    return TM_OK;
}

tm_result_t tm_process_list_refresh(TmAppState *s) {
    if (!s) return TM_ERR_INVALID_ARG;

    tm_process_list_free(s);

    FILE *fp = g_platform->open_process_list();
    if (!fp) {
        tm_log_error("open_process_list() failed");
        return TM_ERR_IO;
    }

    TmProcess entry = {0};
    while (g_platform->parse_process_line(fp, &entry)) {
        tm_result_t r = prepend_process(s, &entry);
        if (r != TM_OK) {
            pclose(fp);
            return r;
        }
    }
    pclose(fp);

    /* Reset selection after refresh */
    s->selected_process_idx  = -1;
    s->end_task_btn.is_enabled = false;

    notify_observers(s);
    tm_log_info("Process list refreshed: %d entries", s->process_count);
    return TM_OK;
}

/* -------------------------------------------------------------------------
 * Process operations
 * ---------------------------------------------------------------------- */

tm_result_t tm_process_kill(uint32_t pid) {
    return g_platform->kill_process(pid);
}

TmProcess *tm_process_get_selected(const TmAppState *s) {
    if (!s || s->selected_process_idx < 0) return NULL;

    TmProcess *cur = s->process_list;
    int        idx = 0;
    while (cur) {
        if (idx == s->selected_process_idx) return cur;
        cur = cur->next;
        idx++;
    }
    return NULL;
}
