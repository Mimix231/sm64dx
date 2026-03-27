#pragma once

#include "sm64.h"

#include "pc/mxui/mxui.h"
#include "pc/mxui/mxui_console.h"
#include "pc/mxui/mxui_font.h"
#include "pc/mxui/mxui_hud.h"
#include "pc/mxui/mxui_language.h"
#include "pc/mxui/mxui_popup.h"
#include "pc/mxui/mxui_assets.h"
#include "pc/mxui/mxui_theme.h"

struct DjuiBase;
struct DjuiText;
struct HudUtilsRotation {
    f32 rotation;
    f32 rotationDiff;
    f32 prevPivotX;
    f32 prevPivotY;
    f32 pivotX;
    f32 pivotY;
};

#define MAX_CHAT_MSG_LENGTH 500
#define MAX_CHAT_PACKET_LENGTH (MAX_CONFIG_STRING + MAX_CHAT_MSG_LENGTH + 19)
#define DOWNLOAD_ESTIMATE_LENGTH 32

extern bool gDjuiDisabled;
extern bool gDjuiInPlayerMenu;
extern bool gDjuiHudToWorldCalcViewport;
extern OSContPad gInteractablePad;
extern struct DjuiText* gDjuiPaletteToggle;
extern float gDownloadProgress;
extern float gDownloadProgressInf;
extern char gDownloadEstimate[];
extern char gDjuiConsoleTmpBuffer[];

char* djui_language_get(const char* section, const char* key);
char* djui_language_find_key(const char* section, const char* value);
void djui_themes_init(void);
void djui_base_set_visible(struct DjuiBase* base, bool visible);
void djui_chat_message_create(const char* message);
void djui_chat_message_create_from(u8 globalIndex, const char* message);
void* djui_chat_box_create(void);
void djui_panel_shutdown(void);
void djui_panel_modlist_create(void* caller);
void djui_panel_join_message_error(char* message);
void djui_panel_join_message_create(void* caller);
void djui_connect_menu_open(void);
void djui_popup_create(const char* message, int lines);
void djui_console_message_create(const char* message, int level);
void lua_profiler_start_counter(void* mod);
void lua_profiler_stop_counter(void* mod);
struct DjuiColor* djui_hud_get_color(void);
struct HudUtilsRotation* djui_hud_get_rotation(void);
u8 djui_hud_get_resolution(void);
void djui_hud_set_resolution(int resolutionType);
u8 djui_hud_get_filter(void);
void djui_hud_set_filter(int filterType);
u8 djui_hud_get_font(void);
void djui_hud_set_font(s8 fontType);
void djui_hud_set_color(u8 r, u8 g, u8 b, u8 a);
void djui_hud_reset_color(void);
void djui_hud_set_rotation(s16 rotation, f32 pivotX, f32 pivotY);
void djui_hud_set_rotation_interpolated(s32 prevRotation, f32 prevPivotX, f32 prevPivotY, s32 rotation, f32 pivotX, f32 pivotY);
u32 djui_hud_get_screen_width(void);
u32 djui_hud_get_screen_height(void);
f32 djui_hud_get_mouse_x(void);
f32 djui_hud_get_mouse_y(void);
f32 djui_hud_get_raw_mouse_x(void);
f32 djui_hud_get_raw_mouse_y(void);
bool djui_hud_is_mouse_locked(void);
void djui_hud_set_mouse_locked(bool locked);
u8 djui_hud_get_mouse_buttons_down(void);
u8 djui_hud_get_mouse_buttons_pressed(void);
u8 djui_hud_get_mouse_buttons_released(void);
f32 djui_hud_get_mouse_scroll_x(void);
f32 djui_hud_get_mouse_scroll_y(void);
void djui_hud_set_viewport(f32 x, f32 y, f32 width, f32 height);
void djui_hud_reset_viewport(void);
void djui_hud_set_scissor(f32 x, f32 y, f32 width, f32 height);
void djui_hud_reset_scissor(void);
f32 djui_hud_measure_text(const char* message);
void djui_hud_print_text(const char* message, f32 x, f32 y, f32 scale);
void djui_hud_print_text_interpolated(const char* message, f32 prevX, f32 prevY, f32 prevScale, f32 x, f32 y, f32 scale);
void djui_hud_render_texture(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH);
void djui_hud_render_texture_tile(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH);
void djui_hud_render_texture_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH);
void djui_hud_render_texture_tile_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH);
void djui_hud_render_rect(f32 x, f32 y, f32 width, f32 height);
void djui_hud_render_rect_interpolated(f32 prevX, f32 prevY, f32 prevWidth, f32 prevHeight, f32 x, f32 y, f32 width, f32 height);
void djui_hud_render_line(f32 p1X, f32 p1Y, f32 p2X, f32 p2Y, f32 size);
f32 djui_hud_get_fov_coeff(void);
bool djui_hud_world_pos_to_screen_pos(Vec3f pos, VEC_OUT Vec3f out);
bool djui_hud_is_pause_menu_created(void);
void djui_open_pause_menu(void);
void djui_lua_error(char* text, struct DjuiColor color);
void djui_lua_error_clear(void);
