#include "lumaui_theme.h"

#include <PR/gbi.h>

static const struct LumaUITheme sLumaUITheme = {
    .backdrop      = {  10,  16,  28, 255 },
    .panel         = {  22,  31,  52, 255 },
    .panelBorder   = { 104, 150, 255, 255 },
    .card          = {  15,  21,  38, 255 },
    .cardBorder    = {  72, 102, 170, 255 },
    .primary       = { 240, 198,  76, 255 },
    .primaryBorder = { 255, 234, 148, 255 },
    .accent        = { 154, 198, 255, 255 },
    .text          = { 255, 255, 255, 255 },
    .mutedText     = { 181, 197, 230, 255 },
    .badge         = {  44,  68, 110, 255 },
    .badgeBorder   = { 120, 174, 255, 255 },
    .cursor        = { 255, 255, 255, 255 },
    .cursorShadow  = {  24,  24,  24, 255 },
};

const struct LumaUITheme *lumaui_theme_get(void) {
    return &sLumaUITheme;
}

u32 lumaui_theme_pack_fill_color(const struct LumaUIColor *color) {
    u16 packed = GPACK_RGBA5551(color->r, color->g, color->b, color->a > 0);
    return ((u32) packed << 16) | packed;
}
