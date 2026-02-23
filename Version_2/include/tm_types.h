/**
 * @file tm_types.h
 * @brief All shared types, result codes, and constants for the Task Manager.
 *
 * This is the single source of truth for the public API boundary.
 * All other modules import from here only -- prevents circular includes.
 */

#ifndef TM_TYPES_H
#define TM_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "raylib.h"

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#define TM_NAME_MAX           256
#define TM_PUBLISHER_MAX      256
#define TM_STATUS_MAX         32
#define TM_CMD_MAX            256
#define TM_MSG_MAX            256
#define TM_HIST_LEN           100
#define TM_HIST_SHORT         30
#define TM_CORE_COUNT         8
#define TM_MAX_OBSERVERS      8
#define TM_MAX_STARTUP_APPS   8
#define TM_MAX_HISTORY_APPS   8

#define TM_RESIZE_BORDER_PX   8
#define TM_ROW_HEIGHT_PX      30
#define TM_STARTUP_ROW_PX     45
#define TM_HISTORY_ROW_PX     65
#define TM_HEADER_HEIGHT_PX   25
#define TM_TAB_HEIGHT_PX      35
#define TM_BUTTON_HEIGHT_PX   30
#define TM_SCROLLBAR_WIDTH_PX 12
#define TM_MIN_WINDOW_W       800
#define TM_MIN_WINDOW_H       600

#define TM_PERF_UPDATE_INTERVAL_S 1.0f
#define TM_HISTORY_UPDATE_INTERVAL_S 2.0f
#define TM_MSG_DISPLAY_FRAMES 120
#define TM_MSG_SHORT_FRAMES   60

/* -------------------------------------------------------------------------
 * Result Codes
 * ---------------------------------------------------------------------- */

/** Return type for all fallible public functions. */
typedef enum {
    TM_OK              =  0,
    TM_ERR_ALLOC       = -1,  /**< malloc / arena exhausted */
    TM_ERR_IO          = -2,  /**< popen / file failure */
    TM_ERR_PLATFORM    = -3,  /**< OS call failed */
    TM_ERR_INVALID_ARG = -4,  /**< NULL or out-of-range param */
} tm_result_t;

/**
 * Mandatory result check macro.
 * Logs and propagates errors; asserts in debug builds.
 */
#define TM_CHECK(expr) \
    do { \
        tm_result_t _r = (expr); \
        if (_r != TM_OK) { \
            tm_log_error("TM_CHECK failed: %s (%s:%d)", #expr, __FILE__, __LINE__); \
            return _r; \
        } \
    } while (0)

/* -------------------------------------------------------------------------
 * Tab IDs
 * ---------------------------------------------------------------------- */

typedef enum {
    TM_TAB_PROCESSES   = 0,
    TM_TAB_PERFORMANCE = 1,
    TM_TAB_APP_HISTORY = 2,
    TM_TAB_STARTUP     = 3,
    TM_TAB_COUNT       = 4,
} TmTabId;

/* -------------------------------------------------------------------------
 * Core Data Structures
 * ---------------------------------------------------------------------- */

/** A single process entry (intrusive singly-linked list). */
typedef struct TmProcess {
    char             name[TM_NAME_MAX];
    uint32_t         pid;
    uint64_t         memory_bytes;
    float            cpu_percent;
    bool             is_selected;
    struct TmProcess *next;
} TmProcess;

/** A single startup application entry. */
typedef struct TmStartupApp {
    char                name[TM_NAME_MAX];
    char                publisher[TM_PUBLISHER_MAX];
    char                status[TM_STATUS_MAX];
    float               impact_s;
    bool                is_enabled;
    struct TmStartupApp *next;
} TmStartupApp;

/** Per-application resource history (30-sample ring buffer). */
typedef struct TmAppHistory {
    char                 name[TM_NAME_MAX];
    float                cpu_time;
    float                cpu_time_history[TM_HIST_SHORT];
    uint64_t             memory_kb;
    uint64_t             memory_history[TM_HIST_SHORT];
    uint64_t             network_kb;
    uint64_t             network_history[TM_HIST_SHORT];
    int                  history_idx;
    clock_t              last_update;
    struct TmAppHistory *next;
} TmAppHistory;

/** System-wide performance metrics with 100-sample history rings. */
typedef struct {
    float    cpu_percent;
    float    cpu_history[TM_HIST_LEN];
    int      cpu_idx;

    uint64_t mem_used_kb;
    uint64_t mem_total_kb;
    uint64_t mem_available_kb;
    uint64_t mem_history[TM_HIST_LEN];
    int      mem_idx;

    uint64_t disk_used_kb;
    uint64_t disk_total_kb;
    uint64_t disk_history[TM_HIST_LEN];
    int      disk_idx;

    float    gpu_percent;
    float    gpu_history[TM_HIST_LEN];
    int      gpu_idx;

    int      process_count;
    int      thread_count;
    uint32_t uptime_s;

    clock_t  last_update;
} TmPerfData;

/* -------------------------------------------------------------------------
 * UI Structures
 * ---------------------------------------------------------------------- */

typedef struct {
    Rectangle bounds;
    char      text[50];
    bool      is_hovered;
    Color     color;
    Color     hover_color;
    bool      is_enabled;
} TmButton;

typedef struct {
    Rectangle bounds;
    char      text[50];
    bool      is_active;
    bool      is_hovered;
} TmTab;

typedef struct {
    Rectangle bounds;
    Rectangle thumb;
    bool      is_dragging;
    int       drag_offset;
    int       content_height;
    int       visible_height;
    int       scroll_pos;
    int       max_scroll;
} TmScrollBar;

/* -------------------------------------------------------------------------
 * Application State
 * ---------------------------------------------------------------------- */

/** Forward-declare command function pointer. */
struct TmAppState;
typedef tm_result_t (*TmCommandFn)(struct TmAppState *s, void *param);

/** Encapsulates all mutable application state; passed by pointer everywhere. */
typedef struct TmAppState {
    /* Data layer */
    TmProcess    *process_list;
    int           process_count;
    TmStartupApp *startup_list;
    TmAppHistory *history_list;
    TmPerfData    perf;
    float         cpu_core_usage[TM_CORE_COUNT];

    /* UI state */
    TmTab       tabs[TM_TAB_COUNT];
    TmButton    refresh_btn;
    TmButton    end_task_btn;
    TmButton    enable_startup_btn;
    TmButton    disable_startup_btn;
    TmScrollBar process_scroll;
    TmScrollBar startup_scroll;
    TmScrollBar history_scroll;

    /* Selection */
    int           selected_process_idx;
    int           selected_startup_idx;
    TmTabId       active_tab;

    /* Notification message */
    char  message[TM_MSG_MAX];
    int   message_timer;
    Color message_color;

    /* Window */
    int  screen_w;
    int  screen_h;
    bool is_resizing;
} TmAppState;

/* -------------------------------------------------------------------------
 * Observer
 * ---------------------------------------------------------------------- */

typedef void (*TmProcessChangedFn)(const TmAppState *s, void *user_data);

typedef struct {
    TmProcessChangedFn on_process_changed;
    void              *user_data;
} TmObserver;

/* -------------------------------------------------------------------------
 * Tab descriptor (Strategy Pattern)
 * ---------------------------------------------------------------------- */

typedef void (*TmTabDrawFn)(const TmAppState *s);

typedef struct {
    TmTabId      id;
    const char  *label;
    TmTabDrawFn  draw;
} TmTabDescriptor;

#endif /* TM_TYPES_H */
