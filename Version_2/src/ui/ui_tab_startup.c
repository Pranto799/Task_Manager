/**
 * @file ui_tab_startup.c
 * @brief Renders the Startup tab content.
 */

#include <stdio.h>
#include "../../include/tm_ui.h"
#include "../../include/tm_startup.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static void draw_startup_header(int content_w, int content_h) {
    DrawRectangle(20, 120, content_w, content_h, TM_COLOR_HEADER);
    DrawText("Startup Applications", 30, 140, 20, TM_COLOR_TEXT);
    DrawText("Programs that run when system starts", 30, 170, 16, TM_COLOR_SUBTLE);
}

static void draw_startup_row(const TmStartupApp *app, int y, int content_w,
                              int row_idx, bool is_selected) {
    Color row_col = is_selected
                    ? TM_COLOR_SELECTED
                    : ((row_idx % 2 == 0) ? TM_COLOR_ROW1 : TM_COLOR_ROW2);

    DrawRectangle(30, y, content_w - 20, 40, row_col);
    DrawRectangle(35, y + 12, 16, 16, TM_COLOR_ACCENT);

    DrawText(app->name,      60, y + 8,  14, TM_COLOR_TEXT);
    DrawText(app->publisher, 60, y + 24, 12, TM_COLOR_SUBTLE);

    Color status_col = app->is_enabled ? TM_COLOR_ENABLED : TM_COLOR_DISABLED;
    DrawText(app->status, 400, y + 16, 14, status_col);

    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f s", app->impact_s);
    DrawText(buf, 500, y + 16, 14, TM_COLOR_SUBTLE);
}

/* -------------------------------------------------------------------------
 * Public renderer
 * ---------------------------------------------------------------------- */

void ui_tab_startup_draw(const TmAppState *s) {
    int content_w  = s->screen_w - 30;
    int content_h  = s->screen_h - 170;

    draw_startup_header(content_w, content_h);

    int scroll    = s->startup_scroll.scroll_pos;
    int start_y   = 220;
    int max_vis   = (content_h - 100) / TM_STARTUP_ROW_PX + 1;

    const TmStartupApp *cur     = s->startup_list;
    int                 idx     = 0;
    int                 vis_cnt = 0;

    while (cur && vis_cnt < max_vis) {
        int y = start_y + idx * TM_STARTUP_ROW_PX - scroll;
        if (y >= start_y && y < start_y + content_h - TM_STARTUP_ROW_PX) {
            bool selected = (idx == s->selected_startup_idx);
            draw_startup_row(cur, y, content_w, idx, selected);
            vis_cnt++;
        }
        cur = cur->next;
        idx++;
    }

    ui_scrollbar_draw(&s->startup_scroll);
}
