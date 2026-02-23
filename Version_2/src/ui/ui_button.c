/**
 * @file ui_button.c
 * @brief Button rendering and command dispatch (Command Pattern).
 */

#include "../../include/tm_ui.h"
#include "../../include/tm_log.h"

static Color resolve_color(const TmButton *btn) {
    if (!btn->is_enabled) return (Color){ 80, 80, 80, 255 };
    return btn->is_hovered ? btn->hover_color : btn->color;
}

static void draw_button_background(const TmButton *btn) {
    DrawRectangleRec(btn->bounds, resolve_color(btn));
    DrawRectangleLines(
        (int)btn->bounds.x, (int)btn->bounds.y,
        (int)btn->bounds.width, (int)btn->bounds.height,
        (Color){ 80, 80, 80, 255 });
}

static void draw_button_label(const TmButton *btn) {
    int text_w = MeasureText(btn->text, 14);
    int text_x = (int)(btn->bounds.x + (btn->bounds.width - text_w) / 2.0f);
    int text_y = (int)(btn->bounds.y + 7.0f);
    DrawText(btn->text, text_x, text_y, 14, TM_COLOR_TEXT);
}

static bool is_clicked(const TmButton *btn, Vector2 mouse) {
    return btn->is_enabled
        && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
        && CheckCollisionPointRec(mouse, btn->bounds);
}

void ui_button_draw_and_handle(TmButton *btn, TmAppState *s,
                                TmCommandFn cmd, void *param) {
    if (!btn || !s) return;

    Vector2 mouse = GetMousePosition();
    btn->is_hovered = btn->is_enabled
                      && CheckCollisionPointRec(mouse, btn->bounds);

    draw_button_background(btn);
    draw_button_label(btn);

    if (cmd && is_clicked(btn, mouse)) {
        tm_result_t r = cmd(s, param);
        if (r != TM_OK) {
            tm_log_warn("Button command failed with code %d", r);
            ui_toast_show(s, "Action failed", RED, TM_MSG_DISPLAY_FRAMES);
        }
    }
}
