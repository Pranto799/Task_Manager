/**
 * @file ui_core.c
 * @brief UI orchestration: initialisation, layout, input dispatch, drawing.
 *
 * Implements the Strategy pattern for tab dispatch (TmTabDescriptor table)
 * and the Command pattern for button actions.
 */

#include <string.h>
#include <stdio.h>

#include "../../include/tm_ui.h"
#include "../../include/tm_platform.h"
#include "../../include/tm_process.h"
#include "../../include/tm_startup.h"
#include "../../include/tm_perf.h"
#include "../../include/tm_log.h"

/* forward declaration of internal helper used before definition */
static void update_scrollbar_content(TmAppState *s);

/* -------------------------------------------------------------------------
 * Strategy table -- tab renderers
 * ---------------------------------------------------------------------- */

static const TmTabDescriptor k_tabs[TM_TAB_COUNT] = {
    { TM_TAB_PROCESSES,   "Processes",   ui_tab_process_draw  },
    { TM_TAB_PERFORMANCE, "Performance", ui_tab_perf_draw     },
    { TM_TAB_APP_HISTORY, "App History", ui_tab_history_draw  },
    { TM_TAB_STARTUP,     "Startup",     ui_tab_startup_draw  },
};

/* -------------------------------------------------------------------------
 * Command functions (Command Pattern)
 * ---------------------------------------------------------------------- */

static tm_result_t cmd_refresh(TmAppState *s, void *param) {
    (void)param;
    TM_CHECK(tm_process_list_refresh(s));
    TM_CHECK(tm_perf_update(s));
    ui_toast_show(s, "Refreshed", GREEN, TM_MSG_SHORT_FRAMES);
    return TM_OK;
}

static tm_result_t cmd_end_task(TmAppState *s, void *param) {
    (void)param;
    TmProcess *sel = tm_process_get_selected(s);
    if (!sel) return TM_ERR_INVALID_ARG;

    tm_result_t r = tm_process_kill(sel->pid);
    if (r == TM_OK) {
        ui_toast_show(s, "Process terminated", GREEN, TM_MSG_DISPLAY_FRAMES);
        tm_process_list_refresh(s);
    } else {
        ui_toast_show(s, "Failed to terminate process", RED, TM_MSG_DISPLAY_FRAMES);
    }
    return r;
}

static tm_result_t cmd_enable_startup(TmAppState *s, void *param) {
    (void)param;
    if (s->selected_startup_idx < 0) return TM_ERR_INVALID_ARG;
    TM_CHECK(tm_startup_toggle(s, s->selected_startup_idx));
    TmStartupApp *app = tm_startup_get(s, s->selected_startup_idx);
    if (app) {
        char msg[TM_MSG_MAX];
        /* Reserve space for " enabled" (8 chars) + null */
        strncpy(msg, app->name, sizeof(msg) - 9); msg[sizeof(msg) - 9] = '\0';
        strncat(msg, " enabled", sizeof(msg) - strlen(msg) - 1);
        ui_toast_show(s, msg, GREEN, TM_MSG_DISPLAY_FRAMES);
        s->enable_startup_btn.is_enabled  = false;
        s->disable_startup_btn.is_enabled = true;
    }
    return TM_OK;
}

static tm_result_t cmd_disable_startup(TmAppState *s, void *param) {
    (void)param;
    if (s->selected_startup_idx < 0) return TM_ERR_INVALID_ARG;
    TM_CHECK(tm_startup_toggle(s, s->selected_startup_idx));
    TmStartupApp *app = tm_startup_get(s, s->selected_startup_idx);
    if (app) {
        char msg[TM_MSG_MAX];
        /* Reserve space for " disabled" (9 chars) + null */
        strncpy(msg, app->name, sizeof(msg) - 10); msg[sizeof(msg) - 10] = '\0';
        strncat(msg, " disabled", sizeof(msg) - strlen(msg) - 1);
        ui_toast_show(s, msg, ORANGE, TM_MSG_DISPLAY_FRAMES);
        s->enable_startup_btn.is_enabled  = true;
        s->disable_startup_btn.is_enabled = false;
    }
    return TM_OK;
}

/* -------------------------------------------------------------------------
 * Scrollbar layout
 * ---------------------------------------------------------------------- */

static void layout_scrollbars(TmAppState *s) {
    int x = s->screen_w - TM_SCROLLBAR_WIDTH_PX - 3;

    s->process_scroll.bounds = (Rectangle){
        (float)x, 120.0f,
        TM_SCROLLBAR_WIDTH_PX, (float)(s->screen_h - 200) };
    s->process_scroll.visible_height = s->screen_h - 200;

    s->startup_scroll.bounds = (Rectangle){
        (float)x, 220.0f,
        TM_SCROLLBAR_WIDTH_PX, (float)(s->screen_h - 300) };
    s->startup_scroll.visible_height = s->screen_h - 300;

    s->history_scroll.bounds = (Rectangle){
        (float)x, 230.0f,
        TM_SCROLLBAR_WIDTH_PX, (float)(s->screen_h - 310) };
    s->history_scroll.visible_height = s->screen_h - 310;
}

static void layout_buttons(TmAppState *s) {
    s->refresh_btn.bounds = (Rectangle){
        (float)(s->screen_w - 180), 10.0f, 120.0f, TM_BUTTON_HEIGHT_PX };
    s->end_task_btn.bounds = (Rectangle){
        (float)(s->screen_w - 310), 10.0f, 120.0f, TM_BUTTON_HEIGHT_PX };
    s->enable_startup_btn.bounds = (Rectangle){
        (float)(s->screen_w - 320), 10.0f, 140.0f, TM_BUTTON_HEIGHT_PX };
    s->disable_startup_btn.bounds = (Rectangle){
        (float)(s->screen_w - 470), 10.0f, 140.0f, TM_BUTTON_HEIGHT_PX };
}

static void layout_tabs(TmAppState *s) {
    int tab_x = 10;
    int tab_w[] = { 100, 100, 100, 80 };
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        s->tabs[i].bounds = (Rectangle){
            (float)tab_x, 50.0f, (float)tab_w[i], TM_TAB_HEIGHT_PX };
        tab_x += tab_w[i] + 5;
    }
}

/* -------------------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------------- */

static void init_tabs(TmAppState *s) {
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        strncpy(s->tabs[i].text, k_tabs[i].label, sizeof(s->tabs[i].text) - 1);
        s->tabs[i].text[sizeof(s->tabs[i].text) - 1] = '\0';
        s->tabs[i].is_active  = (i == TM_TAB_PROCESSES);
        s->tabs[i].is_hovered = false;
    }
}

static void init_buttons(TmAppState *s) {
    s->refresh_btn = (TmButton){
        {0}, "Refresh", false,
        { 60,  60,  80, 255 }, { 80,  80, 100, 255 }, true
    };
    s->end_task_btn = (TmButton){
        {0}, "End Task", false,
        { 200, 60, 60, 255 }, { 220, 80, 80, 255 }, false
    };
    s->enable_startup_btn = (TmButton){
        {0}, "Enable Startup", false,
        { 60, 160, 60, 255 }, { 80, 180, 80, 255 }, false
    };
    s->disable_startup_btn = (TmButton){
        {0}, "Disable Startup", false,
        { 200, 60, 60, 255 }, { 220, 80, 80, 255 }, false
    };
}

void ui_init(TmAppState *s) {
    if (!s) return;
    init_tabs(s);
    init_buttons(s);
    s->selected_process_idx = -1;
    s->selected_startup_idx = -1;
    s->active_tab           = TM_TAB_PROCESSES;
    ui_layout_update(s);
}

void ui_layout_update(TmAppState *s) {
    if (!s) return;
    layout_tabs(s);
    layout_buttons(s);
    layout_scrollbars(s);
    update_scrollbar_content(s);
}

/* -------------------------------------------------------------------------
 * Scrollbar content size updates
 * ---------------------------------------------------------------------- */

static void update_scrollbar_content(TmAppState *s) {
    /* Process */
    s->process_scroll.content_height = s->process_count * TM_ROW_HEIGHT_PX;
    int excess = s->process_scroll.content_height - s->process_scroll.visible_height;
    s->process_scroll.max_scroll = (excess > 0) ? excess : 0;

    /* Startup */
    int startup_cnt = 0;
    for (TmStartupApp *a = s->startup_list; a; a = a->next) startup_cnt++;
    s->startup_scroll.content_height = startup_cnt * TM_STARTUP_ROW_PX;
    excess = s->startup_scroll.content_height - s->startup_scroll.visible_height;
    s->startup_scroll.max_scroll = (excess > 0) ? excess : 0;

    /* History */
    int hist_cnt = 0;
    for (TmAppHistory *a = s->history_list; a; a = a->next) hist_cnt++;
    s->history_scroll.content_height = hist_cnt * TM_HISTORY_ROW_PX;
    excess = s->history_scroll.content_height - s->history_scroll.visible_height;
    s->history_scroll.max_scroll = (excess > 0) ? excess : 0;
}

/* -------------------------------------------------------------------------
 * Input: tab switching
 * ---------------------------------------------------------------------- */

static void handle_tab_clicks(TmAppState *s, Vector2 mouse) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    for (int i = 0; i < TM_TAB_COUNT; i++) {
        if (!CheckCollisionPointRec(mouse, s->tabs[i].bounds)) continue;
        for (int j = 0; j < TM_TAB_COUNT; j++) s->tabs[j].is_active = (j == i);
        s->active_tab              = (TmTabId)i;
        s->selected_process_idx    = -1;
        s->selected_startup_idx    = -1;
        s->end_task_btn.is_enabled      = false;
        s->enable_startup_btn.is_enabled  = false;
        s->disable_startup_btn.is_enabled = false;
        break;
    }
}

/* -------------------------------------------------------------------------
 * Input: list row selection
 * ---------------------------------------------------------------------- */

static void handle_process_selection(TmAppState *s, Vector2 mouse) {
    if (!s->tabs[TM_TAB_PROCESSES].is_active) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Rectangle area = { 10, 120, (float)(s->screen_w - 30),
                        (float)(s->screen_h - 200) };
    if (!CheckCollisionPointRec(mouse, area)) return;

    int click_y  = (int)(mouse.y - 120) + s->process_scroll.scroll_pos;
    int new_idx  = click_y / TM_ROW_HEIGHT_PX;
    if (new_idx < 0 || new_idx >= s->process_count) return;

    s->selected_process_idx = new_idx;
    s->end_task_btn.is_enabled = true;

    TmProcess *cur = s->process_list;
    int         i   = 0;
    while (cur) {
        cur->is_selected = (i == new_idx);
        cur = cur->next;
        i++;
    }
}

static void handle_startup_selection(TmAppState *s, Vector2 mouse) {
    if (!s->tabs[TM_TAB_STARTUP].is_active) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    Rectangle area = { 30, 220, (float)(s->screen_w - 60),
                        (float)(s->screen_h - 300) };
    if (!CheckCollisionPointRec(mouse, area)) return;

    int click_y = (int)(mouse.y - 220) + s->startup_scroll.scroll_pos;
    int new_idx = click_y / TM_STARTUP_ROW_PX;
    TmStartupApp *app = tm_startup_get(s, new_idx);
    if (!app) return;

    s->selected_startup_idx           = new_idx;
    s->enable_startup_btn.is_enabled   = !app->is_enabled;
    s->disable_startup_btn.is_enabled  = app->is_enabled;
}

/* -------------------------------------------------------------------------
 * Input: keyboard shortcuts
 * ---------------------------------------------------------------------- */

static void handle_keyboard(TmAppState *s) {
    if (IsKeyPressed(KEY_F5)) {
        cmd_refresh(s, NULL);
    }
    if (IsKeyPressed(KEY_DELETE) && s->selected_process_idx >= 0) {
        cmd_end_task(s, NULL);
    }
    if (s->selected_startup_idx >= 0) {
        if (IsKeyPressed(KEY_E)) cmd_enable_startup(s, NULL);
        if (IsKeyPressed(KEY_D)) cmd_disable_startup(s, NULL);
    }
}

/* -------------------------------------------------------------------------
 * Input: scroll bars
 * ---------------------------------------------------------------------- */

static void handle_scrollbars(TmAppState *s, Vector2 mouse, float wheel) {
    if (s->active_tab == TM_TAB_PROCESSES) {
        Rectangle area = { 10, 120, (float)(s->screen_w - 30),
                           (float)(s->screen_h - 200) };
        ui_scrollbar_update(&s->process_scroll, mouse, wheel, area);
    } else if (s->active_tab == TM_TAB_STARTUP) {
        Rectangle area = { 20, 120, (float)(s->screen_w - 30),
                           (float)(s->screen_h - 170) };
        ui_scrollbar_update(&s->startup_scroll, mouse, wheel, area);
    } else if (s->active_tab == TM_TAB_APP_HISTORY) {
        Rectangle area = { 20, 120, (float)(s->screen_w - 30),
                           (float)(s->screen_h - 170) };
        ui_scrollbar_update(&s->history_scroll, mouse, wheel, area);
    }
}

/* -------------------------------------------------------------------------
 * Main input update
 * ---------------------------------------------------------------------- */

void ui_input_update(TmAppState *s) {
    if (!s) return;

    Vector2 mouse = GetMousePosition();
    float   wheel = GetMouseWheelMove();

    /* Update hover states */
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        s->tabs[i].is_hovered = CheckCollisionPointRec(mouse, s->tabs[i].bounds);
    }

    handle_tab_clicks(s, mouse);
    handle_process_selection(s, mouse);
    handle_startup_selection(s, mouse);
    handle_keyboard(s);
    handle_scrollbars(s, mouse, wheel);
    update_scrollbar_content(s);
}

/* -------------------------------------------------------------------------
 * Window resize
 * ---------------------------------------------------------------------- */

static bool is_mouse_over_resize_handle(const TmAppState *s, Vector2 mouse) {
    Rectangle handle = {
        (float)(s->screen_w - TM_RESIZE_BORDER_PX),
        (float)(s->screen_h - TM_RESIZE_BORDER_PX),
        TM_RESIZE_BORDER_PX, TM_RESIZE_BORDER_PX
    };
    return CheckCollisionPointRec(mouse, handle);
}

void ui_window_resize_handle(TmAppState *s) {
    if (!s) return;

    Vector2 mouse = GetMousePosition();
    bool    over  = is_mouse_over_resize_handle(s, mouse);

    SetMouseCursor(over ? MOUSE_CURSOR_RESIZE_NWSE : MOUSE_CURSOR_DEFAULT);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && over) s->is_resizing = true;

    if (s->is_resizing) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            int new_w = (int)mouse.x;
            int new_h = (int)mouse.y;
            if (new_w < TM_MIN_WINDOW_W) new_w = TM_MIN_WINDOW_W;
            if (new_h < TM_MIN_WINDOW_H) new_h = TM_MIN_WINDOW_H;
            SetWindowSize(new_w, new_h);
            s->screen_w = new_w;
            s->screen_h = new_h;
            ui_layout_update(s);
        } else {
            s->is_resizing = false;
        }
    }

    if (IsWindowResized() && !s->is_resizing) {
        s->screen_w = GetScreenWidth();
        s->screen_h = GetScreenHeight();
        ui_layout_update(s);
    }
}

/* -------------------------------------------------------------------------
 * Drawing
 * ---------------------------------------------------------------------- */

void ui_titlebar_draw(const TmAppState *s) {
    DrawRectangle(0, 0, s->screen_w, 40, TM_COLOR_ACCENT);
    DrawText("Advanced Task Manager", 10, 10, 20, WHITE);
}

void ui_tabs_draw(const TmAppState *s) {
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        const TmTab *tab  = &s->tabs[i];
        Color col         = tab->is_active ? TM_COLOR_ACCENT : TM_COLOR_HEADER;
        Color border_col  = tab->is_active ? TM_COLOR_ACCENT : (Color){ 80, 80, 80, 255 };

        DrawRectangleRec(tab->bounds, col);
        DrawRectangleLines((int)tab->bounds.x, (int)tab->bounds.y,
                           (int)tab->bounds.width, (int)tab->bounds.height, border_col);

        int text_w = MeasureText(tab->text, 16);
        int text_x = (int)(tab->bounds.x + (tab->bounds.width - text_w) / 2.0f);
        DrawText(tab->text, text_x, (int)(tab->bounds.y + 8),
                 16, tab->is_active ? WHITE : TM_COLOR_SUBTLE);
    }
}

void ui_buttons_draw(const TmAppState *s) {
    /* Refresh is always visible */
    ui_button_draw_and_handle(
        (TmButton *)&s->refresh_btn,
        (TmAppState *)s, cmd_refresh, NULL);

    if (s->active_tab == TM_TAB_PROCESSES) {
        ui_button_draw_and_handle(
            (TmButton *)&s->end_task_btn,
            (TmAppState *)s, cmd_end_task, NULL);
    }
    if (s->active_tab == TM_TAB_STARTUP) {
        ui_button_draw_and_handle(
            (TmButton *)&s->enable_startup_btn,
            (TmAppState *)s, cmd_enable_startup, NULL);
        ui_button_draw_and_handle(
            (TmButton *)&s->disable_startup_btn,
            (TmAppState *)s, cmd_disable_startup, NULL);
    }
}

void ui_statusbar_draw(const TmAppState *s) {
    DrawRectangle(0, s->screen_h - 80, s->screen_w, 80, TM_COLOR_HEADER);
    DrawText("F5: Refresh   |   Delete: End Task   |   E/D: Enable/Disable Startup",
             15, s->screen_h - 35, 14, TM_COLOR_SUBTLE);
}

void ui_resize_handle_draw(const TmAppState *s) {
    for (int i = 0; i < 3; i++) {
        DrawLine(s->screen_w - 12 + i * 4, s->screen_h - 4,
                 s->screen_w - 4,          s->screen_h - 12 + i * 4,
                 TM_COLOR_SUBTLE);
    }
}

void ui_content_draw(const TmAppState *s) {
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        if (k_tabs[i].id == s->active_tab) {
            k_tabs[i].draw(s);
            return;
        }
    }
}

/* -------------------------------------------------------------------------
 * Toast notification
 * ---------------------------------------------------------------------- */

void ui_toast_show(TmAppState *s, const char *msg, Color color, int frames) {
    if (!s || !msg) return;
    strncpy(s->message, msg, TM_MSG_MAX - 1);
    s->message[TM_MSG_MAX - 1] = '\0';
    s->message_color            = color;
    s->message_timer            = frames;
}

void ui_toast_tick(TmAppState *s) {
    if (s && s->message_timer > 0) s->message_timer--;
}

void ui_toast_draw(const TmAppState *s) {
    if (!s || s->message_timer <= 0) return;

    int text_w = MeasureText(s->message, 16);
    int msg_x  = s->screen_w / 2 - text_w / 2;

    DrawRectangle(msg_x - 10, 10, text_w + 20, 30, s->message_color);
    DrawRectangleLines(msg_x - 10, 10, text_w + 20, 30,
                       (Color){ 100, 100, 100, 255 });
    DrawText(s->message, msg_x, 15, 16, WHITE);
}
