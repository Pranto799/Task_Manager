#include "../../include/tm_app_history.h"
/**
 * @file tm_app_history.c
 * @brief Application history subsystem -- business logic, no Raylib.
 */

#include <stdlib.h>
#include <string.h>

#include "../../include/tm_types.h"
#include "../../include/tm_log.h"

/* -------------------------------------------------------------------------
 * Static demo app names
 * ---------------------------------------------------------------------- */

static const char *k_history_apps[] = {
    "chrome.exe", "Code.exe", "explorer.exe", "Spotify.exe",
    "Discord.exe", "steam.exe", "msedge.exe",  "devenv.exe"
};

/* -------------------------------------------------------------------------
 * List management
 * ---------------------------------------------------------------------- */

void tm_history_list_free(TmAppState *s) {
    if (!s) return;
    TmAppHistory *cur = s->history_list;
    while (cur) {
        TmAppHistory *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    s->history_list = NULL;
}

static void init_history_entry(TmAppHistory *app, const char *name) {
    strncpy(app->name, name, TM_NAME_MAX - 1);
    app->name[TM_NAME_MAX - 1] = '\0';

    app->cpu_time    = 5.0f + (float)(rand() % 50);
    app->memory_kb   = 100 + (uint64_t)(rand() % 500);
    app->network_kb  = 10  + (uint64_t)(rand() % 100);
    app->history_idx = 0;
    app->last_update = clock();
    app->next        = NULL;

    for (int j = 0; j < TM_HIST_SHORT; j++) {
        float factor = 0.8f + (float)(rand() % 40) / 100.0f;
        app->cpu_time_history[j] = app->cpu_time * factor;
        app->memory_history[j]   = (uint64_t)((float)app->memory_kb * factor);
        app->network_history[j]  = (uint64_t)((float)app->network_kb * factor);
    }
}

tm_result_t tm_history_init(TmAppState *s) {
    if (!s) return TM_ERR_INVALID_ARG;

    tm_history_list_free(s);

    int count = (int)(sizeof(k_history_apps) / sizeof(k_history_apps[0]));
    for (int i = 0; i < count; i++) {
        TmAppHistory *app = (TmAppHistory *)malloc(sizeof(TmAppHistory));
        if (!app) return TM_ERR_ALLOC;

        init_history_entry(app, k_history_apps[i]);
        app->next      = s->history_list;
        s->history_list = app;
    }

    tm_log_info("App history initialised: %d entries", count);
    return TM_OK;
}

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

static void update_history_entry(TmAppHistory *app) {
    float factor     = 0.9f + (float)(rand() % 20) / 100.0f;
    app->cpu_time   *= factor;
    app->memory_kb   = (uint64_t)((float)app->memory_kb  * factor);
    app->network_kb  = (uint64_t)((float)app->network_kb
                       * (0.8f + (float)(rand() % 40) / 100.0f));

    app->cpu_time_history[app->history_idx] = app->cpu_time;
    app->memory_history[app->history_idx]   = app->memory_kb;
    app->network_history[app->history_idx]  = app->network_kb;
    app->history_idx = (app->history_idx + 1) % TM_HIST_SHORT;
    app->last_update = clock();
}

tm_result_t tm_history_tick(TmAppState *s) {
    if (!s) return TM_ERR_INVALID_ARG;

    TmAppHistory *cur = s->history_list;
    while (cur) {
        float delta = (float)(clock() - cur->last_update) / (float)CLOCKS_PER_SEC;
        if (delta >= TM_HISTORY_UPDATE_INTERVAL_S) {
            update_history_entry(cur);
        }
        cur = cur->next;
    }
    return TM_OK;
}
