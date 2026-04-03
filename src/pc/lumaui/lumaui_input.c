#include "lumaui_input.h"

#include "lumaui_core.h"
#include "lumaui_space.h"

#include "sm64.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "pc/pc_main.h"

#include "gfx_dimensions.h"

#if defined(HAVE_SDL3)
#include <SDL3/SDL.h>
#elif defined(HAVE_SDL2)
#include <SDL2/SDL.h>
#endif

static bool lumaui_button_pressed(u16 mask) {
    return gPlayer1Controller != NULL && (gPlayer1Controller->buttonPressed & mask) != 0;
}

static bool lumaui_input_poll_pointer(struct LumaUIInputState *input, bool *relativeMode) {
#if defined(HAVE_SDL3)
    SDL_Window *window = SDL_GL_GetCurrentWindow();
    float rawX = 0.0f;
    float rawY = 0.0f;
    SDL_MouseButtonFlags buttons = 0;

    if (window == NULL) {
        return false;
    }

    buttons = SDL_GetMouseState(&rawX, &rawY);
    input->cursorX = (s32) rawX - gfx_current_dimensions.x_adjust_4by3;
    input->cursorY = (s32) rawY;
    input->pointerDown = (buttons & SDL_BUTTON_LMASK) != 0;
    *relativeMode = SDL_GetWindowRelativeMouseMode(window);
    return true;
#elif defined(HAVE_SDL2)
    int rawX = 0;
    int rawY = 0;
    Uint32 buttons = SDL_GetMouseState(&rawX, &rawY);

    input->cursorX = rawX - gfx_current_dimensions.x_adjust_4by3;
    input->cursorY = rawY;
    input->pointerDown = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    *relativeMode = SDL_GetRelativeMouseMode() == SDL_TRUE;
    return true;
#else
    (void) input;
    *relativeMode = false;
    return false;
#endif
}

void lumaui_input_update(struct LumaUIState *state) {
    struct LumaUIInputState *input = &state->input;
    bool previousPointerDown = input->pointerDown;
    bool relativeMode = false;
    bool hasPointer = lumaui_input_poll_pointer(input, &relativeMode);

    if (!hasPointer) {
        input->cursorX = 0;
        input->cursorY = 0;
        input->pointerDown = false;
    }

    if (input->cursorX < 0) {
        input->cursorX = 0;
    }
    if (input->cursorY < 0) {
        input->cursorY = 0;
    }
    if (input->cursorX >= LUMAUI_LOGICAL_WIDTH) {
        input->cursorX = LUMAUI_LOGICAL_WIDTH - 1;
    }
    if (input->cursorY >= LUMAUI_LOGICAL_HEIGHT) {
        input->cursorY = LUMAUI_LOGICAL_HEIGHT - 1;
    }

    input->pointerPressed = input->pointerDown && !previousPointerDown;
    input->confirmPressed = input->pointerPressed || lumaui_button_pressed(A_BUTTON | START_BUTTON);
    input->backPressed = lumaui_button_pressed(B_BUTTON);
    input->upPressed = lumaui_button_pressed(U_JPAD);
    input->downPressed = lumaui_button_pressed(D_JPAD);
    input->leftPressed = lumaui_button_pressed(L_JPAD);
    input->rightPressed = lumaui_button_pressed(R_JPAD);
    input->cursorVisible = hasPointer && !relativeMode && wm_api != NULL && wm_api->has_focus != NULL && wm_api->has_focus();

    if (lumaui_core_has_active_scene() && wm_api != NULL && wm_api->set_cursor_visible != NULL) {
        wm_api->set_cursor_visible(!input->cursorVisible);
    }
}
