#include "lumaui_input.h"

#include "lumaui_core.h"

#include "sm64.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "pc/controller/controller_mouse.h"
#include "pc/pc_main.h"

static bool lumaui_button_pressed(u16 mask) {
    return gPlayer1Controller != NULL && (gPlayer1Controller->buttonPressed & mask) != 0;
}

void lumaui_input_update(struct LumaUIState *state) {
    struct LumaUIInputState *input = &state->input;

    input->cursorX = mouse_window_x;
    input->cursorY = mouse_window_y;
    if (input->cursorX < 0) { input->cursorX = 0; }
    if (input->cursorY < 0) { input->cursorY = 0; }
    if (input->cursorX >= SCREEN_WIDTH) { input->cursorX = SCREEN_WIDTH - 1; }
    if (input->cursorY >= SCREEN_HEIGHT) { input->cursorY = SCREEN_HEIGHT - 1; }

    input->pointerDown = (mouse_window_buttons & L_MOUSE_BUTTON) != 0;
    input->pointerPressed = (mouse_window_buttons_pressed & L_MOUSE_BUTTON) != 0;
    input->confirmPressed = input->pointerPressed || lumaui_button_pressed(A_BUTTON | START_BUTTON);
    input->backPressed = lumaui_button_pressed(B_BUTTON);
    input->upPressed = lumaui_button_pressed(U_JPAD);
    input->downPressed = lumaui_button_pressed(D_JPAD);
    input->leftPressed = lumaui_button_pressed(L_JPAD);
    input->rightPressed = lumaui_button_pressed(R_JPAD);
    input->cursorVisible = mouse_init_ok && !mouse_relative_enabled && WAPI.has_focus();

    if (lumaui_core_has_active_scene() && wm_api != NULL && wm_api->set_cursor_visible != NULL) {
        wm_api->set_cursor_visible(!input->cursorVisible);
    }
}
