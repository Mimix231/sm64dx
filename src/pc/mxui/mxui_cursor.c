#include "mxui_cursor.h"

#include "sm64.h"
#include "game/game_init.h"

#include "pc/configfile.h"
#include "pc/controller/controller_api.h"
#include "pc/controller/controller_mouse.h"
#include "pc/pc_main.h"
#include "pc/utils/misc.h"

#include "gfx_dimensions.h"

#include "mxui_internal.h"
#include "mxui_render.h"

static f32 sMxuiCursorX = 0.0f;
static f32 sMxuiCursorY = 0.0f;
static f32 sMxuiPrevCursorX = 0.0f;
static f32 sMxuiPrevCursorY = 0.0f;
static bool sMxuiCursorVisible = false;

static void mxui_cursor_snap_to_focus(void);

static void mxui_cursor_draw_piece(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    mxui_render_set_color(r, g, b, a);
    mxui_render_rect(x, y, w, h);
}

static void mxui_cursor_clamp_position(void) {
    f32 screenW = mxui_render_screen_width();
    f32 screenH = mxui_render_screen_height();

    if (screenW <= 1.0f || screenH <= 1.0f) {
        return;
    }

    if (sMxuiCursorX < 0.0f || sMxuiCursorX > screenW || sMxuiCursorY < 0.0f || sMxuiCursorY > screenH) {
        sMxuiCursorX = screenW * 0.5f;
        sMxuiCursorY = screenH * 0.5f;
    }

    if (sMxuiPrevCursorX < 0.0f || sMxuiPrevCursorX > screenW || sMxuiPrevCursorY < 0.0f || sMxuiPrevCursorY > screenH) {
        sMxuiPrevCursorX = sMxuiCursorX;
        sMxuiPrevCursorY = sMxuiCursorY;
    }

    if ((sMxuiCursorX <= 1.0f && sMxuiCursorY <= 1.0f) && sMxui.focusedRectValid) {
        mxui_cursor_snap_to_focus();
        sMxuiPrevCursorX = sMxuiCursorX;
        sMxuiPrevCursorY = sMxuiCursorY;
    }
}

static void mxui_cursor_snap_to_focus(void) {
    if (!sMxui.focusedRectValid) {
        return;
    }

    sMxuiCursorX = sMxui.focusedRect.x + MIN(sMxui.focusedRect.w * 0.18f, 28.0f);
    sMxuiCursorY = sMxui.focusedRect.y + MIN(sMxui.focusedRect.h * 0.50f, 24.0f);
}

static void mxui_cursor_update_position(void) {
    f32 screenW = mxui_render_screen_width();
    f32 screenH = mxui_render_screen_height();
    f32 mouseX = mxui_render_mouse_x();
    f32 mouseY = mxui_render_mouse_y();
    bool mouseValid = screenW > 1.0f && screenH > 1.0f
        && mouseX >= 0.0f && mouseX <= screenW
        && mouseY >= 0.0f && mouseY <= screenH;

    sMxuiPrevCursorX = sMxuiCursorX;
    sMxuiPrevCursorY = sMxuiCursorY;

    if (!mouseValid || ((mouseX <= 1.0f && mouseY <= 1.0f) && sMxui.focusedRectValid)) {
        mxui_cursor_snap_to_focus();
    } else {
        sMxuiCursorX = mouseX;
        sMxuiCursorY = mouseY;
    }

    mxui_cursor_clamp_position();
}

static void mxui_cursor_render_cursor(void) {
    if (!sMxuiCursorVisible) {
        return;
    }

    const bool pressed = ((gPlayer1Controller->buttonDown & A_BUTTON) != 0) || ((mouse_window_buttons & MOUSE_BUTTON_1) != 0);
    const f32 x = sMxuiCursorX - 13.0f;
    const f32 y = sMxuiCursorY - 12.0f;
    const f32 pressOffset = pressed ? 1.0f : 0.0f;

    mxui_render_reset_scissor();
    mxui_render_reset_texture_clipping();

    /* glow */
    mxui_cursor_draw_piece(x + 1.0f + pressOffset, y + 1.0f + pressOffset, 11.0f, 23.0f, 210, 236, 255, 72);
    mxui_cursor_draw_piece(x + 10.0f + pressOffset, y + 12.0f + pressOffset, 12.0f, 10.0f, 210, 236, 255, 72);
    mxui_cursor_draw_piece(x + 2.0f + pressOffset, y + 22.0f + pressOffset, 13.0f, 8.0f, 210, 236, 255, 72);

    /* shadow */
    mxui_cursor_draw_piece(x + 5.0f + pressOffset, y + 4.0f + pressOffset, 8.0f, 20.0f, 0, 0, 0, 88);
    mxui_cursor_draw_piece(x + 4.0f + pressOffset, y + 20.0f + pressOffset, 16.0f, 9.0f, 0, 0, 0, 88);
    mxui_cursor_draw_piece(x + 14.0f + pressOffset, y + 13.0f + pressOffset, 7.0f, 10.0f, 0, 0, 0, 88);

    /* outline */
    mxui_cursor_draw_piece(x + 1.0f + pressOffset, y + pressOffset, 8.0f, 21.0f, 0, 0, 0, 255);
    mxui_cursor_draw_piece(x + pressOffset, y + 20.0f + pressOffset, 17.0f, 9.0f, 0, 0, 0, 255);
    mxui_cursor_draw_piece(x + 10.0f + pressOffset, y + 12.0f + pressOffset, 8.0f, 11.0f, 0, 0, 0, 255);
    mxui_cursor_draw_piece(x + 3.0f + pressOffset, y + 27.0f + pressOffset, 10.0f, 4.0f, 0, 0, 0, 255);

    /* glove fill */
    mxui_cursor_draw_piece(x + 2.0f + pressOffset, y + 1.0f + pressOffset, 6.0f, 19.0f, 252, 252, 255, 255);
    mxui_cursor_draw_piece(x + 1.0f + pressOffset, y + 21.0f + pressOffset, 15.0f, 7.0f, 247, 247, 252, 255);
    mxui_cursor_draw_piece(x + 11.0f + pressOffset, y + 13.0f + pressOffset, 6.0f, 8.0f, 245, 245, 251, 255);
    mxui_cursor_draw_piece(x + 3.0f + pressOffset, y + 28.0f + pressOffset, 8.0f, 2.0f, 214, 226, 255, 255);

    /* highlights */
    mxui_cursor_draw_piece(x + 2.0f + pressOffset, y + 2.0f + pressOffset, 2.0f, 16.0f, 255, 255, 255, 220);
    mxui_cursor_draw_piece(x + 2.0f + pressOffset, y + 22.0f + pressOffset, 4.0f, 2.0f, 255, 255, 255, 192);

    mxui_render_reset_texture_clipping();
}

void mxui_cursor_set_visible(bool visible) {
    sMxuiCursorVisible = visible;

    if (visible) {
        controller_mouse_leave_relative();
        controller_mouse_read_window();
        sMxuiCursorX = mxui_render_mouse_x();
        sMxuiCursorY = mxui_render_mouse_y();
        if ((sMxuiCursorX <= 1.0f && sMxuiCursorY <= 1.0f) || !sMxui.focusedRectValid) {
            mxui_cursor_snap_to_focus();
        }
        sMxuiPrevCursorX = sMxuiCursorX;
        sMxuiPrevCursorY = sMxuiCursorY;

        f32 screenW = mxui_render_screen_width();
        f32 screenH = mxui_render_screen_height();
        if (sMxuiCursorX < 0.0f || sMxuiCursorX > screenW || sMxuiCursorY < 0.0f || sMxuiCursorY > screenH) {
            sMxuiCursorX = screenW * 0.5f;
            sMxuiCursorY = screenH * 0.5f;
            sMxuiPrevCursorX = sMxuiCursorX;
            sMxuiPrevCursorY = sMxuiCursorY;
        }
    }

    if (wm_api != NULL) {
        if (configWindow.fullscreen) {
            wm_api->set_cursor_visible(false);
        } else {
            wm_api->set_cursor_visible(!visible);
        }
    }
}

void mxui_cursor_interp(void) {
    if (!sMxuiCursorVisible && sMxui.active && sMxui.depth > 0) {
        mxui_cursor_set_visible(true);
    }
    mxui_cursor_update_position();
    if (sMxuiCursorVisible) {
        mxui_cursor_render_cursor();
    }
}

void mxui_cursor_update(void) {
    if (!sMxuiCursorVisible && sMxui.active && sMxui.depth > 0) {
        mxui_cursor_set_visible(true);
    }
    if (!sMxuiCursorVisible) {
        return;
    }

    mxui_cursor_update_position();
    mxui_cursor_render_cursor();
}
