#include "lumaui_theme.h"

#include <PR/gbi.h>

static const struct LumaUITheme sLumaUITheme = {
    .backdrop      = {   5,   9,  18, 255 },
    .panel         = {  20,  29,  49, 255 },
    .panelBorder   = { 226, 190, 102, 255 },
    .card          = {  11,  17,  29, 255 },
    .cardBorder    = {  82, 112, 170, 255 },
    .primary       = { 244, 202,  78, 255 },
    .primaryBorder = { 255, 243, 176, 255 },
    .accent        = { 114, 201, 255, 255 },
    .text          = { 255, 249, 232, 255 },
    .mutedText     = { 187, 210, 232, 255 },
    .badge         = {  31,  54,  98, 255 },
    .badgeBorder   = { 154, 211, 255, 255 },
    .cursor        = { 255, 255, 255, 255 },
    .cursorShadow  = {  12,  14,  19, 255 },
};

const struct LumaUITheme *lumaui_theme_get(void) {
    return &sLumaUITheme;
}

u32 lumaui_theme_pack_fill_color(const struct LumaUIColor *color) {
    u16 packed = GPACK_RGBA5551(color->r, color->g, color->b, color->a > 0);
    return ((u32) packed << 16) | packed;
}
