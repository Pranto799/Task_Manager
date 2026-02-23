/**
 * @file ui_scrollbar.c
 * @brief Scrollbar rendering and interaction helpers.
 *
 * All functions are pure with respect to Raylib state -- no hidden
 * calls to GetMousePosition(). Mouse data is injected by the caller.
 */

#include "../../include/tm_ui.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static bool ui_scrollbar_needed(const TmScrollBar *sb) {
    return sb->content_height > sb->visible_height;
}

static float ui_scrollbar_thumb_h(const TmScrollBar *sb) {
    float th = (float)(sb->visible_height * sb->visible_height)
               / (float)sb->content_height;
    return (th < 30.0f) ? 30.0f : th;
}

static Rectangle ui_scrollbar_thumb_rect(const TmScrollBar *sb, float th) {
    float range = sb->bounds.height - th;
    float y     = (sb->max_scroll > 0)
                  ? sb->bounds.y + ((float)sb->scroll_pos * range)
                                   / (float)sb->max_scroll
                  : sb->bounds.y;
    return (Rectangle){ sb->bounds.x, y, sb->bounds.width, th };
}

static void ui_scrollbar_handle_click(TmScrollBar *sb, Vector2 mouse, float th) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    if (CheckCollisionPointRec(mouse, sb->thumb)) {
        sb->is_dragging = true;
        sb->drag_offset = (int)(mouse.y - sb->thumb.y);
        return;
    }
    if (CheckCollisionPointRec(mouse, sb->bounds)) {
        float new_y = mouse.y - th / 2.0f;
        float lo    = sb->bounds.y;
        float hi    = sb->bounds.y + sb->bounds.height - th;
        if (new_y < lo) new_y = lo;
        if (new_y > hi) new_y = hi;
        float range = sb->bounds.height - th;
        sb->scroll_pos = (range > 0)
                         ? (int)((new_y - sb->bounds.y) * sb->max_scroll / range)
                         : 0;
    }
}

static void ui_scrollbar_handle_drag(TmScrollBar *sb, Vector2 mouse, float th) {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        sb->is_dragging = false;
        return;
    }
    if (!sb->is_dragging) return;

    float new_y = mouse.y - (float)sb->drag_offset;
    float lo    = sb->bounds.y;
    float hi    = sb->bounds.y + sb->bounds.height - th;
    if (new_y < lo) new_y = lo;
    if (new_y > hi) new_y = hi;
    float range = sb->bounds.height - th;
    sb->scroll_pos = (range > 0)
                     ? (int)((new_y - sb->bounds.y) * sb->max_scroll / range)
                     : 0;
}

static void ui_scrollbar_handle_wheel(TmScrollBar *sb, float wheel,
                                      Rectangle scroll_area, Vector2 mouse) {
    if (wheel == 0.0f) return;
    if (!CheckCollisionPointRec(mouse, scroll_area)) return;

    sb->scroll_pos -= (int)(wheel * TM_ROW_HEIGHT_PX);
    if (sb->scroll_pos < 0)              sb->scroll_pos = 0;
    if (sb->scroll_pos > sb->max_scroll) sb->scroll_pos = sb->max_scroll;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void ui_scrollbar_update(TmScrollBar *sb, Vector2 mouse, float wheel,
                         Rectangle scroll_area) {
    if (!sb || !ui_scrollbar_needed(sb)) return;

    float th   = ui_scrollbar_thumb_h(sb);
    sb->thumb  = ui_scrollbar_thumb_rect(sb, th);

    ui_scrollbar_handle_click(sb, mouse, th);
    ui_scrollbar_handle_drag(sb, mouse, th);
    ui_scrollbar_handle_wheel(sb, wheel, scroll_area, mouse);
}

void ui_scrollbar_draw(const TmScrollBar *sb) {
    if (!sb || sb->content_height <= sb->visible_height) return;

    DrawRectangleRec(sb->bounds, (Color){ 50, 50, 60, 255 });
    Color thumb_col = sb->is_dragging
                      ? (Color){ 200, 200, 220, 255 }
                      : TM_COLOR_ACCENT;
    DrawRectangleRec(sb->thumb, thumb_col);
}
