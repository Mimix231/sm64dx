#include "lumaui_render.h"

#include <string.h>

#include "gfx_dimensions.h"
#include "game/ingame_menu.h"
#include "game/print.h"
#include "game/game_init.h"
#include "game/segment2.h"

#include "lumaui_core.h"
#include "lumaui_theme.h"

extern Vp gViewportFullscreen;

static s32 lumaui_to_screen_x(s32 x) {
    return GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(x);
}

static s32 lumaui_safe_left(void) {
    return GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0);
}

static s32 lumaui_safe_right(void) {
    return GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0);
}

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
    s32 left = lumaui_to_screen_x(rect->x);
    s32 top = rect->y;
    s32 right = lumaui_to_screen_x(rect->x + rect->w) - 1;
    s32 bottom = rect->y + rect->h - 1;
    lumaui_render_fill_rect_pixels(left, top, right, top, color);
    lumaui_render_fill_rect_pixels(left, bottom, right, bottom, color);
    lumaui_render_fill_rect_pixels(left, top, left, bottom, color);
    lumaui_render_fill_rect_pixels(right, top, right, bottom, color);
}

static void lumaui_render_text_internal(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, color->r, color->g, color->b, 255);
    print_generic_ascii_string(x, y, text);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

void lumaui_render_begin(void) {
    create_dl_ortho_matrix();
    gSPViewport(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(&gViewportFullscreen));
    gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void lumaui_render_backdrop(void) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    lumaui_render_fill_rect_pixels(lumaui_safe_left(), 0, lumaui_safe_right() - 1, SCREEN_HEIGHT - 1, &theme->backdrop);
}

void lumaui_render_panel(const struct LumaUIRect *rect, const struct LumaUIColor *fill,
                         const struct LumaUIColor *border) {
    s32 left = lumaui_to_screen_x(rect->x);
    s32 top = rect->y;
    s32 right = lumaui_to_screen_x(rect->x + rect->w) - 1;
    s32 bottom = rect->y + rect->h - 1;

    lumaui_render_fill_rect_pixels(left, top, right, bottom, fill);
    lumaui_render_outline(rect, border);
}

void lumaui_render_card(const struct LumaUIRect *rect, bool selected) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor *border = selected ? &theme->accent : &theme->cardBorder;
    lumaui_render_panel(rect, &theme->card, border);
}

void lumaui_render_button(const struct LumaUIButtonSpec *button) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor *fill = button->primary ? &theme->primary : &theme->card;
    const struct LumaUIColor *border = button->primary ? &theme->primaryBorder : &theme->cardBorder;
    const struct LumaUIColor *textColor = button->primary ? &theme->card : &theme->text;
    struct LumaUIRect inner = button->rect;

    if (button->selected || button->hovered) {
        border = &theme->accent;
    }

    lumaui_render_panel(&button->rect, fill, border);
    if (button->selected || button->hovered) {
        inner.x += 2;
        inner.y += 2;
        inner.w -= 4;
        inner.h -= 4;
        if (inner.w > 0 && inner.h > 0) {
            lumaui_render_outline(&inner, border);
        }
    }

    lumaui_render_text_centered(button->rect.x + (button->rect.w / 2),
                                button->rect.y + (button->rect.h / 2) + 5,
                                button->label, textColor);
}

void lumaui_render_tab_strip(const struct LumaUIRect *rect, const char *leftLabel, const char *rightLabel) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect left = *rect;
    struct LumaUIRect right = *rect;

    left.w /= 2;
    right.x += left.w;
    right.w -= left.w;

    lumaui_render_panel(&left, &theme->panel, &theme->badgeBorder);
    lumaui_render_panel(&right, &theme->card, &theme->cardBorder);
    lumaui_render_text_centered(left.x + (left.w / 2), left.y + (left.h / 2) + 4, leftLabel, &theme->text);
    lumaui_render_text_centered(right.x + (right.w / 2), right.y + (right.h / 2) + 4, rightLabel, &theme->mutedText);
}

void lumaui_render_scroll_region(const struct LumaUIRect *rect) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect thumb = { rect->x + rect->w - 8, rect->y + 8, 4, 18 };

    lumaui_render_panel(rect, &theme->card, &theme->cardBorder);
    lumaui_render_fill_rect_pixels(lumaui_to_screen_x(thumb.x), thumb.y,
                                   lumaui_to_screen_x(thumb.x + thumb.w) - 1, thumb.y + thumb.h - 1,
                                   &theme->accent);
}

void lumaui_render_badge(const struct LumaUIRect *rect, const char *label) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    lumaui_render_panel(rect, &theme->badge, &theme->badgeBorder);
    lumaui_render_text_centered(rect->x + (rect->w / 2), rect->y + (rect->h / 2) + 4, label, &theme->text);
}

void lumaui_render_icon_slot(const struct LumaUIRect *rect, const char *label, bool selected) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect icon = { rect->x + 4, rect->y + 4, 18, rect->h - 8 };
    struct LumaUIRect labelRect = { rect->x + 26, rect->y + 4, rect->w - 30, rect->h - 8 };
    const struct LumaUIColor *iconColor = selected ? &theme->primary : &theme->accent;

    lumaui_render_panel(rect, &theme->card, selected ? &theme->accent : &theme->cardBorder);
    lumaui_render_panel(&icon, iconColor, &theme->panelBorder);
    lumaui_render_text(labelRect.x, labelRect.y + 12, label, &theme->text);
}

void lumaui_render_action_bar(const char *leftText, const char *rightText) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect bar = { 16, 220, 288, 16 };

    lumaui_render_panel(&bar, &theme->panel, &theme->panelBorder);
    lumaui_render_text(bar.x + 6, bar.y + 11, leftText, &theme->text);
    if (rightText != NULL && rightText[0] != '\0') {
        s16 width = (s16) get_generic_ascii_string_width((const char *) rightText);
        lumaui_render_text(bar.x + bar.w - width - 8, bar.y + 11, rightText, &theme->mutedText);
    }
}

void lumaui_render_text(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    if (text == NULL || text[0] == '\0') {
        return;
    }
    lumaui_render_text_internal(x, y, text, color);
}

void lumaui_render_text_centered(s16 centerX, s16 y, const char *text, const struct LumaUIColor *color) {
    s16 width = (s16) get_generic_ascii_string_width((const char *) text);
    lumaui_render_text_internal(centerX - (width / 2), y, text, color);
}

void lumaui_render_text_block(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    char line[128] = { 0 };
    int lineLen = 0;

    if (text == NULL) {
        return;
    }

    for (const char *cursor = text; ; cursor++) {
        if (*cursor == '\n' || *cursor == '\0') {
            line[lineLen] = '\0';
            if (lineLen > 0) {
                lumaui_render_text_internal(x, y, line, color);
            }
            y += 12;
            lineLen = 0;
            if (*cursor == '\0') {
                break;
            }
            continue;
        }

        if (lineLen < (int) sizeof(line) - 1) {
            line[lineLen++] = *cursor;
        }
    }
}

void lumaui_render_cursor(s16 x, s16 y) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect shadow = { x + 1, y + 1, 6, 10 };
    struct LumaUIRect main = { x, y, 6, 10 };
    struct LumaUIRect notch = { x + 2, y + 8, 6, 3 };

    lumaui_render_panel(&shadow, &theme->cursorShadow, &theme->cursorShadow);
    lumaui_render_panel(&main, &theme->cursor, &theme->cursorShadow);
    lumaui_render_panel(&notch, &theme->cursor, &theme->cursorShadow);
}

void lumaui_render_modal(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect backdrop = { 48, 64, 224, 112 };
    struct LumaUIRect badge = { 60, 78, 52, 14 };

    if (!state->modal.active) {
        return;
    }

    lumaui_render_panel(&backdrop, &theme->panel, &theme->panelBorder);
    lumaui_render_badge(&badge, "Modal");
    lumaui_render_text(backdrop.x + 12, backdrop.y + 34, state->modal.title, &theme->text);
    lumaui_render_text_block(backdrop.x + 12, backdrop.y + 52, state->modal.body, &theme->mutedText);
}

bool lumaui_render_point_in_rect(s32 x, s32 y, const struct LumaUIRect *rect) {
    s32 left = lumaui_to_screen_x(rect->x);
    s32 right = lumaui_to_screen_x(rect->x + rect->w);

    return x >= left && x < right && y >= rect->y && y < (rect->y + rect->h);
}
