/**
 * @file ui_tab_processes.c
 * @brief Renders the Processes tab content.
 */

#include <stdio.h>
#include "../../include/tm_ui.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static void draw_column_headers(int content_w) {
    DrawRectangle(10, 90, content_w, TM_HEADER_HEIGHT_PX, TM_COLOR_HEADER);
    DrawText("Name",   20,  95, 16, TM_COLOR_TEXT);
    DrawText("PID",   300,  95, 16, TM_COLOR_TEXT);
    DrawText("CPU",   400,  95, 16, TM_COLOR_TEXT);
    DrawText("Memory",500,  95, 16, TM_COLOR_TEXT);
}

static Color cpu_value_color(float cpu) {
    return (cpu > 50.0f) ? (Color){ 255, 100, 100, 255 } : TM_COLOR_SUBTLE;
}

static void draw_process_row(const TmProcess *proc, int y_pos,
                              int content_w, int row_index) {
    Color row_col = proc->is_selected
                    ? TM_COLOR_SELECTED
                    : ((row_index % 2 == 0) ? TM_COLOR_ROW1 : TM_COLOR_ROW2);

    DrawRectangle(10, y_pos, content_w, TM_ROW_HEIGHT_PX, row_col);
    DrawRectangle(20, y_pos + 8, 12, 12, TM_COLOR_ACCENT);
    DrawText(proc->name, 37, y_pos + 8, 14, TM_COLOR_TEXT);

    char buf[32];
    snprintf(buf, sizeof(buf), "%u", proc->pid);
    DrawText(buf, 300, y_pos + 8, 14, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "%.1f%%", proc->cpu_percent);
    DrawText(buf, 400, y_pos + 8, 14, cpu_value_color(proc->cpu_percent));

    snprintf(buf, sizeof(buf), "%.1f MB",
             (double)proc->memory_bytes / (1024.0 * 1024.0));
    DrawText(buf, 500, y_pos + 8, 14, TM_COLOR_SUBTLE);
}

static void draw_process_rows(const TmAppState *s, int start_y,
                               int list_h, int content_w) {
    int scroll_px = s->process_scroll.scroll_pos;
    int skip      = scroll_px / TM_ROW_HEIGHT_PX;
    int row_off   = scroll_px % TM_ROW_HEIGHT_PX;
    int max_vis   = list_h / TM_ROW_HEIGHT_PX + 1;

    const TmProcess *cur = s->process_list;
    int abs_idx  = 0;
    int vis_idx  = 0;

    /* Skip rows above the viewport */
    while (cur && abs_idx < skip) {
        cur = cur->next;
        abs_idx++;
    }

    while (cur && vis_idx < max_vis) {
        int y = start_y + vis_idx * TM_ROW_HEIGHT_PX - row_off;
        if (y >= start_y && y < start_y + list_h) {
            draw_process_row(cur, y, content_w, abs_idx);
        }
        cur = cur->next;
        abs_idx++;
        vis_idx++;
    }
}

static void draw_stats_bar(const TmAppState *s) {
    DrawRectangle(0, s->screen_h - 80, s->screen_w, 80, TM_COLOR_HEADER);
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Processes: %d | CPU Usage: %.1f%% | Memory: %.1f/%.1f GB",
             s->process_count, s->perf.cpu_percent,
             (double)s->perf.mem_used_kb  / (1024.0 * 1024.0),
             (double)s->perf.mem_total_kb / (1024.0 * 1024.0));
    DrawText(buf, 15, s->screen_h - 65, 14, TM_COLOR_SUBTLE);
}

/* -------------------------------------------------------------------------
 * Public renderer
 * ---------------------------------------------------------------------- */

void ui_tab_process_draw(const TmAppState *s) {
    int start_y    = 120;
    int content_w  = s->screen_w - 30;
    int list_h     = s->screen_h - 200;

    draw_column_headers(content_w);
    draw_process_rows(s, start_y, list_h, content_w);
    ui_scrollbar_draw(&s->process_scroll);
    draw_stats_bar(s);
}
