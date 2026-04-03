#ifndef LUMAUI_THEME_H
#define LUMAUI_THEME_H

#include <PR/ultratypes.h>

struct LumaUIColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct LumaUITheme {
    struct LumaUIColor backdrop;
    struct LumaUIColor panel;
    struct LumaUIColor panelBorder;
    struct LumaUIColor card;
    struct LumaUIColor cardBorder;
    struct LumaUIColor primary;
    struct LumaUIColor primaryBorder;
    struct LumaUIColor accent;
    struct LumaUIColor text;
    struct LumaUIColor mutedText;
    struct LumaUIColor badge;
    struct LumaUIColor badgeBorder;
    struct LumaUIColor cursor;
    struct LumaUIColor cursorShadow;
};

const struct LumaUITheme *lumaui_theme_get(void);
u32 lumaui_theme_pack_fill_color(const struct LumaUIColor *color);

#endif
