#pragma once

#include <stdbool.h>
#include <PR/ultratypes.h>

typedef void (*MxuiActionCallback)(void);

#define MXUI_THEME_COUNT 7

enum MxuiScreenId {
    MXUI_SCREEN_NONE,
    MXUI_SCREEN_BOOT,
    MXUI_SCREEN_SAVE_SELECT,
    MXUI_SCREEN_HOME,
    MXUI_SCREEN_PAUSE,
    MXUI_SCREEN_SETTINGS_HUB,
    MXUI_SCREEN_SETTINGS_DISPLAY,
    MXUI_SCREEN_SETTINGS_SOUND,
    MXUI_SCREEN_SETTINGS_CONTROLS,
    MXUI_SCREEN_SETTINGS_CONTROLS_N64,
    MXUI_SCREEN_SETTINGS_HOTKEYS,
    MXUI_SCREEN_SETTINGS_CAMERA,
    MXUI_SCREEN_SETTINGS_FREE_CAMERA,
    MXUI_SCREEN_SETTINGS_ROMHACK_CAMERA,
    MXUI_SCREEN_SETTINGS_ACCESSIBILITY,
    MXUI_SCREEN_SETTINGS_MISC,
    MXUI_SCREEN_SETTINGS_MENU_OPTIONS,
    MXUI_SCREEN_MODS,
    MXUI_SCREEN_MOD_DETAILS,
    MXUI_SCREEN_DYNOS,
    MXUI_SCREEN_PLAYER,
    MXUI_SCREEN_LANGUAGE,
    MXUI_SCREEN_INFO,
    MXUI_SCREEN_FIRST_RUN,
    MXUI_SCREEN_MANAGE_SAVES,
    MXUI_SCREEN_MANAGE_SLOT,
};

void mxui_init(void);
void mxui_shutdown(void);
void mxui_render(void);
bool mxui_language_validate(const char* lang);

bool mxui_is_active(void);
bool mxui_is_main_menu_active(void);
bool mxui_is_pause_active(void);
bool mxui_is_character_select_active(void);
bool mxui_has_confirm(void);
bool mxui_can_open_pause_menu(void);
extern bool gDjuiInMainMenu;
extern bool gDjuiPanelPauseCreated;
bool mxui_try_open_pause_menu(void);

void mxui_open_main_flow(void);
void mxui_open_pause_menu(void);
void mxui_close_pause_menu_with_mode(s16 pauseScreenMode);
void mxui_open_screen(enum MxuiScreenId screenId);
void mxui_push_screen(enum MxuiScreenId screenId);
void mxui_open_screen_with_tag(enum MxuiScreenId screenId, s32 tag);
void mxui_push_screen_with_tag(enum MxuiScreenId screenId, s32 tag);
void mxui_pop_screen(void);
void mxui_clear(void);
bool mxui_open_character_select_menu(void);

void mxui_open_confirm(const char* title, const char* message, MxuiActionCallback on_yes_click);
