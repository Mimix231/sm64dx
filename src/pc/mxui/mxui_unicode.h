#pragma once

#include <PR/ultratypes.h>
#include <stdbool.h>

void mxui_unicode_init(void);
u32 mxui_unicode_get_sprite_index(char* text);
f32 mxui_unicode_get_sprite_width(char* text, const f32 font_widths[], f32 unicodeScale);
char* mxui_unicode_next_char(char* text);
char* mxui_unicode_at_index(char* text, s32 index);
size_t mxui_unicode_len(char* text);
bool mxui_unicode_valid_char(char* text);
void mxui_unicode_cleanup_end(char* text);
char mxui_unicode_get_base_char(char* text);
void mxui_unicode_get_char(char* text, char* output);
