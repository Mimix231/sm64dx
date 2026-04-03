#ifndef LUMAUI_WIDGET_H
#define LUMAUI_WIDGET_H

#include "lumaui_render.h"

void lumaui_widget_render_button(const struct LumaUIButtonSpec *button);
void lumaui_widget_render_tab_strip(const struct LumaUIRect *rect, const char *leftLabel, const char *rightLabel);
void lumaui_widget_render_scroll_region(const struct LumaUIRect *rect);
void lumaui_widget_render_badge(const struct LumaUIRect *rect, const char *label);
void lumaui_widget_render_icon_slot(const struct LumaUIRect *rect, const char *label, bool selected);
void lumaui_widget_render_action_bar(const char *leftText, const char *rightText);

#endif
