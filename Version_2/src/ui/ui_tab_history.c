/**
 * @file ui_tab_history.c
 * @brief Renders the App History tab content.
 */

#include <stdio.h>
#include "../../include/tm_ui.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static void draw_history_header(int content_w) {
    DrawRectangle(20, 120, content_w, 85, TM_COLOR_HEADER);
    DrawText("Application History", 30, 140, 20, TM_COLOR_TEXT);
    DrawText("Resource usage history for applications (Last 30 samples)",
             30, 170, 16, TM_COLOR_SUBTLE);

    DrawRectangle(20, 200, content_w, TM_HEADER_HEIGHT_PX,
                  (Color){ 50, 50, 60, 255 });
    DrawText("Application", 30,  205, 14, TM_COLOR_TEXT);
    DrawText("CPU Time",   250,  205, 14, TM_COLOR_TEXT);
    DrawText("Memory",     350,  205, 14, TM_COLOR_TEXT);
    DrawText("Network",    450,  205, 14, TM_COLOR_TEXT);
    DrawText("History",    550,  205, 14, TM_COLOR_TEXT);
}

static float max_float_array(const float *arr, int len) {
    float m = 1.0f;
    for (int i = 0; i < len; i++) {
        if (arr[i] > m) m = arr[i];
    }
    return m;
}

static void draw_history_mini_graph(const TmAppHistory *app, int x, int y,
                                     int w, int h) {
    DrawRectangle(x, y, w, h, (Color){ 20, 20, 20, 255 });
    float max_cpu = max_float_array(app->cpu_time_history, TM_HIST_SHORT);
    float x_step  = (float)w / (float)(TM_HIST_SHORT - 1);

    for (int i = 0; i < TM_HIST_SHORT - 1; i++) {
        int   idx1 = (app->history_idx + i)     % TM_HIST_SHORT;
        int   idx2 = (app->history_idx + i + 1) % TM_HIST_SHORT;
        float y1   = (float)(y + h) - app->cpu_time_history[idx1] * (float)h / max_cpu;
        float y2   = (float)(y + h) - app->cpu_time_history[idx2] * (float)h / max_cpu;
        DrawLine((int)(x + i * x_step), (int)y1,
                 (int)(x + (i + 1) * x_step), (int)y2, TM_COLOR_CPU);
    }
    DrawRectangleLines(x, y, w, h, (Color){ 80, 80, 80, 255 });
}

static void draw_history_row(const TmAppHistory *app, int y, int content_w,
                              int row_idx) {
    Color row_col = (row_idx % 2 == 0) ? TM_COLOR_ROW1 : TM_COLOR_ROW2;
    DrawRectangle(20, y, content_w, 60, row_col);

    DrawRectangle(25, y + 5, 12, 12, TM_COLOR_ACCENT);
    DrawText(app->name, 45, y + 5, 14, TM_COLOR_TEXT);

    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f%%", app->cpu_time);
    DrawText(buf, 250, y + 5, 14, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "%.1f MB",
             (double)app->memory_kb / 1024.0);
    DrawText(buf, 350, y + 5, 14, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "%.1f KB/s",
             (double)app->network_kb);
    DrawText(buf, 450, y + 5, 14, TM_COLOR_SUBTLE);

    draw_history_mini_graph(app, 550, y + 10, 200, 40);
}

/* -------------------------------------------------------------------------
 * Public renderer
 * ---------------------------------------------------------------------- */

void ui_tab_history_draw(const TmAppState *s) {
    int content_w  = s->screen_w - 30;
    int content_h  = s->screen_h - 170;

    draw_history_header(content_w);

    int scroll     = s->history_scroll.scroll_pos;
    int start_y    = 230;
    int viewport_h = content_h - 30;
    int max_vis    = viewport_h / TM_HISTORY_ROW_PX + 1;

    const TmAppHistory *cur     = s->history_list;
    int                 idx     = 0;
    int                 vis_cnt = 0;

    while (cur && vis_cnt < max_vis) {
        int y = start_y + idx * TM_HISTORY_ROW_PX - scroll;
        if (y >= start_y && y < start_y + viewport_h) {
            draw_history_row(cur, y, content_w, idx);
            vis_cnt++;
        }
        cur = cur->next;
        idx++;
    }

    ui_scrollbar_draw(&s->history_scroll);
}
