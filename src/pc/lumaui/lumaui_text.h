#ifndef LUMAUI_TEXT_H
#define LUMAUI_TEXT_H

#include <PR/ultratypes.h>

struct LumaUIColor;

#define LUMAUI_TEXT_GLYPH_W 8
#define LUMAUI_TEXT_GLYPH_H 14
#define LUMAUI_TEXT_LINE_HEIGHT 16

s16 lumaui_text_line_height(void);
s16 lumaui_text_measure_width(const char *text);
s16 lumaui_text_measure_block_height(const char *text);

void lumaui_text_draw(s16 x, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_text_draw_centered(s16 centerX, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_text_draw_block(s16 x, s16 y, const char *text, const struct LumaUIColor *color);
void lumaui_text_draw_block_wrapped(s16 x, s16 y, s16 maxWidth, const char *text,
                                    const struct LumaUIColor *color);

#endif
