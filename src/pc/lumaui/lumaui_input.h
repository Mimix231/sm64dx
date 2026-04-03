#ifndef LUMAUI_INPUT_H
#define LUMAUI_INPUT_H

#include <stdbool.h>
#include <PR/ultratypes.h>

struct LumaUIInputState {
    s32 cursorX;
    s32 cursorY;
    bool cursorVisible;
    bool pointerDown;
    bool pointerPressed;
    bool confirmPressed;
    bool backPressed;
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;
};

struct LumaUIState;

void lumaui_input_update(struct LumaUIState *state);

#endif
