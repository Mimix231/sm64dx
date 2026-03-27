#pragma once

#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include "types.h"

enum MxuiFontType {
    FONT_NORMAL,
    FONT_MENU,
    FONT_HUD,
    FONT_ALIASED,
    FONT_CUSTOM_HUD,
    FONT_RECOLOR_HUD,
    FONT_SPECIAL,
    FONT_COUNT,
};

struct MxuiFont {
    f32 charWidth;
    f32 charHeight;
    f32 lineHeight;
    f32 xOffset;
    f32 yOffset;
    f32 defaultFontScale;
    const Gfx* textBeginDisplayList;
    void (*render_char)(char*);
    f32 (*char_width)(char*);
};

extern const struct MxuiFont* gMxuiFonts[];

enum MxuiFontType mxui_font_get_current(void);
void mxui_font_set_current(enum MxuiFontType font);
f32 mxui_font_measure_text_raw(const char* text);
void mxui_font_print_text_raw(const char* text, f32 x, f32 y, f32 scale);
f32 mxui_font_line_height_raw(enum MxuiFontType font);
f32 mxui_font_default_scale_raw(enum MxuiFontType font);
