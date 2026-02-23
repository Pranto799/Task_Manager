/**
 * @file tm_startup.c
 * @brief Startup applications subsystem -- business logic, no Raylib.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../include/tm_startup.h"
#include "../../include/tm_log.h"

/* -------------------------------------------------------------------------
 * Static demo data
 * ---------------------------------------------------------------------- */

static const char *k_app_names[] = {
    "Microsoft OneDrive", "Spotify",       "Discord",        "Steam Client",
    "Adobe Creative Cloud", "NVIDIA Display", "Realtek Audio", "Microsoft Teams"
};

static const char *k_publishers[] = {
    "Microsoft Corporation", "Spotify AB",          "Discord Inc.",       "Valve Corporation",
    "Adobe Inc.",            "NVIDIA Corporation",  "Realtek Semiconductor", "Microsoft Corporation"
};

static const float k_impacts[] = { 2.1f, 1.5f, 3.2f, 4.5f, 2.8f, 0.5f, 0.3f, 3.8f };
static const bool  k_enabled[] = { true, true, false, true, true, true, true, false };

/* -------------------------------------------------------------------------
 * List management
 * ---------------------------------------------------------------------- */

void tm_startup_list_free(TmAppState *s) {
    if (!s) return;
    TmStartupApp *cur = s->startup_list;
    while (cur) {
        TmStartupApp *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    s->startup_list = NULL;
}

tm_result_t tm_startup_list_load(TmAppState *s) {
    if (!s) return TM_ERR_INVALID_ARG;

    tm_startup_list_free(s);

    int count = (int)(sizeof(k_app_names) / sizeof(k_app_names[0]));
    for (int i = 0; i < count; i++) {
        TmStartupApp *app = (TmStartupApp *)malloc(sizeof(TmStartupApp));
        if (!app) return TM_ERR_ALLOC;

        strncpy(app->name,      k_app_names[i], TM_NAME_MAX - 1);
        strncpy(app->publisher, k_publishers[i], TM_PUBLISHER_MAX - 1);
        app->name[TM_NAME_MAX - 1]          = '\0';
        app->publisher[TM_PUBLISHER_MAX - 1] = '\0';

        app->is_enabled = k_enabled[i];
        app->impact_s   = k_impacts[i];
        strncpy(app->status, app->is_enabled ? "Enabled" : "Disabled",
                TM_STATUS_MAX - 1);
        app->status[TM_STATUS_MAX - 1] = '\0';

        app->next          = s->startup_list;
        s->startup_list    = app;
    }

    tm_log_info("Startup list loaded: %d entries", count);
    return TM_OK;
}

/* -------------------------------------------------------------------------
 * Operations
 * ---------------------------------------------------------------------- */

TmStartupApp *tm_startup_get(const TmAppState *s, int index) {
    if (!s || index < 0) return NULL;
    TmStartupApp *cur = s->startup_list;
    int           i   = 0;
    while (cur && i < index) {
        cur = cur->next;
        i++;
    }
    return cur;
}

tm_result_t tm_startup_toggle(TmAppState *s, int index) {
    if (!s) return TM_ERR_INVALID_ARG;

    TmStartupApp *app = tm_startup_get(s, index);
    if (!app) return TM_ERR_INVALID_ARG;

    app->is_enabled = !app->is_enabled;
    strncpy(app->status, app->is_enabled ? "Enabled" : "Disabled",
            TM_STATUS_MAX - 1);
    app->status[TM_STATUS_MAX - 1] = '\0';

    tm_log_info("Startup app '%s' %s", app->name,
                app->is_enabled ? "enabled" : "disabled");
    return TM_OK;
}
