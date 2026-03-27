#include <stdio.h>
#include <string.h>

#include "sm64.h"
#include "seq_ids.h"

#include "pc/configfile.h"
#include "pc/network/network.h"
#include "pc/utils/misc.h"

#include "mxui.h"
#include "mxui_console.h"
#include "mxui_font.h"
#include "mxui_hud.h"
#include "mxui_internal.h"
#include "mxui_language.h"
#include "mxui_assets.h"
#include "mxui_popup.h"
#include "mxui_runtime.h"
#include "mxui_theme.h"

#define STAGE_MUSIC 0
#define DOWNLOAD_ESTIMATE_LENGTH 32

struct DjuiBase;
struct DjuiText;

struct DjuiFont {
    f32 charWidth;
    f32 charHeight;
    f32 lineHeight;
    f32 xOffset;
    f32 yOffset;
    f32 defaultFontScale;
    const Gfx* textBeginDisplayList;
    void (*render_char)(char*);
    f32 (*char_width)(char*);
};

struct HudUtilsRotation {
    f32 rotation;
    f32 rotationDiff;
    f32 prevPivotX;
    f32 prevPivotY;
    f32 pivotX;
    f32 pivotY;
};

bool gDjuiDisabled = false;
bool gDjuiInPlayerMenu = false;
bool gDjuiHudToWorldCalcViewport = true;
OSContPad gInteractablePad = { 0 };
struct DjuiText* gDjuiPaletteToggle = NULL;
float gDownloadProgress = 0.0f;
float gDownloadProgressInf = 0.0f;
char gDownloadEstimate[DOWNLOAD_ESTIMATE_LENGTH] = " ";
char gDjuiConsoleTmpBuffer[MXUI_CONSOLE_MAX_TMP_BUFFER] = { 0 };

struct MainMenuSounds gMainMenuSounds[] = {
    { "Title Screen", SEQ_MENU_TITLE_SCREEN },
    { "File Select", SEQ_MENU_FILE_SELECT },
    { "Grass", SEQ_LEVEL_GRASS },
    { "Water", SEQ_LEVEL_WATER },
    { "Snow", SEQ_LEVEL_SNOW },
    { "Slide", SEQ_LEVEL_SLIDE },
    { "Bowser Stage", SEQ_LEVEL_KOOPA_ROAD },
    { "Bowser Fight", SEQ_LEVEL_BOSS_KOOPA },
    { "Spooky", SEQ_LEVEL_SPOOKY },
    { "Hot", SEQ_LEVEL_HOT },
    { "Underground", SEQ_LEVEL_UNDERGROUND },
    { "Bowser Finale", SEQ_LEVEL_BOSS_KOOPA_FINAL },
    { "Staff Roll", SEQ_EVENT_CUTSCENE_CREDITS },
    { "Stage Music", STAGE_MUSIC },
};

static struct DjuiTheme sThemeBobOmbBattlefield = {
    .id = "BOB_OMB_BATTLEFIELD",
    .name = "Bob-omb Battlefield",
    .interactables = {
        .textColor = { 252, 248, 236, 255 },
        .defaultRectColor = { 101, 176, 106, 255 },
        .cursorDownRectColor = { 255, 237, 138, 255 },
        .hoveredRectColor = { 126, 204, 130, 255 },
        .defaultBorderColor = { 255, 221, 118, 255 },
        .cursorDownBorderColor = { 255, 221, 118, 255 },
        .hoveredBorderColor = { 124, 210, 255, 96 },
    },
    .threePanels = { { 35, 86, 124, 226 }, { 255, 221, 118, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeCoolCoolMountain = {
    .id = "COOL_COOL_MOUNTAIN",
    .name = "Cool, Cool Mountain",
    .interactables = {
        .textColor = { 242, 248, 255, 255 },
        .defaultRectColor = { 152, 214, 255, 255 },
        .cursorDownRectColor = { 255, 255, 255, 255 },
        .hoveredRectColor = { 194, 232, 255, 255 },
        .defaultBorderColor = { 244, 250, 255, 255 },
        .cursorDownBorderColor = { 244, 250, 255, 255 },
        .hoveredBorderColor = { 214, 238, 255, 110 },
    },
    .threePanels = { { 62, 98, 156, 228 }, { 244, 250, 255, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeJollyRogerBay = {
    .id = "JOLLY_ROGER_BAY",
    .name = "Jolly Roger Bay",
    .interactables = {
        .textColor = { 242, 252, 248, 255 },
        .defaultRectColor = { 76, 178, 188, 255 },
        .cursorDownRectColor = { 202, 252, 246, 255 },
        .hoveredRectColor = { 112, 214, 220, 255 },
        .defaultBorderColor = { 162, 246, 236, 255 },
        .cursorDownBorderColor = { 162, 246, 236, 255 },
        .hoveredBorderColor = { 104, 236, 226, 96 },
    },
    .threePanels = { { 26, 84, 112, 228 }, { 162, 246, 236, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeBigBoosHaunt = {
    .id = "BIG_BOOS_HAUNT",
    .name = "Big Boo's Haunt",
    .interactables = {
        .textColor = { 248, 244, 255, 255 },
        .defaultRectColor = { 146, 124, 210, 255 },
        .cursorDownRectColor = { 235, 222, 255, 255 },
        .hoveredRectColor = { 178, 152, 234, 255 },
        .defaultBorderColor = { 232, 220, 255, 255 },
        .cursorDownBorderColor = { 232, 220, 255, 255 },
        .hoveredBorderColor = { 174, 156, 255, 104 },
    },
    .threePanels = { { 66, 52, 108, 228 }, { 232, 220, 255, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeLethalLavaLand = {
    .id = "LETHAL_LAVA_LAND",
    .name = "Lethal Lava Land",
    .interactables = {
        .textColor = { 255, 246, 232, 255 },
        .defaultRectColor = { 232, 114, 48, 255 },
        .cursorDownRectColor = { 255, 220, 116, 255 },
        .hoveredRectColor = { 252, 142, 64, 255 },
        .defaultBorderColor = { 255, 202, 98, 255 },
        .cursorDownBorderColor = { 255, 202, 98, 255 },
        .hoveredBorderColor = { 255, 128, 48, 110 },
    },
    .threePanels = { { 106, 38, 24, 230 }, { 255, 202, 98, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeWhompsFortress = {
    .id = "WHOMPS_FORTRESS",
    .name = "Whomp's Fortress",
    .interactables = {
        .textColor = { 250, 248, 242, 255 },
        .defaultRectColor = { 124, 184, 128, 255 },
        .cursorDownRectColor = { 248, 236, 170, 255 },
        .hoveredRectColor = { 148, 208, 150, 255 },
        .defaultBorderColor = { 230, 240, 250, 255 },
        .cursorDownBorderColor = { 230, 240, 250, 255 },
        .hoveredBorderColor = { 178, 214, 255, 100 },
    },
    .threePanels = { { 68, 92, 122, 230 }, { 230, 240, 250, 255 } },
    .panels = { false },
};

static struct DjuiTheme sThemeShiftingSandLand = {
    .id = "SHIFTING_SAND_LAND",
    .name = "Shifting Sand Land",
    .interactables = {
        .textColor = { 252, 248, 236, 255 },
        .defaultRectColor = { 72, 170, 152, 255 },
        .cursorDownRectColor = { 255, 236, 156, 255 },
        .hoveredRectColor = { 94, 194, 176, 255 },
        .defaultBorderColor = { 255, 222, 132, 255 },
        .cursorDownBorderColor = { 255, 222, 132, 255 },
        .hoveredBorderColor = { 255, 210, 112, 108 },
    },
    .threePanels = { { 124, 76, 28, 230 }, { 255, 222, 132, 255 } },
    .panels = { false },
};

struct DjuiTheme* gDjuiThemes[] = {
    &sThemeBobOmbBattlefield,
    &sThemeCoolCoolMountain,
    &sThemeJollyRogerBay,
    &sThemeBigBoosHaunt,
    &sThemeLethalLavaLand,
    &sThemeWhompsFortress,
    &sThemeShiftingSandLand,
};

static struct DjuiFont sDjuiFontCopies[FONT_COUNT];
const struct DjuiFont* gDjuiFonts[FONT_COUNT] = { NULL };

void mxui_exports_init(void) {
    for (s32 i = 0; i < FONT_COUNT; i++) {
        const struct MxuiFont* font = gMxuiFonts[i];
        if (font == NULL) {
            gDjuiFonts[i] = NULL;
            continue;
        }

        sDjuiFontCopies[i].charWidth = font->charWidth;
        sDjuiFontCopies[i].charHeight = font->charHeight;
        sDjuiFontCopies[i].lineHeight = font->lineHeight;
        sDjuiFontCopies[i].xOffset = font->xOffset;
        sDjuiFontCopies[i].yOffset = font->yOffset;
        sDjuiFontCopies[i].defaultFontScale = font->defaultFontScale;
        sDjuiFontCopies[i].textBeginDisplayList = font->textBeginDisplayList;
        sDjuiFontCopies[i].render_char = font->render_char;
        sDjuiFontCopies[i].char_width = font->char_width;
        gDjuiFonts[i] = &sDjuiFontCopies[i];
    }
}

char* djui_language_get(const char* section, const char* key) {
    return mxui_language_get(section, key);
}

char* djui_language_find_key(const char* section, const char* value) {
    return mxui_language_find_key(section, value);
}

void djui_themes_init(void) {
    mxui_exports_init();
    (void)gDjuiThemes;
}

void djui_base_set_visible(struct DjuiBase* base, bool visible) {
    (void)base;
    (void)visible;
}

void djui_chat_message_create(const char* message) {
    (void)message;
}

void djui_chat_message_create_from(u8 globalIndex, const char* message) {
    (void)globalIndex;
    (void)message;
}

void* djui_chat_box_create(void) {
    return NULL;
}

void djui_panel_shutdown(void) {
}

void djui_panel_modlist_create(void* caller) {
    (void)caller;
    if (mxui_is_active()) {
        mxui_push_screen(MXUI_SCREEN_MODS);
    }
}

void djui_panel_join_message_error(char* message) {
    mxui_popup_create(message == NULL ? "Join failed." : message, 3);
}

void djui_panel_join_message_create(void* caller) {
    (void)caller;
    mxui_popup_create("Online join UI is disabled in this offline SM64 DX build.", 3);
}

void djui_connect_menu_open(void) {
    mxui_runtime_open_main_flow();
}

void djui_popup_create(const char* message, int lines) {
    mxui_popup_create(message, lines);
}

void djui_console_message_create(const char* message, int level) {
    int mapped = MXUI_CONSOLE_MESSAGE_INFO;
    if (level == 1) { mapped = MXUI_CONSOLE_MESSAGE_WARNING; }
    if (level >= 2) { mapped = MXUI_CONSOLE_MESSAGE_ERROR; }
    mxui_console_message_create(message, mapped);
}

void lua_profiler_start_counter(void* mod) {
    (void)mod;
}

void lua_profiler_stop_counter(void* mod) {
    (void)mod;
}

struct DjuiColor* djui_hud_get_color(void) {
    return (struct DjuiColor*)mxui_hud_get_color();
}

struct HudUtilsRotation* djui_hud_get_rotation(void) {
    return (struct HudUtilsRotation*)mxui_hud_get_rotation();
}

u8 djui_hud_get_resolution(void) { return mxui_hud_get_resolution(); }
void djui_hud_set_resolution(int resolutionType) { mxui_hud_set_resolution((u8)resolutionType); }
u8 djui_hud_get_filter(void) { return mxui_hud_get_filter(); }
void djui_hud_set_filter(int filterType) { mxui_hud_set_filter((u8)filterType); }
u8 djui_hud_get_font(void) { return mxui_hud_get_font(); }
void djui_hud_set_font(s8 fontType) { mxui_hud_set_font(fontType); }
void djui_hud_set_color(u8 r, u8 g, u8 b, u8 a) { mxui_hud_set_color(r, g, b, a); }
void djui_hud_reset_color(void) { mxui_hud_reset_color(); }
void djui_hud_set_rotation(s16 rotation, f32 pivotX, f32 pivotY) { mxui_hud_set_rotation(rotation, pivotX, pivotY); }
void djui_hud_set_rotation_interpolated(s32 prevRotation, f32 prevPivotX, f32 prevPivotY, s32 rotation, f32 pivotX, f32 pivotY) { mxui_hud_set_rotation_interpolated(prevRotation, prevPivotX, prevPivotY, rotation, pivotX, pivotY); }
u32 djui_hud_get_screen_width(void) { return mxui_hud_get_screen_width(); }
u32 djui_hud_get_screen_height(void) { return mxui_hud_get_screen_height(); }
f32 djui_hud_get_mouse_x(void) { return mxui_hud_get_mouse_x(); }
f32 djui_hud_get_mouse_y(void) { return mxui_hud_get_mouse_y(); }
f32 djui_hud_get_raw_mouse_x(void) { return mxui_hud_get_raw_mouse_x(); }
f32 djui_hud_get_raw_mouse_y(void) { return mxui_hud_get_raw_mouse_y(); }
bool djui_hud_is_mouse_locked(void) { return mxui_hud_is_mouse_locked(); }
void djui_hud_set_mouse_locked(bool locked) { mxui_hud_set_mouse_locked(locked); }
u8 djui_hud_get_mouse_buttons_down(void) { return mxui_hud_get_mouse_buttons_down(); }
u8 djui_hud_get_mouse_buttons_pressed(void) { return mxui_hud_get_mouse_buttons_pressed(); }
u8 djui_hud_get_mouse_buttons_released(void) { return mxui_hud_get_mouse_buttons_released(); }
f32 djui_hud_get_mouse_scroll_x(void) { return mxui_hud_get_mouse_scroll_x(); }
f32 djui_hud_get_mouse_scroll_y(void) { return mxui_hud_get_mouse_scroll_y(); }
void djui_hud_set_viewport(f32 x, f32 y, f32 width, f32 height) { mxui_hud_set_viewport(x, y, width, height); }
void djui_hud_reset_viewport(void) { mxui_hud_reset_viewport(); }
void djui_hud_set_scissor(f32 x, f32 y, f32 width, f32 height) { mxui_hud_set_scissor(x, y, width, height); }
void djui_hud_reset_scissor(void) { mxui_hud_reset_scissor(); }
f32 djui_hud_measure_text(const char* message) { return mxui_hud_measure_text(message); }
void djui_hud_print_text(const char* message, f32 x, f32 y, f32 scale) { mxui_hud_print_text(message, x, y, scale); }
void djui_hud_print_text_interpolated(const char* message, f32 prevX, f32 prevY, f32 prevScale, f32 x, f32 y, f32 scale) { mxui_hud_print_text_interpolated(message, prevX, prevY, prevScale, x, y, scale); }
void djui_hud_render_texture(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH) { mxui_hud_render_texture(texInfo, x, y, scaleW, scaleH); }
void djui_hud_render_texture_tile(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH) { mxui_hud_render_texture_tile(texInfo, x, y, scaleW, scaleH, tileX, tileY, tileW, tileH); }
void djui_hud_render_texture_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH) { mxui_hud_render_texture_interpolated(texInfo, prevX, prevY, prevScaleW, prevScaleH, x, y, scaleW, scaleH); }
void djui_hud_render_texture_tile_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH) { mxui_hud_render_texture_tile_interpolated(texInfo, prevX, prevY, prevScaleW, prevScaleH, x, y, scaleW, scaleH, tileX, tileY, tileW, tileH); }
void djui_hud_render_rect(f32 x, f32 y, f32 width, f32 height) { mxui_hud_render_rect(x, y, width, height); }
void djui_hud_render_rect_interpolated(f32 prevX, f32 prevY, f32 prevWidth, f32 prevHeight, f32 x, f32 y, f32 width, f32 height) { mxui_hud_render_rect_interpolated(prevX, prevY, prevWidth, prevHeight, x, y, width, height); }
void djui_hud_render_line(f32 p1X, f32 p1Y, f32 p2X, f32 p2Y, f32 size) { mxui_hud_render_line(p1X, p1Y, p2X, p2Y, size); }
f32 djui_hud_get_fov_coeff(void) { return mxui_hud_get_fov_coeff(); }
bool djui_hud_world_pos_to_screen_pos(Vec3f pos, VEC_OUT Vec3f out) { return mxui_hud_world_pos_to_screen_pos(pos, out); }
bool djui_hud_is_pause_menu_created(void) { return mxui_hud_is_pause_menu_created(); }

void djui_open_pause_menu(void) {
    mxui_open_pause_menu();
}

void djui_lua_error(char* text, struct DjuiColor color) {
    (void)color;
    mxui_console_message_create(text, MXUI_CONSOLE_MESSAGE_ERROR);
}

void djui_lua_error_clear(void) {
}
