#include "mxui_font.h"

#include "sm64.h"

#include "pc/configfile.h"
#include "game/segment2.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"

#include "mxui_render.h"
#include "mxui_unicode.h"

#include "engine/math_util.h"

static enum MxuiFontType sMxuiCurrentFont = FONT_NORMAL;

static void mxui_font_normal_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    if (index & 0x010000) {
        index &= ~0x010000;
        u32 tx = index % 64;
        u32 ty = index / 64;
        extern ALIGNED8 const Texture texture_font_jp[];
        mxui_render_texture_tile_raw(texture_font_jp, 512, 1024, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 8, ty * 16, 8, 16, false, true);
    } else {
        u32 tx = index % 32;
        u32 ty = index / 32;
        extern ALIGNED8 const Texture texture_font_normal[];
        mxui_render_texture_tile_raw(texture_font_normal, 256, 128, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 8, ty * 16, 8, 16, false, true);
    }
}

static f32 mxui_font_normal_char_width(char* c) {
    if (*c == ' ') { return configExCoopTheme ? 6 / 32.0f : 0.30f; }
    extern const f32 font_normal_widths[];
    return mxui_unicode_get_sprite_width(c, font_normal_widths, 32.0f);
}

static const struct MxuiFont sMxuiFontNormal = {
    .charWidth = 0.5f,
    .charHeight = 1.0f,
    .lineHeight = 0.8125f,
    .xOffset = 0.0f,
    .yOffset = 0.0f,
    .defaultFontScale = 32.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_normal_render_char,
    .char_width = mxui_font_normal_char_width,
};

static void mxui_font_title_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    if ((u8)*c < '!' || (u8)*c > '~' + 1) {
        char tmp[2] = { 0 };
        tmp[0] = mxui_unicode_get_base_char(c);
        index = mxui_unicode_get_sprite_index(tmp);
    }

    u32 tx = index % 16;
    u32 ty = index / 16;
    extern ALIGNED8 const Texture texture_font_title[];
    mxui_render_texture_tile_raw(texture_font_title, 1024, 512, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 64, ty * 64, 64, 64, false, true);
}

static f32 mxui_font_title_char_width(char* text) {
    char c = *text;
    if (c == ' ') { return 0.30f; }
    c = mxui_unicode_get_base_char(text);
    extern const f32 font_title_widths[];
    return font_title_widths[(u8)c - '!'] * (configExCoopTheme ? 1.0f : 1.1f);
}

static const struct MxuiFont sMxuiFontTitle = {
    .charWidth = 1.0f,
    .charHeight = 0.9f,
    .lineHeight = 0.7f,
    .xOffset = 0.0f,
    .yOffset = 0.0f,
    .defaultFontScale = 64.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_title_render_char,
    .char_width = mxui_font_title_char_width,
};

static u8 mxui_font_hud_index(char c) {
    if ((u8)c < ' ' || (u8)c > 127) { return 41; }

    switch (c) {
        case '!':  return 36;
        case '#':  return 37;
        case '?':  return 38;
        case '&':  return 39;
        case '%':  return 40;
        case '@':  return 41;
        case '$':  return 42;
        case ',':  return 43;
        case '*':  return 44;
        case '.':  return 45;
        case '^':  return 46;
        case '\'': return 47;
        case '"':  return 48;
        case '/':  return 49;
        case '-':  return 50;
        case '~':  return 51;
        case '+':  return 52;
    }

    if (c >= '0' && c <= '9') { return 0  + c - '0'; }
    if (c >= 'a' && c <= 'z') { return 10 + c - 'a'; }
    if (c >= 'A' && c <= 'Z') { return 10 + c - 'A'; }
    if (c >= 58 || main_hud_lut[(int)c] == NULL) { return 41; }
    return c;
}

static void mxui_font_hud_render_char(char* text) {
    char c = *text;
    if (c == ' ') { return; }
    c = mxui_unicode_get_base_char(text);
    u8 index = mxui_font_hud_index(c);
    mxui_render_texture_raw(main_hud_lut[index], 16, 16, G_IM_FMT_RGBA, G_IM_SIZ_16b, false);
}

static f32 mxui_font_hud_char_width(UNUSED char* text) {
    return 0.75f;
}

static const struct MxuiFont sMxuiFontHud = {
    .charWidth = 1.0f,
    .charHeight = 0.9f,
    .lineHeight = 0.7f,
    .xOffset = 0.0f,
    .yOffset = 0.0f,
    .defaultFontScale = 16.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_hud_render_char,
    .char_width = mxui_font_hud_char_width,
};

static void mxui_font_aliased_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    if (index & 0x010000) {
        index &= ~0x010000;
        u32 tx = index % 64;
        u32 ty = index / 64;
        extern ALIGNED8 const Texture texture_font_jp_aliased[];
        mxui_render_texture_tile_raw(texture_font_jp_aliased, 1024, 2048, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 16, ty * 32, 16, 32, false, true);
    } else {
        u32 tx = index % 32;
        u32 ty = index / 32;
        extern ALIGNED8 const Texture texture_font_aliased[];
        mxui_render_texture_tile_raw(texture_font_aliased, 512, 256, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 16, ty * 32, 16, 32, false, true);
    }
}

static f32 mxui_font_aliased_char_width(char* c) {
    if (*c == ' ') { return 6 / 32.0f; }
    extern const f32 font_aliased_widths[];
    return mxui_unicode_get_sprite_width(c, font_aliased_widths, 1.0f) / 32.0f;
}

static const struct MxuiFont sMxuiFontAliased = {
    .charWidth = 0.5f,
    .charHeight = 1.0f,
    .xOffset = 0.0f,
    .yOffset = 0.0f,
    .lineHeight = 0.8125f,
    .defaultFontScale = 32.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_aliased_render_char,
    .char_width = mxui_font_aliased_char_width,
};

static void mxui_font_custom_hud_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    u32 tx = index % 16;
    u32 ty = index / 16;
    extern ALIGNED8 const Texture texture_font_hud[];
    mxui_render_texture_tile_raw(texture_font_hud, 512, 512, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 32, ty * 32, 32, 32, false, true);
}

static void mxui_font_custom_hud_recolor_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    u32 tx = index % 16;
    u32 ty = index / 16;
    extern ALIGNED8 const Texture texture_font_hud_recolor[];
    mxui_render_texture_tile_raw(texture_font_hud_recolor, 512, 512, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 32, ty * 32, 32, 32, false, true);
}

static f32 mxui_font_custom_hud_char_width(char* text) {
    char c = *text;
    if (c == ' ') { return 0.3750f; }
    c = mxui_unicode_get_base_char(text);
    extern const f32 font_hud_widths[];
    return font_hud_widths[(u8)c - '!'];
}

static const struct MxuiFont sMxuiFontCustomHud = {
    .charWidth = 1.0f,
    .charHeight = 0.9f,
    .lineHeight = 0.7f,
    .xOffset = -0.25f,
    .yOffset = -10.25f,
    .defaultFontScale = 32.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_custom_hud_render_char,
    .char_width = mxui_font_custom_hud_char_width,
};

static const struct MxuiFont sMxuiFontCustomHudRecolor = {
    .charWidth = 1.0f,
    .charHeight = 0.9f,
    .lineHeight = 0.7f,
    .xOffset = -0.25f,
    .yOffset = -10.25f,
    .defaultFontScale = 32.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_custom_hud_recolor_render_char,
    .char_width = mxui_font_custom_hud_char_width,
};

static void mxui_font_special_render_char(char* c) {
    if (*c == ' ') { return; }

    u32 index = mxui_unicode_get_sprite_index(c);
    if (index & 0x010000) {
        index &= ~0x010000;
        u32 tx = index % 64;
        u32 ty = index / 64;
        extern ALIGNED8 const Texture texture_font_jp[];
        mxui_render_texture_tile_raw(texture_font_jp, 512, 1024, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 8, ty * 16, 8, 16, false, true);
    } else {
        u32 tx = index % 32;
        u32 ty = index / 32;
        extern ALIGNED8 const Texture texture_font_special[];
        mxui_render_texture_tile_raw(texture_font_special, 256, 128, G_IM_FMT_RGBA, G_IM_SIZ_32b, tx * 8, ty * 16, 8, 16, false, true);
    }
}

static f32 mxui_font_special_char_width(char* c) {
    if (*c == ' ') { return 0.5f; }
    extern const f32 font_special_widths[];
    return mxui_unicode_get_sprite_width(c, font_special_widths, 32.0f);
}

static const struct MxuiFont sMxuiFontSpecial = {
    .charWidth = 0.5f,
    .charHeight = 1.0f,
    .lineHeight = 0.8125f,
    .xOffset = 0.0f,
    .yOffset = 0.0f,
    .defaultFontScale = 32.0f,
    .textBeginDisplayList = NULL,
    .render_char = mxui_font_special_render_char,
    .char_width = mxui_font_special_char_width,
};

const struct MxuiFont* gMxuiFonts[] = {
    &sMxuiFontNormal,
    &sMxuiFontTitle,
    &sMxuiFontHud,
    &sMxuiFontAliased,
    &sMxuiFontCustomHud,
    &sMxuiFontCustomHudRecolor,
    &sMxuiFontSpecial,
};

enum MxuiFontType mxui_font_get_current(void) {
    return sMxuiCurrentFont;
}

void mxui_font_set_current(enum MxuiFontType font) {
    if (font < 0 || font >= FONT_COUNT) {
        return;
    }
    sMxuiCurrentFont = font;
}

f32 mxui_font_measure_text_raw(const char* text) {
    if (text == NULL) {
        return 0.0f;
    }

    const struct MxuiFont* font = gMxuiFonts[sMxuiCurrentFont];
    f32 width = 0.0f;
    char* cursor = (char*)text;
    while (*cursor != '\0') {
        if (*cursor != '\n') {
            width += font->char_width(cursor);
        }
        cursor = mxui_unicode_next_char(cursor);
    }
    return width * font->defaultFontScale;
}

void mxui_font_print_text_raw(const char* text, f32 x, f32 y, f32 scale) {
    if (text == NULL) {
        return;
    }

    const struct MxuiFont* font = gMxuiFonts[sMxuiCurrentFont];
    f32 translatedX = x + (font->xOffset * scale);
    f32 translatedY = y + (font->yOffset * scale);
    f32 translatedScale = font->defaultFontScale * scale;

    if (font->textBeginDisplayList != NULL) {
        gSPDisplayList(gDisplayListHead++, font->textBeginDisplayList);
    }

    mxui_render_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, 0.0f);
    mxui_render_size_translate(&translatedScale);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, translatedScale, translatedScale, 1.0f);

    char* cursor = (char*)text;
    mxui_render_reset_texture_clipping();
    while (*cursor != '\0') {
        if (*cursor != '\n') {
            f32 charWidth = font->char_width(cursor);
            font->render_char(cursor);
            create_dl_translation_matrix(MXUI_MTX_NOPUSH, charWidth, 0.0f, 0.0f);
        }
        cursor = mxui_unicode_next_char(cursor);
    }
    mxui_render_reset_texture_clipping();
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

f32 mxui_font_line_height_raw(enum MxuiFontType font) {
    return gMxuiFonts[font]->lineHeight;
}

f32 mxui_font_default_scale_raw(enum MxuiFontType font) {
    return gMxuiFonts[font]->defaultFontScale;
}
