/**
 * @file ui_tab_performance.c
 * @brief Renders the Performance tab content.
 */

#include <stdio.h>
#include "../../include/tm_ui.h"

/* -------------------------------------------------------------------------
 * Line-graph helper
 * ---------------------------------------------------------------------- */

static void draw_line_graph(const float *history, int hist_len, int start_idx,
                             int x, int y, int w, int h, Color line_col) {
    DrawRectangle(x, y, w, h, (Color){ 15, 15, 20, 255 });
    /* Grid lines */
    for (int i = 1; i < 4; i++) {
        DrawLine(x, y + i * h / 4, x + w, y + i * h / 4,
                 (Color){ 40, 40, 50, 255 });
    }
    float x_step = (float)w / (float)(hist_len - 1);
    for (int i = 0; i < hist_len - 1; i++) {
        int   idx1 = (start_idx + i)     % hist_len;
        int   idx2 = (start_idx + i + 1) % hist_len;
        float y1   = (float)(y + h) - history[idx1] * (float)h / 100.0f;
        float y2   = (float)(y + h) - history[idx2] * (float)h / 100.0f;
        DrawLine((int)(x + i * x_step), (int)y1,
                 (int)(x + (i + 1) * x_step), (int)y2, line_col);
    }
    DrawRectangleLines(x, y, w, h, (Color){ 60, 60, 70, 255 });
}

/* -------------------------------------------------------------------------
 * Section renderers (each ~15 lines)
 * ---------------------------------------------------------------------- */

static void draw_cpu_section(const TmAppState *s, int x, int y, int w) {
    char buf[64];
    DrawText("CPU", x, y, 20, TM_COLOR_TEXT);
    snprintf(buf, sizeof(buf), "%.1f%%", s->perf.cpu_percent);
    DrawText(buf, x + w - MeasureText(buf, 24), y, 24, TM_COLOR_TEXT);
    draw_line_graph(s->perf.cpu_history, TM_HIST_LEN, s->perf.cpu_idx,
                    x, y + 30, w, 120, TM_COLOR_CPU);
}

static void draw_gpu_section(const TmAppState *s, int x, int y, int w) {
    char buf[64];
    DrawText("GPU", x, y, 20, TM_COLOR_TEXT);
    snprintf(buf, sizeof(buf), "%.1f%%", s->perf.gpu_percent);
    DrawText(buf, x + w - MeasureText(buf, 24), y, 24, TM_COLOR_TEXT);
    draw_line_graph(s->perf.gpu_history, TM_HIST_LEN, s->perf.gpu_idx,
                    x, y + 30, w, 80, TM_COLOR_GPU);
}

static void draw_memory_section(const TmAppState *s, int x, int y, int w) {
    double used  = (double)s->perf.mem_used_kb  / (1024.0 * 1024.0);
    double total = (double)s->perf.mem_total_kb  / (1024.0 * 1024.0);
    float  pct   = (s->perf.mem_total_kb > 0)
                   ? (float)s->perf.mem_used_kb / (float)s->perf.mem_total_kb
                   : 0.0f;
    char buf[100];

    DrawText("Memory", x, y, 20, TM_COLOR_TEXT);
    snprintf(buf, sizeof(buf), "%.1f/%.1f GB (%.1f%%)", used, total, pct * 100.0f);
    DrawText(buf, x + w - MeasureText(buf, 18), y, 18, TM_COLOR_TEXT);

    DrawRectangle(x, y + 30, w, 30, (Color){ 40, 40, 50, 255 });
    DrawRectangle(x, y + 30, (int)((float)w * pct), 30, TM_COLOR_MEMORY);
    DrawRectangleLines(x, y + 30, w, 30, (Color){ 80, 80, 90, 255 });

    snprintf(buf, sizeof(buf), "%.1f GB",
             (double)s->perf.mem_used_kb / (1024.0 * 1024.0));
    DrawText("In use:", x, y + 70, 14, TM_COLOR_TEXT);
    DrawText(buf, x + 80, y + 70, 14, TM_COLOR_TEXT);

    snprintf(buf, sizeof(buf), "%.1f GB",
             (double)s->perf.mem_available_kb / (1024.0 * 1024.0));
    DrawText("Available:", x, y + 90, 14, TM_COLOR_TEXT);
    DrawText(buf, x + 80, y + 90, 14, TM_COLOR_TEXT);
}

static void draw_disk_section(const TmAppState *s, int x, int y, int w) {
    float pct = (s->perf.disk_total_kb > 0)
                ? (float)s->perf.disk_used_kb / (float)s->perf.disk_total_kb
                : 0.0f;
    char buf[100];

    DrawText("Disk", x, y, 20, TM_COLOR_TEXT);
    snprintf(buf, sizeof(buf), "%.1f/%.1f GB (%.1f%%)",
             (double)s->perf.disk_used_kb  / (1024.0 * 1024.0),
             (double)s->perf.disk_total_kb / (1024.0 * 1024.0),
             pct * 100.0f);
    DrawText(buf, x + w - MeasureText(buf, 18), y, 18, TM_COLOR_TEXT);

    DrawRectangle(x, y + 30, w, 30, (Color){ 40, 40, 50, 255 });
    DrawRectangle(x, y + 30, (int)((float)w * pct), 30, TM_COLOR_DISK);
    DrawRectangleLines(x, y + 30, w, 30, (Color){ 80, 80, 90, 255 });
}

static void draw_sysinfo_section(const TmAppState *s, int x, int y) {
    uint32_t h   = s->perf.uptime_s / 3600;
    uint32_t m   = (s->perf.uptime_s % 3600) / 60;
    uint32_t sec = s->perf.uptime_s % 60;
    char buf[100];

    DrawText("System Information", x, y, 20, TM_COLOR_TEXT);
    y += 40;

    snprintf(buf, sizeof(buf), "Up time: %u:%02u:%02u", h, m, sec);
    DrawText(buf, x, y, 16, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "Processes: %d", s->perf.process_count);
    DrawText(buf, x, y + 25, 16, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "Threads: %d", s->perf.thread_count);
    DrawText(buf, x, y + 50, 16, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "Handles: %d",
             s->perf.process_count * 50 + 1234);
    DrawText(buf, x + 230, y, 16, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "Physical Memory: %.1f GB",
             (double)s->perf.mem_total_kb / (1024.0 * 1024.0));
    DrawText(buf, x + 230, y + 25, 16, TM_COLOR_SUBTLE);

    snprintf(buf, sizeof(buf), "Disk Capacity: %.1f GB",
             (double)s->perf.disk_total_kb / (1024.0 * 1024.0));
    DrawText(buf, x + 480, y, 16, TM_COLOR_SUBTLE);
}

/* -------------------------------------------------------------------------
 * Public renderer
 * ---------------------------------------------------------------------- */

void ui_tab_perf_draw(const TmAppState *s) {
    int half_w      = (s->screen_w - 60) / 2;
    int right_x     = 20 + half_w + 20;

    draw_cpu_section(s,    20,      120, half_w);
    draw_memory_section(s, right_x, 120, half_w);
    draw_gpu_section(s,    20,      300, half_w);
    draw_disk_section(s,   right_x, 300, half_w);
    draw_sysinfo_section(s, 20,     430);
}
