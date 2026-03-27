#include "mxui_internal.h"

static const struct MxuiScreenConfig sMxuiScreenConfigs[] = {
    { MXUI_SCREEN_BOOT, "SUPER MARIO 64 DX", "Offline singleplayer. Choose a save, mod your game, and jump in.", MXUI_TEMPLATE_FRONT_PAGE, false, NULL },
    { MXUI_SCREEN_SAVE_SELECT, "Save Select", "Choose a local save file or set up a fresh adventure.", MXUI_TEMPLATE_FRONT_PAGE, true, "Back" },
    { MXUI_SCREEN_HOME, "Adventure", "Your offline front end for play, mods, settings, and packs.", MXUI_TEMPLATE_FRONT_PAGE, false, NULL },
    { MXUI_SCREEN_PAUSE, "Pause", "Gameplay is frozen while the MXUI pause shell is open.", MXUI_TEMPLATE_FRONT_PAGE, false, NULL },
    { MXUI_SCREEN_SETTINGS_HUB, "Settings", "Tune controls, visuals, audio, accessibility, character packs, and local mod setup.", MXUI_TEMPLATE_FRONT_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_DISPLAY, "Display", "Choose display mode, frame pacing, and rendering quality.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_SOUND, "Sound", "Balance music, effects, and ambience.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_CONTROLS, "Controls", "Manage binds, pads, and analog behavior.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_CONTROLS_N64, "N64 Binds", "Edit the core gameplay button layout.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_HOTKEYS, "Hotkeys", "Game menu shortcuts and mod actions in one place.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_CAMERA, "Camera", "Choose a camera style and inversion behavior.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_FREE_CAMERA, "Free Camera", "Tune free camera movement, collision, and look sensitivity.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_ROMHACK_CAMERA, "Romhack Camera", "Classic romhack camera behavior and stage rules.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_ACCESSIBILITY, "Accessibility", "Comfort presets and visibility options for more players.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_MISC, "Misc", "Language, menu behavior, and support tools.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_SETTINGS_MENU_OPTIONS, "Menu Options", "Theme, scale, font, and title presentation.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_MODS, "Mods", "Local mods, profiles, quick actions, and conflict-safe management.", MXUI_TEMPLATE_GRID_PAGE, true, "Back" },
    { MXUI_SCREEN_MOD_DETAILS, "Mod Details", "Detailed controls and mod-specific actions.", MXUI_TEMPLATE_DETAIL_PAGE, true, "Back" },
    { MXUI_SCREEN_DYNOS, "DynOS Packs", "Enable or disable local DynOS packs.", MXUI_TEMPLATE_GRID_PAGE, true, "Back" },
    { MXUI_SCREEN_PLAYER, "Player", "Character and local player presentation tools.", MXUI_TEMPLATE_DETAIL_PAGE, true, "Back" },
    { MXUI_SCREEN_LANGUAGE, "Language", "Choose the interface language used by SM64 DX.", MXUI_TEMPLATE_SETTINGS_PAGE, true, "Back" },
    { MXUI_SCREEN_INFO, "Information", "What SM64 DX is and how it handles your local data.", MXUI_TEMPLATE_DETAIL_PAGE, true, "Back" },
    { MXUI_SCREEN_FIRST_RUN, "Welcome to SM64 DX", "Pick a starter save slot and comfort preset.", MXUI_TEMPLATE_FRONT_PAGE, true, "Back" },
    { MXUI_SCREEN_MANAGE_SAVES, "Manage Saves", "Open any slot to activate, copy, or erase it.", MXUI_TEMPLATE_FRONT_PAGE, true, "Back" },
    { MXUI_SCREEN_MANAGE_SLOT, "Manage Save", "Handle one save slot without leaving MXUI.", MXUI_TEMPLATE_DETAIL_PAGE, true, "Back" },
};

const struct MxuiScreenConfig* mxui_screen_config(enum MxuiScreenId screenId) {
    for (s32 i = 0; i < (s32)(sizeof(sMxuiScreenConfigs) / sizeof(sMxuiScreenConfigs[0])); i++) {
        if (sMxuiScreenConfigs[i].id == screenId) {
            return &sMxuiScreenConfigs[i];
        }
    }

    return &sMxuiScreenConfigs[0];
}
