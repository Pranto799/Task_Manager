/**
 * @file tm_ui.h
 * @brief Public API for all Raylib rendering and UI interaction.
 *
 * All functions in this header call Raylib and must only be called
 * inside a BeginDrawing() / EndDrawing() block (except init/resize).
 */

#ifndef TM_UI_H
#define TM_UI_H

#include "tm_types.h"

/* -------------------------------------------------------------------------
 * Theme colours (defined in ui/ui_theme.c)
 * ---------------------------------------------------------------------- */

extern const Color TM_COLOR_BG;
extern const Color TM_COLOR_HEADER;
extern const Color TM_COLOR_ROW1;
extern const Color TM_COLOR_ROW2;
extern const Color TM_COLOR_SELECTED;
extern const Color TM_COLOR_ACCENT;
extern const Color TM_COLOR_TEXT;
extern const Color TM_COLOR_SUBTLE;
extern const Color TM_COLOR_CPU;
extern const Color TM_COLOR_MEMORY;
extern const Color TM_COLOR_DISK;
extern const Color TM_COLOR_GPU;
extern const Color TM_COLOR_ENABLED;
extern const Color TM_COLOR_DISABLED;

/* -------------------------------------------------------------------------
 * Layout / Init
 * ---------------------------------------------------------------------- */

/**
 * Initialise all UI subsystems (tabs, buttons, scrollbars).
 * Must be called once before the main loop.
 * @param s  Application state. Must not be NULL.
 */
void ui_init(TmAppState *s);

/**
 * Recompute all layout rectangles after a window resize.
 * @param s  Application state. Must not be NULL.
 */
void ui_layout_update(TmAppState *s);

/* -------------------------------------------------------------------------
 * Per-frame update (input + drawing)
 * ---------------------------------------------------------------------- */

/**
 * Handle all input, update hover states, and dispatch button commands.
 * Call once per frame before drawing.
 * @param s  Application state. Must not be NULL.
 */
void ui_input_update(TmAppState *s);

/**
 * Handle window resize events (including drag-resize handle).
 * @param s  Application state. Must not be NULL.
 */
void ui_window_resize_handle(TmAppState *s);

/* -------------------------------------------------------------------------
 * Drawing
 * ---------------------------------------------------------------------- */

/** Draw the title bar. */
void ui_titlebar_draw(const TmAppState *s);

/** Draw all tabs (active/inactive highlight). */
void ui_tabs_draw(const TmAppState *s);

/** Draw context-sensitive action buttons. */
void ui_buttons_draw(const TmAppState *s);

/** Draw the status bar at the bottom of the window. */
void ui_statusbar_draw(const TmAppState *s);

/** Draw the resize handle in the bottom-right corner. */
void ui_resize_handle_draw(const TmAppState *s);

/** Draw the active tab's content area. Dispatches via strategy table. */
void ui_content_draw(const TmAppState *s);

/** Draw the notification toast message if one is active. */
void ui_toast_draw(const TmAppState *s);

/**
 * Decrement the message timer (call once per frame outside drawing block).
 * @param s  Application state. Must not be NULL.
 */
void ui_toast_tick(TmAppState *s);

/**
 * Set and display a toast notification.
 * @param s      Application state. Must not be NULL.
 * @param msg    Null-terminated message text.
 * @param color  Background colour for the toast.
 * @param frames Number of frames to display.
 */
void ui_toast_show(TmAppState *s, const char *msg, Color color, int frames);

/* -------------------------------------------------------------------------
 * Scrollbar helpers (ui/ui_scrollbar.c)
 * ---------------------------------------------------------------------- */

/**
 * Update scrollbar geometry, handle mouse interaction, and handle wheel.
 * @param sb            Scrollbar to update. Must not be NULL.
 * @param mouse         Current mouse position.
 * @param wheel         Mouse wheel delta (from GetMouseWheelMove()).
 * @param scroll_area   Rectangle of the scrollable content region.
 */
void ui_scrollbar_update(TmScrollBar *sb, Vector2 mouse, float wheel,
                         Rectangle scroll_area);

/** Draw the scrollbar if content exceeds the visible area. */
void ui_scrollbar_draw(const TmScrollBar *sb);

/* -------------------------------------------------------------------------
 * Button helpers (ui/ui_button.c)
 * ---------------------------------------------------------------------- */

/**
 * Draw a button and execute its command on click.
 * @param btn      Button descriptor. Must not be NULL.
 * @param s        Application state passed to the command. Must not be NULL.
 * @param cmd      Command function pointer, or NULL for a display-only button.
 * @param param    Optional parameter forwarded to cmd().
 */
void ui_button_draw_and_handle(TmButton *btn, TmAppState *s,
                                TmCommandFn cmd, void *param);

/* -------------------------------------------------------------------------
 * Tab content renderers (one file each in ui/)
 * ---------------------------------------------------------------------- */

void ui_tab_process_draw(const TmAppState *s);
void ui_tab_perf_draw(const TmAppState *s);
void ui_tab_history_draw(const TmAppState *s);
void ui_tab_startup_draw(const TmAppState *s);

#endif /* TM_UI_H */
