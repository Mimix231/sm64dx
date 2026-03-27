#pragma once

#include "mxui_internal.h"

bool mxui_component_button(struct MxuiRect rect, const char* label, bool danger);
bool mxui_component_toggle(struct MxuiRect rect, const char* label, bool* value);
bool mxui_component_select_u32(struct MxuiRect rect, const char* label, const char* const* choices, s32 count, unsigned int* value);
bool mxui_component_slider_u32(struct MxuiRect rect, const char* label, unsigned int* value, unsigned int minValue, unsigned int maxValue, unsigned int step);
bool mxui_component_bind_button(struct MxuiRect rect, const char* text, bool focused, bool hovered);
bool mxui_component_bind_row(struct MxuiRect rect, const char* label, unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS], struct Mod* mod, const char* bindId, s32 hookIndex);
void mxui_component_footer_button(struct MxuiRect footer, bool right, const char* label, bool* clicked);
void mxui_component_footer_center_text(struct MxuiRect footer, const char* text);
void mxui_component_render_confirm(void);
