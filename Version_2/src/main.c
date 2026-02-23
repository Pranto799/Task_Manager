/**
 * @file main.c
 * @brief Application entry point -- wires platform, inits subsystems, runs loop.
 *
 * This file contains ~30 lines of executable code.  All logic lives in
 * src/core/, src/ui/, src/platform/, and src/utils/.
 */

#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "../include/tm_types.h"
#include "../include/tm_platform.h"
#include "../include/tm_process.h"
#include "../include/tm_perf.h"
#include "../include/tm_app_history.h"
#include "../include/tm_startup.h"
#include "../include/tm_ui.h"
#include "../include/tm_log.h"

/* Defined in ui/ui_core.c -- forward-declare for this translation unit */
/* Platform pointer definition (declared extern in tm_platform.h) */
const TmPlatform *g_platform = NULL;

/* Observer: fired after each process list refresh */
static void on_process_changed(const TmAppState *s, void *user) {
    (void)user;
    /* Recompute scrollbar sizes after list changes */
    ui_layout_update((TmAppState *)s);
}

static void app_init(TmAppState *s) {
    tm_perf_data_init(&s->perf);
    s->screen_w = 1200;
    s->screen_h = 800;

    ui_init(s);

    if (tm_startup_list_load(s) != TM_OK)
        tm_log_warn("Startup list load failed");

    if (tm_history_init(s) != TM_OK)
        tm_log_warn("History init failed");

    tm_process_observer_add(on_process_changed, NULL);

    if (tm_process_list_refresh(s) != TM_OK)
        tm_log_warn("Initial process list refresh failed");
}

static void app_update(TmAppState *s) {
    ui_window_resize_handle(s);
    ui_input_update(s);
    tm_perf_update(s);
    ui_toast_tick(s);
}

static void app_draw(TmAppState *s) {
    BeginDrawing();
    ClearBackground(TM_COLOR_BG);

    ui_titlebar_draw(s);
    ui_tabs_draw(s);
    ui_buttons_draw(s);
    ui_toast_draw(s);
    ui_content_draw(s);
    ui_resize_handle_draw(s);
    ui_statusbar_draw(s);

    EndDrawing();
}

static void app_cleanup(TmAppState *s) {
    tm_process_list_free(s);
    tm_startup_list_free(s);
    tm_history_list_free(s);
}

int main(void) {
    srand((unsigned int)time(NULL));

    /* Select platform adapter -- the only #ifdef in the project */
#ifdef _WIN32
    g_platform = &k_platform_win32;
#else
    g_platform = &k_platform_posix;
#endif

    InitWindow(1200, 800, "Advanced Task Manager");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    TmAppState app = {0};
    app_init(&app);

    while (!WindowShouldClose()) {
        app_update(&app);
        app_draw(&app);
    }

    app_cleanup(&app);
    CloseWindow();
    return 0;
}
