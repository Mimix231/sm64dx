#ifndef LUMAUI_RENDER_H
#define LUMAUI_RENDER_H

#include <stdbool.h>
#include <PR/ultratypes.h>

#include "lumaui_space.h"

struct LumaUIColor;
struct LumaUIState;

struct LumaUIButtonSpec {
    struct LumaUIRect rect;
    const char *label;
    bool primary;
    bool selected;
    bool hovered;
};

void lumaui_render_begin(void);
void lumaui_render_backdrop(void);
void lumaui_render_panel(const struct LumaUIRect *rect, const struct LumaUIColor *fill,
                         const struct LumaUIColor *border);
void lumaui_render_card(const struct LumaUIRect *rect, bool selected);
void lumaui_render_button(const struct LumaUIButtonSpec *button);
void lumaui_render_tab_strip(const struct LumaUIRect *rect, const char *leftLabel, const char *rightLabel);
void lumaui_render_scroll_region(const struct LumaUIRect *rect);
void lumaui_render_badge(const struct LumaUIRect *rect, const char *label);
void lumaui_render_icon_slot(const struct LumaUIRect *rect, const char *label, bool selected);
void lumaui_render_action_bar(const char *leftText, const char *rightText);
void lumaui_render_text(s16 x, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_render_text_centered(s16 centerX, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_render_text_block(s16 x, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_render_text_block_wrapped(s16 x, s16 y, s16 maxWidth, const char *text,
                                      const struct LumaUIColor *color);
void lumaui_render_cursor(s16 x, s16 y, bool pressed);
void lumaui_render_modal(struct LumaUIState *state);
bool lumaui_render_point_in_rect(s32 x, s32 y, const struct LumaUIRect *rect);
void lumaui_render_push_clip(const struct LumaUIRect *rect);
void lumaui_render_pop_clip(void);

#endif
