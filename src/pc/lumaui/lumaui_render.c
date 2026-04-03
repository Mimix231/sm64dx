#include "lumaui_render.h"

#include "gfx_dimensions.h"
#include "game/game_init.h"
#include "game/hud.h"
#include "game/ingame_menu.h"

#include "lumaui_core.h"
#include "lumaui_text.h"
#include "lumaui_theme.h"
#include "lumaui_widget.h"

extern Vp gViewportFullscreen;
extern ALIGNED8 u8 gd_texture_hand_open[];
extern ALIGNED8 u8 gd_texture_hand_closed[];

#define LUMAUI_CURSOR_SIZE 20
#define LUMAUI_CURSOR_HOTSPOT_X 4
#define LUMAUI_CURSOR_HOTSPOT_Y 2

static void lumaui_render_fill_rect_pixels(s32 left, s32 top, s32 right, s32 bottom,
                                           const struct LumaUIColor *color) {
    if (left > right || top > bottom) {
        return;
    }

    gDPPipeSync(gDisplayListHead++);
    gDPSetRenderMode(gDisplayListHead++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListHead++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListHead++, lumaui_theme_pack_fill_color(color));
    gDPFillRectangle(gDisplayListHead++, left, top, right, bottom);
    gDPPipeSync(gDisplayListHead++);
    gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
}

static void lumaui_render_outline(const struct LumaUIRect *rect, const struct LumaUIColor *color) {
    s32 left = lumaui_space_to_screen_x(rect->x);
    s32 top = rect->y;
    s32 right = lumaui_space_to_screen_x(rect->x + rect->w) - 1;
    s32 bottom = rect->y + rect->h - 1;

    lumaui_render_fill_rect_pixels(left, top, right, top, color);
    lumaui_render_fill_rect_pixels(left, bottom, right, bottom, color);
    lumaui_render_fill_rect_pixels(left, top, left, bottom, color);
    lumaui_render_fill_rect_pixels(right, top, right, bottom, color);
}

static void lumaui_render_shadowed_panel(const struct LumaUIRect *rect,
                                         const struct LumaUIColor *fill,
                                         const struct LumaUIColor *border) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor shadowColor = { 2, 3, 6, 255 };
    struct LumaUIRect shadow = { rect->x + 2, rect->y + 3, rect->w, rect->h };
    struct LumaUIRect inner = lumaui_space_inset_rect(rect, 1, 1);
    struct LumaUIRect topLip = { inner.x + 1, inner.y + 1, inner.w - 2, 2 };

    lumaui_render_fill_rect_pixels(lumaui_space_to_screen_x(shadow.x), shadow.y,
                                   lumaui_space_to_screen_x(shadow.x + shadow.w) - 1, shadow.y + shadow.h - 1,
                                   &shadowColor);
    lumaui_render_fill_rect_pixels(lumaui_space_to_screen_x(rect->x), rect->y,
                                   lumaui_space_to_screen_x(rect->x + rect->w) - 1, rect->y + rect->h - 1,
                                   border);
    if (inner.w > 0 && inner.h > 0) {
        lumaui_render_fill_rect_pixels(lumaui_space_to_screen_x(inner.x), inner.y,
                                       lumaui_space_to_screen_x(inner.x + inner.w) - 1, inner.y + inner.h - 1,
                                       fill);
    }
    if (topLip.w > 0 && topLip.h > 0) {
        lumaui_render_fill_rect_pixels(lumaui_space_to_screen_x(topLip.x), topLip.y,
                                       lumaui_space_to_screen_x(topLip.x + topLip.w) - 1, topLip.y + topLip.h - 1,
                                       &theme->panelBorder);
    }
}

void lumaui_render_begin(void) {
    create_dl_ortho_matrix();
    gSPViewport(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(&gViewportFullscreen));
    lumaui_space_begin_frame();
}

void lumaui_render_backdrop(void) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor topBand = { 12, 22, 43, 255 };
    const struct LumaUIColor lowerBand = { 8, 14, 28, 255 };

    lumaui_render_fill_rect_pixels(GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0), 0,
                                   GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0) - 1, SCREEN_HEIGHT - 1,
                                   &theme->backdrop);
    lumaui_render_fill_rect_pixels(GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0), 0,
                                   GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0) - 1, 30,
                                   &topBand);
    lumaui_render_fill_rect_pixels(GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0), 180,
                                   GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0) - 1, SCREEN_HEIGHT - 1,
                                   &lowerBand);
}

void lumaui_render_panel(const struct LumaUIRect *rect, const struct LumaUIColor *fill,
                         const struct LumaUIColor *border) {
    s32 left = lumaui_space_to_screen_x(rect->x);
    s32 top = rect->y;
    s32 right = lumaui_space_to_screen_x(rect->x + rect->w) - 1;
    s32 bottom = rect->y + rect->h - 1;

    lumaui_render_fill_rect_pixels(left, top, right, bottom, fill);
    lumaui_render_outline(rect, border);
}

void lumaui_render_card(const struct LumaUIRect *rect, bool selected) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor *border = selected ? &theme->accent : &theme->cardBorder;

    lumaui_render_shadowed_panel(rect, &theme->card, border);
}

void lumaui_render_button(const struct LumaUIButtonSpec *button) {
    lumaui_widget_render_button(button);
}

void lumaui_render_tab_strip(const struct LumaUIRect *rect, const char *leftLabel, const char *rightLabel) {
    lumaui_widget_render_tab_strip(rect, leftLabel, rightLabel);
}

void lumaui_render_scroll_region(const struct LumaUIRect *rect) {
    lumaui_widget_render_scroll_region(rect);
}

void lumaui_render_badge(const struct LumaUIRect *rect, const char *label) {
    lumaui_widget_render_badge(rect, label);
}

void lumaui_render_icon_slot(const struct LumaUIRect *rect, const char *label, bool selected) {
    lumaui_widget_render_icon_slot(rect, label, selected);
}

void lumaui_render_action_bar(const char *leftText, const char *rightText) {
    lumaui_widget_render_action_bar(leftText, rightText);
}

void lumaui_render_text(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    lumaui_text_draw(x, y, text, color);
}

void lumaui_render_text_centered(s16 centerX, s16 y, const char *text, const struct LumaUIColor *color) {
    lumaui_text_draw_centered(centerX, y, text, color);
}

void lumaui_render_text_block(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    lumaui_text_draw_block(x, y, text, color);
}

void lumaui_render_text_block_wrapped(s16 x, s16 y, s16 maxWidth, const char *text,
                                      const struct LumaUIColor *color) {
    lumaui_text_draw_block_wrapped(x, y, maxWidth, text, color);
}

void lumaui_render_cursor(s16 x, s16 y, bool pressed) {
    s32 left = lumaui_space_to_screen_x(x - LUMAUI_CURSOR_HOTSPOT_X);
    s32 bottom = SCREEN_HEIGHT - (y - LUMAUI_CURSOR_HOTSPOT_Y);
    const Texture *texture = pressed ? gd_texture_hand_closed : gd_texture_hand_open;

    render_hud_icon(NULL, texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32,
                    left, bottom, LUMAUI_CURSOR_SIZE, LUMAUI_CURSOR_SIZE,
                    0, 0, 32, 32);
}

void lumaui_render_modal(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect backdrop = { 48, 64, 224, 112 };
    struct LumaUIRect badge = { 60, 78, 52, 18 };
    struct LumaUIRect bodyRect = { 60, 102, 200, 56 };

    if (!state->modal.active) {
        return;
    }

    lumaui_render_shadowed_panel(&backdrop, &theme->panel, &theme->panelBorder);
    lumaui_render_badge(&badge, "Modal");
    lumaui_render_text(backdrop.x + 12, backdrop.y + 36, state->modal.title, &theme->text);
    lumaui_render_push_clip(&bodyRect);
    lumaui_render_text_block_wrapped(bodyRect.x, bodyRect.y, bodyRect.w, state->modal.body, &theme->mutedText);
    lumaui_render_pop_clip();
}

bool lumaui_render_point_in_rect(s32 x, s32 y, const struct LumaUIRect *rect) {
    return lumaui_space_point_in_rect(x, y, rect);
}

void lumaui_render_push_clip(const struct LumaUIRect *rect) {
    lumaui_space_push_clip(rect);
}

void lumaui_render_pop_clip(void) {
    lumaui_space_pop_clip();
}
