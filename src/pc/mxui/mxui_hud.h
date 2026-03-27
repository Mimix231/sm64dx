#pragma once

#include <stdbool.h>

#include "types.h"

struct MxuiHudColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct MxuiHudRotation {
    f32 rotation;
    f32 rotationDiff;
    f32 prevPivotX;
    f32 prevPivotY;
    f32 pivotX;
    f32 pivotY;
};

enum MxuiHudResolution {
    MXUI_HUD_RESOLUTION_DJUI = 0,
    MXUI_HUD_RESOLUTION_N64,
    MXUI_HUD_RESOLUTION_COUNT,
};

enum MxuiHudFilter {
    MXUI_HUD_FILTER_NEAREST = 0,
    MXUI_HUD_FILTER_LINEAR,
    MXUI_HUD_FILTER_COUNT,
};

u8 mxui_hud_get_resolution(void);
void mxui_hud_set_resolution(u8 resolutionType);

u8 mxui_hud_get_filter(void);
void mxui_hud_set_filter(u8 filterType);

u8 mxui_hud_get_font(void);
void mxui_hud_set_font(s8 fontType);

struct MxuiHudColor* mxui_hud_get_color(void);
void mxui_hud_set_color(u8 r, u8 g, u8 b, u8 a);
void mxui_hud_reset_color(void);

struct MxuiHudRotation* mxui_hud_get_rotation(void);
void mxui_hud_set_rotation(s16 rotation, f32 pivotX, f32 pivotY);
void mxui_hud_set_rotation_interpolated(s32 prevRotation, f32 prevPivotX, f32 prevPivotY, s32 rotation, f32 pivotX, f32 pivotY);

u32 mxui_hud_get_screen_width(void);
u32 mxui_hud_get_screen_height(void);

f32 mxui_hud_get_mouse_x(void);
f32 mxui_hud_get_mouse_y(void);
f32 mxui_hud_get_raw_mouse_x(void);
f32 mxui_hud_get_raw_mouse_y(void);
bool mxui_hud_is_mouse_locked(void);
void mxui_hud_set_mouse_locked(bool locked);
u8 mxui_hud_get_mouse_buttons_down(void);
u8 mxui_hud_get_mouse_buttons_pressed(void);
u8 mxui_hud_get_mouse_buttons_released(void);
f32 mxui_hud_get_mouse_scroll_x(void);
f32 mxui_hud_get_mouse_scroll_y(void);

void mxui_hud_set_viewport(f32 x, f32 y, f32 width, f32 height);
void mxui_hud_reset_viewport(void);
void mxui_hud_set_scissor(f32 x, f32 y, f32 width, f32 height);
void mxui_hud_reset_scissor(void);

f32 mxui_hud_measure_text(const char* message);
void mxui_hud_print_text(const char* message, f32 x, f32 y, f32 scale);
void mxui_hud_print_text_interpolated(const char* message, f32 prevX, f32 prevY, f32 prevScale, f32 x, f32 y, f32 scale);
void mxui_hud_render_texture(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH);
void mxui_hud_render_texture_tile(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH);
void mxui_hud_render_texture_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH);
void mxui_hud_render_texture_tile_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH);
void mxui_hud_render_rect(f32 x, f32 y, f32 width, f32 height);
void mxui_hud_render_rect_interpolated(f32 prevX, f32 prevY, f32 prevWidth, f32 prevHeight, f32 x, f32 y, f32 width, f32 height);
void mxui_hud_render_line(f32 p1X, f32 p1Y, f32 p2X, f32 p2Y, f32 size);

f32 mxui_hud_get_fov_coeff(void);
f32 mxui_hud_get_current_fov(void);
bool mxui_hud_world_pos_to_screen_pos(Vec3f pos, VEC_OUT Vec3f out);
bool mxui_hud_is_pause_menu_created(void);

void patch_mxui_hud_before(void);
void patch_mxui_hud(f32 delta);
