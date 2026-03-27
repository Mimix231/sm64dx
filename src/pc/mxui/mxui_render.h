#pragma once

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "types.h"

#define MXUI_MTX_PUSH   1
#define MXUI_MTX_NOPUSH 2

f32 mxui_render_screen_width(void);
f32 mxui_render_screen_height(void);
f32 mxui_render_scale(void);

f32 mxui_render_mouse_x(void);
f32 mxui_render_mouse_y(void);
f32 mxui_render_mouse_scroll_y(void);
u32 mxui_render_mouse_buttons_down(void);

void mxui_render_set_color(u8 r, u8 g, u8 b, u8 a);
void mxui_render_displaylist_begin(void);
void mxui_render_displaylist_end(void);
void mxui_render_rect(f32 x, f32 y, f32 w, f32 h);
void mxui_render_texture(const struct TextureInfo* texture, f32 x, f32 y, f32 scaleX, f32 scaleY);
void mxui_render_texture_raw(const Texture* texture, u32 w, u32 h, u8 fmt, u8 siz, bool filter);
void mxui_render_texture_tile_raw(const Texture* texture, u32 w, u32 h, u8 fmt, u8 siz, u32 tileX, u32 tileY, u32 tileW, u32 tileH, bool filter, bool font);
void mxui_render_position_translate(f32* x, f32* y);
void mxui_render_size_translate(f32* size);

void mxui_render_set_scissor(f32 x, f32 y, f32 w, f32 h);
void mxui_render_reset_scissor(void);
void mxui_render_reset_texture_clipping(void);
