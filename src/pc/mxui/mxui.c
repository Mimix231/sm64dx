#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sm64.h"
#include "dialog_ids.h"
#include "mxui_internal.h"

#include "pc/controller/controller_api.h"
#include "pc/controller/controller_bind_mapping.h"
#include "pc/controller/controller_keyboard.h"
#include "pc/configfile.h"
#include "pc/fs/fs.h"
#include "pc/ini.h"
#include "pc/mods/mod.h"
#include "pc/mods/mod_bindings.h"
#include "pc/mods/mod_cache.h"
#include "pc/mods/mod_profiles.h"
#include "pc/mods/mods.h"
#include "pc/mods/mods_utils.h"
#include "pc/network/network.h"
#include "pc/network/network_utils.h"
#include "pc/network/network_player.h"
#include "pc/pc_main.h"
#include "pc/platform.h"
#include "pc/startup_experience.h"
#include "pc/lua/smlua.h"
#include "pc/lua/smlua_hooks.h"
#include "pc/lua/utils/smlua_audio_utils.h"
#include "pc/lua/utils/smlua_text_utils.h"
#include "pc/utils/misc.h"

#include "audio/external.h"
#include "data/dynos.c.h"
#include "engine/math_util.h"
#include "game/area.h"
#include "game/bettercamera.h"
#include "game/camera.h"
#include "game/first_person_cam.h"
#include "game/game_init.h"
#include "game/hardcoded.h"
#include "game/hud.h"
#include "game/ingame_menu.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
#include "game/obj_behaviors.h"
#include "game/rumble_init.h"
#include "game/save_file.h"
#include "game/sound_init.h"
#include "menu/intro_geo.h"
#include "sounds.h"

#include "mxui_cursor.h"
#include "mxui_assets.h"
#include "mxui_language.h"
#include "mxui_popup.h"
#include "mxui_render.h"
#include "mxui_unicode.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

struct HotkeyNamedBinding {
    const char* name;
    unsigned int* configKey;
};

struct MxuiState sMxui = { 0 };
static const char* sModProfileIds[MXUI_PROFILE_COUNT] = { "slot-1", "slot-2", "slot-3" };
static const char* sModCategoryLabels[] = { "All", "Misc", "Romhacks", "Gamemodes", "Movesets", "Character Select" };
static const char* sModCategoryKeys[] = { NULL, NULL, "romhack", "gamemode", "moveset", "cs" };
static const char* sMxuiThemeLabels[] = {
    "Bob-omb Battlefield",
    "Cool, Cool Mountain",
    "Jolly Roger Bay",
    "Big Boo's Haunt",
    "Lethal Lava Land",
    "Whomp's Fortress",
    "Shifting Sand Land",
};
static const struct HotkeyNamedBinding sGameplayBindings[] = {
    { "A Button",      configKeyA          },
    { "B Button",      configKeyB          },
    { "Start Button",  configKeyStart      },
    { "L Trigger",     configKeyL          },
    { "R Trigger",     configKeyR          },
    { "Z Trigger",     configKeyZ          },
    { "C-Up",          configKeyCUp        },
    { "C-Down",        configKeyCDown      },
    { "C-Left",        configKeyCLeft      },
    { "C-Right",       configKeyCRight     },
    { "Move Up",       configKeyStickUp    },
    { "Move Down",     configKeyStickDown  },
    { "Move Left",     configKeyStickLeft  },
    { "Move Right",    configKeyStickRight },
};

static bool mxui_char_select_call_bool0(const char* functionName);
static bool mxui_char_select_call_bool1(const char* functionName, s32 arg0);

#define MXUI_CHAR_SELECT_MAX_CHARACTERS 256
#define MXUI_CHAR_SELECT_MAX_VARIANTS 128

struct MxuiCharacterSelectBridge {
    bool available;
    s32 count;
    s32 characterIndices[MXUI_CHAR_SELECT_MAX_CHARACTERS];
    char characterLabels[MXUI_CHAR_SELECT_MAX_CHARACTERS][96];
    s32 currentCharacter;
    s32 currentVariant;
    s32 variantCount;
    char variantLabels[MXUI_CHAR_SELECT_MAX_VARIANTS][96];
    char characterName[96];
    char characterCategory[96];
    char characterSaveName[96];
    char variantName[96];
    char variantCredit[128];
    char variantType[48];
    char variantSource[128];
    char variantDescription[512];
};

static const struct {
    const char* label;
    unsigned int* configKey;
    unsigned int defaults[MAX_BINDS];
} sCoreHotkeys[] = {
    { "Game Menu",  configKeyGameMenu,   { 0x0001,     VK_INVALID, VK_INVALID } },
    { "Prev Page",  configKeyPrevPage,   { 0x0016,     VK_INVALID, VK_INVALID } },
    { "Next Page",  configKeyNextPage,   { 0x0018,     VK_INVALID, VK_INVALID } },
    { "Disconnect", configKeyDisconnect, { VK_INVALID, VK_INVALID, VK_INVALID } },
    { "D-Pad Up",   configKeyDUp,        { 0x0147,     0x100b,     VK_INVALID } },
    { "D-Pad Down", configKeyDDown,      { 0x014f,     0x100c,     VK_INVALID } },
    { "D-Pad Left", configKeyDLeft,      { 0x0153,     0x100d,     VK_INVALID } },
    { "D-Pad Right",configKeyDRight,     { 0x0151,     0x100e,     VK_INVALID } },
    { "X Button",   configKeyX,          { 0x0017,     0x1002,     VK_INVALID } },
    { "Y Button",   configKeyY,          { 0x0032,     0x1003,     VK_INVALID } },
};

bool gDjuiInMainMenu = false;
bool gDjuiPanelPauseCreated = false;
static void mxui_arm_open_guards(void) {
    sMxui.ignoreAcceptUntilRelease = true;
    sMxui.ignoreMenuToggleUntilRelease = true;
}

static void mxui_start_screen_open(void) {
    sMxui.screenAnim = 0.0f;
    sMxui.screenTarget = 1.0f;
}

static void mxui_start_shell_open(void) {
    sMxui.pauseClosing = false;
    sMxui.shellAnim = 0.0f;
    sMxui.shellTarget = 1.0f;
    mxui_start_screen_open();
}

static void mxui_reset_modal_state(void) {
    sMxui.confirmOpen = false;
    sMxui.confirmAnim = 0.0f;
    sMxui.confirmTarget = 0.0f;
    sMxui.confirmTitle[0] = '\0';
    sMxui.confirmMessage[0] = '\0';
    sMxui.confirmYes = NULL;
    sMxui.mouseCaptureValid = false;
    sMxui.pressedRectValid = false;
}

static bool mxui_start_pause_close_animation(s16 pauseScreenMode) {
    if (!sMxui.pauseMenu || sMxui.pauseClosing || !mxui_is_active()) {
        return false;
    }
    if (sMxui.shellAnim <= 0.04f) {
        return false;
    }
    sMxui.pauseClosing = true;
    sMxui.deferredPauseMode = pauseScreenMode;
    sMxui.shellTarget = 0.0f;
    mxui_arm_open_guards();
    return true;
}

void mxui_init(void) {
    if (sMxui.initialized) { return; }
    memset(&sMxui, 0, sizeof(sMxui));
    sMxui.initialized = true;
    mxui_unicode_init();
    sMxui.shellAnim = 0.0f;
    sMxui.shellTarget = 0.0f;
    sMxui.screenAnim = 0.0f;
    sMxui.screenTarget = 0.0f;
    sMxui.confirmAnim = 0.0f;
    sMxui.confirmTarget = 0.0f;
    sMxui.modCategory = 0;
    sMxui.modProfile = 0;
    sMxui.capturedHookIndex = -1;
    sMxui.capturedBindSlot = -1;
    if (!mxui_language_init(configLanguage)) {
        mxui_language_init("English");
    }
}

void mxui_shutdown(void) {
    mxui_language_shutdown();
    gDjuiInMainMenu = false;
    gDjuiPanelPauseCreated = false;
    memset(&sMxui, 0, sizeof(sMxui));
}

bool mxui_is_active(void) {
    return sMxui.active && sMxui.depth > 0;
}

bool mxui_is_main_menu_active(void) {
    return mxui_is_active() && sMxui.mainMenu;
}

bool mxui_is_pause_active(void) {
    return mxui_is_active() && sMxui.pauseMenu;
}

bool mxui_is_character_select_active(void) {
    return mxui_char_select_call_bool0("dx_is_browser_open");
}

bool mxui_has_confirm(void) {
    return sMxui.confirmOpen;
}

bool mxui_can_open_pause_menu(void) {
    if (gDjuiInMainMenu || gDjuiPanelPauseCreated || mxui_has_confirm() || mxui_is_pause_active()) {
        return false;
    }
    if (sMxui.capturedBind != NULL) {
        return false;
    }
    if (gCurrDemoInput != NULL || sDelayedWarpOp != WARP_OP_NONE) {
        return false;
    }
    if (find_object_with_behavior(bhvActSelector) != NULL) {
        return false;
    }
    if (get_dialog_id() != DIALOG_NONE || gMarioStates[0].action == ACT_EXIT_LAND_SAVE_DIALOG) {
        return false;
    }
    return true;
}

bool mxui_try_open_pause_menu(void) {
    if (mxui_is_pause_active()) {
        return true;
    }
    if (!mxui_can_open_pause_menu()) {
        return false;
    }

    lower_background_noise(1);
    cancel_rumble();
    gCameraMovementFlags &= ~CAM_MOVE_PAUSE_SCREEN;
    gPauseMenuHidden = true;
    set_play_mode(PLAY_MODE_PAUSED);
    mxui_open_pause_menu();
    return true;
}

static bool mxui_queue_deferred_action(enum MxuiDeferredActionType action, enum MxuiScreenId screenId, s32 tag, s16 pauseMode, const char* title, const char* message, MxuiActionCallback confirmYes) {
    if (!sMxui.rendering) {
        return false;
    }
    if (sMxui.deferredAction != MXUI_DEFERRED_NONE) {
        return true;
    }

    sMxui.deferredAction = action;
    sMxui.deferredScreenId = screenId;
    sMxui.deferredTag = tag;
    sMxui.deferredPauseMode = pauseMode;
    snprintf(sMxui.deferredConfirmTitle, sizeof(sMxui.deferredConfirmTitle), "%s", title != NULL ? title : "Confirm");
    snprintf(sMxui.deferredConfirmMessage, sizeof(sMxui.deferredConfirmMessage), "%s", message != NULL ? message : "");
    sMxui.deferredConfirmYes = confirmYes;
    return true;
}

static void mxui_open_screen_with_tag_immediate(enum MxuiScreenId screenId, s32 tag) {
    bool wasActive = sMxui.active && sMxui.depth > 0;
    sMxui.depth = 0;
    sMxui.active = true;
    sMxui.pauseClosing = false;
    mxui_reset_modal_state();
    mxui_arm_open_guards();
    mxui_start_screen_open();
    if (!wasActive) {
        mxui_start_shell_open();
    } else {
        sMxui.shellAnim = 1.0f;
        sMxui.shellTarget = 1.0f;
    }
    mxui_push_if_possible(screenId, tag);
}

static void mxui_push_screen_with_tag_immediate(enum MxuiScreenId screenId, s32 tag) {
    if (!mxui_is_active()) {
        mxui_open_screen_with_tag_immediate(screenId, tag);
        return;
    }
    mxui_reset_modal_state();
    mxui_arm_open_guards();
    mxui_start_screen_open();
    mxui_push_if_possible(screenId, tag);
}

static void mxui_pop_screen_immediate(void) {
    if (sMxui.confirmOpen) {
        mxui_reset_modal_state();
        mxui_arm_open_guards();
        return;
    }
    if (sMxui.depth <= 0) { return; }
    sMxui.depth--;
    mxui_arm_open_guards();
    mxui_start_screen_open();
    if (sMxui.depth <= 0) {
        sMxui.active = false;
        sMxui.mainMenu = false;
        sMxui.pauseMenu = false;
    }
    sMxui.wantsConfigSave = true;
}

static void mxui_clear_immediate(void) {
    sMxui.depth = 0;
    sMxui.active = false;
    sMxui.mainMenu = false;
    sMxui.pauseMenu = false;
    sMxui.pauseClosing = false;
    sMxui.shellAnim = 0.0f;
    sMxui.shellTarget = 0.0f;
    sMxui.screenAnim = 0.0f;
    sMxui.screenTarget = 0.0f;
    mxui_reset_modal_state();
    sMxui.ignoreAcceptUntilRelease = false;
    sMxui.ignoreMenuToggleUntilRelease = false;
    mxui_finish_bind_capture();
    mxui_cursor_set_visible(false);
}

static void mxui_open_confirm_immediate(const char* title, const char* message, MxuiActionCallback on_yes_click) {
    mxui_reset_modal_state();
    sMxui.confirmOpen = true;
    sMxui.confirmAnim = 0.0f;
    sMxui.confirmTarget = 1.0f;
    mxui_arm_open_guards();
    snprintf(sMxui.confirmTitle, sizeof(sMxui.confirmTitle), "%s", title != NULL ? title : "Confirm");
    snprintf(sMxui.confirmMessage, sizeof(sMxui.confirmMessage), "%s", message != NULL ? message : "");
    sMxui.confirmYes = on_yes_click;
}

static void mxui_close_pause_menu_with_mode_immediate(s16 pauseScreenMode) {
    mxui_reset_modal_state();
    mxui_clear_immediate();
    gPauseMenuHidden = false;
    set_dialog_box_state(0);
    gPauseScreenMode = pauseScreenMode;
    gMenuMode = -1;
    play_sound(SOUND_MENU_PAUSE_2, gGlobalSoundSource);
    gDjuiPanelPauseCreated = false;
}

void mxui_open_screen_with_tag(enum MxuiScreenId screenId, s32 tag) {
    if (mxui_queue_deferred_action(MXUI_DEFERRED_OPEN_SCREEN, screenId, tag, 0, NULL, NULL, NULL)) {
        return;
    }
    mxui_open_screen_with_tag_immediate(screenId, tag);
}

void mxui_open_screen(enum MxuiScreenId screenId) {
    mxui_open_screen_with_tag(screenId, 0);
}

void mxui_push_screen_with_tag(enum MxuiScreenId screenId, s32 tag) {
    if (mxui_queue_deferred_action(MXUI_DEFERRED_PUSH_SCREEN, screenId, tag, 0, NULL, NULL, NULL)) {
        return;
    }
    mxui_push_screen_with_tag_immediate(screenId, tag);
}

void mxui_push_screen(enum MxuiScreenId screenId) {
    mxui_push_screen_with_tag(screenId, 0);
}

void mxui_pop_screen(void) {
    if (mxui_queue_deferred_action(MXUI_DEFERRED_POP_SCREEN, MXUI_SCREEN_BOOT, 0, 0, NULL, NULL, NULL)) {
        return;
    }
    mxui_pop_screen_immediate();
}

void mxui_clear(void) {
    if (mxui_queue_deferred_action(MXUI_DEFERRED_CLEAR, MXUI_SCREEN_BOOT, 0, 0, NULL, NULL, NULL)) {
        return;
    }
    mxui_clear_immediate();
}

void mxui_open_confirm(const char* title, const char* message, MxuiActionCallback on_yes_click) {
    if (mxui_queue_deferred_action(MXUI_DEFERRED_OPEN_CONFIRM, MXUI_SCREEN_BOOT, 0, 0, title, message, on_yes_click)) {
        return;
    }
    mxui_open_confirm_immediate(title, message, on_yes_click);
}

void mxui_open_main_flow(void) {
    mxui_reset_modal_state();
    mxui_open_screen(MXUI_SCREEN_BOOT);
    mxui_start_shell_open();
    sMxui.mainMenu = true;
    sMxui.pauseMenu = false;
    gDjuiInMainMenu = true;
    mxui_cursor_set_visible(true);

    if (configLanguage[0] == '\0') {
        mxui_push_screen(MXUI_SCREEN_LANGUAGE);
    } else if (startup_experience_should_show_first_run()) {
        mxui_push_screen(MXUI_SCREEN_FIRST_RUN);
    }
}

void mxui_open_pause_menu(void) {
    mxui_reset_modal_state();
    mxui_open_screen(MXUI_SCREEN_PAUSE);
    mxui_start_shell_open();
    sMxui.pauseMenu = true;
    sMxui.mainMenu = false;
    gDjuiPanelPauseCreated = true;
    mxui_cursor_set_visible(true);
}

void mxui_close_pause_menu_with_mode(s16 pauseScreenMode) {
    if (!sMxui.rendering && mxui_start_pause_close_animation(pauseScreenMode)) {
        return;
    }
    if (mxui_queue_deferred_action(MXUI_DEFERRED_CLOSE_PAUSE, MXUI_SCREEN_BOOT, 0, pauseScreenMode, NULL, NULL, NULL)) {
        return;
    }
    mxui_close_pause_menu_with_mode_immediate(pauseScreenMode);
}

void mxui_apply_deferred_action(void) {
    enum MxuiDeferredActionType action = sMxui.deferredAction;
    enum MxuiScreenId screenId = sMxui.deferredScreenId;
    s32 tag = sMxui.deferredTag;
    s16 pauseMode = sMxui.deferredPauseMode;
    char confirmTitle[sizeof(sMxui.deferredConfirmTitle)];
    char confirmMessage[sizeof(sMxui.deferredConfirmMessage)];
    MxuiActionCallback confirmYes = sMxui.deferredConfirmYes;

    if (action == MXUI_DEFERRED_NONE) {
        return;
    }

    snprintf(confirmTitle, sizeof(confirmTitle), "%s", sMxui.deferredConfirmTitle);
    snprintf(confirmMessage, sizeof(confirmMessage), "%s", sMxui.deferredConfirmMessage);

    sMxui.deferredAction = MXUI_DEFERRED_NONE;
    sMxui.deferredScreenId = MXUI_SCREEN_BOOT;
    sMxui.deferredTag = 0;
    sMxui.deferredPauseMode = 0;
    sMxui.deferredConfirmTitle[0] = '\0';
    sMxui.deferredConfirmMessage[0] = '\0';
    sMxui.deferredConfirmYes = NULL;

    switch (action) {
        case MXUI_DEFERRED_OPEN_SCREEN:
            mxui_open_screen_with_tag_immediate(screenId, tag);
            break;
        case MXUI_DEFERRED_PUSH_SCREEN:
            mxui_push_screen_with_tag_immediate(screenId, tag);
            break;
        case MXUI_DEFERRED_POP_SCREEN:
            mxui_pop_screen_immediate();
            break;
        case MXUI_DEFERRED_CLEAR:
            mxui_clear_immediate();
            break;
        case MXUI_DEFERRED_OPEN_CONFIRM:
            mxui_open_confirm_immediate(confirmTitle, confirmMessage, confirmYes);
            break;
        case MXUI_DEFERRED_CLOSE_PAUSE:
            if (!mxui_start_pause_close_animation(pauseMode)) {
                mxui_close_pause_menu_with_mode_immediate(pauseMode);
            }
            break;
        case MXUI_DEFERRED_OPEN_CHAR_SELECT:
            if (mxui_is_pause_active()) {
                mxui_close_pause_menu_with_mode_immediate(1);
            } else if (mxui_is_active()) {
                mxui_clear_immediate();
            }
            mxui_char_select_call_bool0("dx_open_browser");
            break;
        case MXUI_DEFERRED_OPEN_CHAR_SELECT_TAB:
            if (mxui_is_pause_active()) {
                mxui_close_pause_menu_with_mode_immediate(1);
            } else if (mxui_is_active()) {
                mxui_clear_immediate();
            }
            mxui_char_select_call_bool1("dx_open_browser_tab", tag);
            break;
        case MXUI_DEFERRED_NONE:
        default:
            break;
    }

    sMxui.input.mousePressed = false;
    sMxui.input.mouseReleased = false;
    sMxui.input.accept = false;
    sMxui.input.back = false;
    sMxui.mouseCaptureValid = false;
    sMxui.pressedRectValid = false;
}

static struct Mod* mxui_local_mod(s32 index) {
    if (index < 0 || index >= gLocalMods.entryCount) { return NULL; }
    return gLocalMods.entries[index];
}

static bool mxui_mod_matches_category(struct Mod* mod) {
    if (mod == NULL) { return false; }
    if (sMxui.modCategory == 0) { return true; }

    const char* category = mod->category != NULL ? mod->category : mod->incompatible;
    if (sMxui.modCategory == 1) {
        if (category == NULL) { return true; }
        for (s32 i = 2; i < (s32)(sizeof(sModCategoryKeys) / sizeof(sModCategoryKeys[0])); i++) {
            if (sModCategoryKeys[i] != NULL && strstr(category, sModCategoryKeys[i]) != NULL) {
                return false;
            }
        }
        return true;
    }

    if (category == NULL) { return false; }
    return strstr(category, sModCategoryKeys[sMxui.modCategory]) != NULL;
}

static void mxui_build_mod_list(struct MxuiModList* outList) {
    memset(outList, 0, sizeof(*outList));
    for (s32 i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* mod = gLocalMods.entries[i];
        if (mod == NULL || !mxui_mod_matches_category(mod)) { continue; }
        outList->indices[outList->count++] = i;
    }
    outList->pageCount = MAX(1, (outList->count + 5) / 6);
}

static void mxui_sanitize_description(const char* source, char* buffer, size_t size, bool truncate) {
    if (size == 0) { return; }
    buffer[0] = '\0';
    const char* fallback = "No description available.";
    const char* clean = (source != NULL && source[0] != '\0') ? source : fallback;

    size_t out = 0;
    bool lastSpace = false;
    size_t sourceIndex = 0;
    for (; clean[sourceIndex] != '\0' && out + 1 < size; sourceIndex++) {
        char c = clean[sourceIndex];
        if (c == '\\') {
            sourceIndex++;
            while (clean[sourceIndex] != '\0' && clean[sourceIndex] != '\\') {
                sourceIndex++;
            }
            lastSpace = false;
            continue;
        }
        bool space = (c == '\n' || c == '\r' || c == '\t' || c == ' ');
        if (space) {
            if (lastSpace || out == 0) { continue; }
            buffer[out++] = ' ';
            lastSpace = true;
            continue;
        }
        buffer[out++] = c;
        lastSpace = false;
        if (truncate && out >= size - 4) {
            break;
        }
    }
    buffer[out] = '\0';
    if (truncate && clean[sourceIndex] != '\0' && out + 3 < size) {
        strcat(buffer, "...");
    }
}

static bool mxui_mod_matches_hook(struct Mod* mod, struct LuaHookedModMenuElement* hooked) {
    if (mod == NULL || hooked == NULL || hooked->mod == NULL) {
        return false;
    }
    if (hooked->mod == mod) {
        return true;
    }
    if (mod->relativePath[0] != '\0' && hooked->mod->relativePath[0] != '\0'
        && strcmp(mod->relativePath, hooked->mod->relativePath) == 0) {
        return true;
    }
    if (mod->name != NULL && hooked->mod->name != NULL
        && strcmp(mod->name, hooked->mod->name) == 0) {
        return true;
    }
    return false;
}

static s32 mxui_mod_primary_action(struct Mod* mod) {
    if (mod == NULL) { return -1; }
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        if (gHookedModMenuElements[i].element == MOD_MENU_ELEMENT_BUTTON
            && mxui_mod_matches_hook(mod, &gHookedModMenuElements[i])) {
            return i;
        }
    }
    return -1;
}

static s32 mxui_mod_bind_count(struct Mod* mod) {
    s32 count = 0;
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        if (gHookedModMenuElements[i].element == MOD_MENU_ELEMENT_BIND
            && mxui_mod_matches_hook(mod, &gHookedModMenuElements[i])) {
            count++;
        }
    }
    return count;
}

static bool mxui_mod_is_size_blocked(struct Mod* mod) {
    return mod != NULL && mod->size >= MAX_MOD_SIZE;
}

static bool mxui_mod_incompatible_token_match(const char* a, const char* b) {
    const char* aStart = a;
    while (aStart != NULL && *aStart != '\0') {
        while (*aStart == ' ') { aStart++; }
        const char* aEnd = strchr(aStart, ' ');
        size_t aLen = (aEnd != NULL) ? (size_t)(aEnd - aStart) : strlen(aStart);
        if (aLen == 0) { break; }

        const char* bStart = b;
        while (bStart != NULL && *bStart != '\0') {
            while (*bStart == ' ') { bStart++; }
            const char* bEnd = strchr(bStart, ' ');
            size_t bLen = (bEnd != NULL) ? (size_t)(bEnd - bStart) : strlen(bStart);
            if (aLen == bLen && bLen > 0 && strncmp(aStart, bStart, aLen) == 0) {
                return true;
            }
            bStart = (bEnd != NULL) ? (bEnd + 1) : NULL;
        }

        aStart = (aEnd != NULL) ? (aEnd + 1) : NULL;
    }
    return false;
}

static bool mxui_mod_conflicts_with(struct Mod* a, struct Mod* b) {
    if (a == NULL || b == NULL || a->incompatible == NULL || b->incompatible == NULL) {
        return false;
    }
    if (a->incompatible[0] == '\0' || b->incompatible[0] == '\0') {
        return false;
    }
    return mxui_mod_incompatible_token_match(a->incompatible, b->incompatible);
}

static s32 mxui_mod_disable_conflicts(struct Mod* mod) {
    s32 disabled = 0;
    if (mod == NULL) { return 0; }
    for (s32 i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* other = mxui_local_mod(i);
        if (other == NULL || other == mod || !other->enabled) {
            continue;
        }
        if (mxui_mod_conflicts_with(mod, other)) {
            other->enabled = false;
            disabled++;
        }
    }
    return disabled;
}

static void mxui_sync_local_mod_runtime(void) {
    mods_profile_save_last_session();
    mods_activate(&gLocalMods);
    smlua_init();
    dynos_behavior_hook_all_custom_behaviors();
    mods_update_selectable();
}

static bool mxui_mod_toggle_enabled(struct Mod* mod) {
    if (mod == NULL) { return false; }
    if (!mod->enabled && mxui_mod_is_size_blocked(mod)) {
        mxui_toast("This mod is too large to enable.", 90);
        return false;
    }
    if (mod->enabled) {
        mod->enabled = false;
    } else {
        s32 disabledConflicts = mxui_mod_disable_conflicts(mod);
        mods_enable(mod->relativePath);
        if (disabledConflicts > 0) {
            mxui_toast("Disabled conflicting mods and enabled this mod.", 90);
        }
    }
    mxui_sync_local_mod_runtime();
    mod_cache_save();
    return true;
}

static const char* mxui_mod_state_label(struct Mod* mod) {
    if (mod == NULL) {
        return "Disabled";
    }
    if (mod->enabled) {
        return "Enabled";
    }
    if (mxui_mod_is_size_blocked(mod)) {
        return "Too Large";
    }
    return mod->selectable ? "Disabled" : "Conflict";
}

static const char* mxui_mod_toggle_label(struct Mod* mod) {
    if (mod == NULL) {
        return "Enable Mod";
    }
    if (mod->enabled) {
        return "Disable Mod";
    }
    return mxui_mod_is_size_blocked(mod) ? "Too Large" : "Enable Mod";
}

static void mxui_mod_invoke_button(s32 hookIndex) {
    if (hookIndex < 0 || hookIndex >= gHookedModMenuElementsCount) { return; }
    smlua_call_mod_menu_element_hook(&gHookedModMenuElements[hookIndex], hookIndex);
}

static bool mxui_char_select_push_table(lua_State* L) {
    if (L == NULL) {
        return false;
    }

    lua_getglobal(L, "charSelect");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

static bool mxui_char_select_push_function(lua_State* L, const char* functionName) {
    if (!mxui_char_select_push_table(L)) {
        return false;
    }

    lua_getfield(L, -1, functionName);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return false;
    }

    lua_remove(L, -2);
    return true;
}

static void mxui_char_select_copy_string_field(lua_State* L, int index, const char* field, char* buffer, size_t bufferSize) {
    int absIndex = lua_absindex(L, index);
    if (bufferSize == 0) {
        return;
    }

    buffer[0] = '\0';
    lua_getfield(L, absIndex, field);
    if (lua_isstring(L, -1)) {
        snprintf(buffer, bufferSize, "%s", lua_tostring(L, -1));
    }
    lua_pop(L, 1);
}

static s32 mxui_char_select_get_int_field(lua_State* L, int index, const char* field, s32 fallback) {
    int absIndex = lua_absindex(L, index);
    s32 value = fallback;
    lua_getfield(L, absIndex, field);
    if (lua_isnumber(L, -1)) {
        value = (s32)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return value;
}

static bool mxui_char_select_call_integer0(const char* functionName, s32* outValue) {
    lua_State* L = gLuaState;
    if (L == NULL || outValue == NULL) {
        return false;
    }

    int top = lua_gettop(L);
    if (!mxui_char_select_push_function(L, functionName)) {
        lua_settop(L, top);
        return false;
    }

    if (smlua_pcall(L, 0, 1, 0) != 0 || !lua_isnumber(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    *outValue = (s32)lua_tointeger(L, -1);
    lua_settop(L, top);
    return true;
}

static bool mxui_char_select_call_bool2(const char* functionName, s32 arg0, s32 arg1) {
    lua_State* L = gLuaState;
    if (L == NULL) {
        return false;
    }

    int top = lua_gettop(L);
    if (!mxui_char_select_push_function(L, functionName)) {
        lua_settop(L, top);
        return false;
    }

    lua_pushinteger(L, arg0);
    lua_pushinteger(L, arg1);
    if (smlua_pcall(L, 2, 1, 0) != 0 || !lua_isboolean(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    bool value = lua_toboolean(L, -1) != 0;
    lua_settop(L, top);
    return value;
}

static bool mxui_char_select_call_bool1(const char* functionName, s32 arg0) {
    lua_State* L = gLuaState;
    if (L == NULL) {
        return false;
    }

    int top = lua_gettop(L);
    if (!mxui_char_select_push_function(L, functionName)) {
        lua_settop(L, top);
        return false;
    }

    lua_pushinteger(L, arg0);
    if (smlua_pcall(L, 1, 1, 0) != 0 || !lua_isboolean(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    bool value = lua_toboolean(L, -1) != 0;
    lua_settop(L, top);
    return value;
}

static bool mxui_char_select_call_bool0(const char* functionName) {
    lua_State* L = gLuaState;
    if (L == NULL) {
        return false;
    }

    int top = lua_gettop(L);
    if (!mxui_char_select_push_function(L, functionName)) {
        lua_settop(L, top);
        return false;
    }

    if (smlua_pcall(L, 0, 1, 0) != 0 || !lua_isboolean(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    bool value = lua_toboolean(L, -1) != 0;
    lua_settop(L, top);
    return value;
}

static void mxui_char_select_call_void0(const char* functionName) {
    lua_State* L = gLuaState;
    if (L == NULL) {
        return;
    }

    int top = lua_gettop(L);
    if (!mxui_char_select_push_function(L, functionName)) {
        lua_settop(L, top);
        return;
    }

    if (smlua_pcall(L, 0, 0, 0) != 0) {
        lua_settop(L, top);
        return;
    }

    lua_settop(L, top);
}

static bool mxui_char_select_bridge_contains(const struct MxuiCharacterSelectBridge* bridge, s32 charNum) {
    if (bridge == NULL) {
        return false;
    }

    for (s32 i = 0; i < bridge->count; i++) {
        if (bridge->characterIndices[i] == charNum) {
            return true;
        }
    }
    return false;
}

static s32 mxui_char_select_bridge_choice_index(const struct MxuiCharacterSelectBridge* bridge, s32 charNum) {
    if (bridge == NULL) {
        return -1;
    }

    for (s32 i = 0; i < bridge->count; i++) {
        if (bridge->characterIndices[i] == charNum) {
            return i;
        }
    }
    return -1;
}

static void mxui_char_select_bridge_populate_variants(lua_State* L, int charIndex, struct MxuiCharacterSelectBridge* bridge, s32 selectedVariant) {
    int absCharIndex = lua_absindex(L, charIndex);
    bridge->variantCount = 0;
    bridge->variantType[0] = '\0';
    bridge->variantSource[0] = '\0';
    bridge->variantCredit[0] = '\0';
    bridge->variantDescription[0] = '\0';
    bridge->variantName[0] = '\0';

    for (s32 alt = 1; alt < MXUI_CHAR_SELECT_MAX_VARIANTS; alt++) {
        lua_pushinteger(L, alt);
        lua_gettable(L, absCharIndex);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            break;
        }

        int variantIndex = lua_absindex(L, -1);
        char variantName[96] = { 0 };
        char variantType[48] = { 0 };
        mxui_char_select_copy_string_field(L, variantIndex, "name", variantName, sizeof(variantName));
        mxui_char_select_copy_string_field(L, variantIndex, "variantType", variantType, sizeof(variantType));

        if (variantName[0] == '\0') {
            snprintf(variantName, sizeof(variantName), "Variant %d", alt);
        }

        if (variantType[0] != '\0' && strcmp(variantType, "base") != 0 && strcmp(variantType, "palette") != 0) {
            snprintf(bridge->variantLabels[bridge->variantCount], sizeof(bridge->variantLabels[bridge->variantCount]), "%s (%s)", variantName, variantType);
        } else {
            snprintf(bridge->variantLabels[bridge->variantCount], sizeof(bridge->variantLabels[bridge->variantCount]), "%s", variantName);
        }
        bridge->variantCount++;

        if (alt == selectedVariant) {
            snprintf(bridge->variantName, sizeof(bridge->variantName), "%s", variantName);
            snprintf(bridge->variantType, sizeof(bridge->variantType), "%s", variantType);
            mxui_char_select_copy_string_field(L, variantIndex, "variantSource", bridge->variantSource, sizeof(bridge->variantSource));
            mxui_char_select_copy_string_field(L, variantIndex, "credit", bridge->variantCredit, sizeof(bridge->variantCredit));
            mxui_char_select_copy_string_field(L, variantIndex, "description", bridge->variantDescription, sizeof(bridge->variantDescription));
        }

        lua_pop(L, 1);
    }
}

static bool mxui_char_select_refresh_bridge(struct MxuiCharacterSelectBridge* bridge, s32 requestedCharacter, s32 requestedVariant) {
    lua_State* L = gLuaState;
    if (bridge == NULL || L == NULL) {
        return false;
    }

    memset(bridge, 0, sizeof(*bridge));
    int baseTop = lua_gettop(L);

    if (!mxui_char_select_push_function(L, "character_get_full_table")) {
        return false;
    }
    if (smlua_pcall(L, 0, 1, 0) != 0 || !lua_istable(L, -1)) {
        lua_settop(L, baseTop);
        return false;
    }

    int fullTableIndex = lua_absindex(L, -1);

    bridge->available = true;
    mxui_char_select_call_integer0("character_get_current_number", &bridge->currentCharacter);
    mxui_char_select_call_integer0("character_get_current_costume", &bridge->currentVariant);

    for (s32 charNum = 0; charNum < MXUI_CHAR_SELECT_MAX_CHARACTERS && bridge->count < MXUI_CHAR_SELECT_MAX_CHARACTERS; charNum++) {
        lua_pushinteger(L, charNum);
        lua_gettable(L, fullTableIndex);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            continue;
        }

        if (mxui_char_select_get_int_field(L, -1, "locked", 0) == 1) {
            lua_pop(L, 1);
            continue;
        }

        char label[96] = { 0 };
        mxui_char_select_copy_string_field(L, -1, "nickname", label, sizeof(label));
        if (label[0] == '\0') {
            lua_pushinteger(L, 1);
            lua_gettable(L, -2);
            if (lua_istable(L, -1)) {
                mxui_char_select_copy_string_field(L, -1, "name", label, sizeof(label));
            }
            lua_pop(L, 1);
        }
        if (label[0] == '\0') {
            snprintf(label, sizeof(label), "Character %d", charNum);
        }

        bridge->characterIndices[bridge->count] = charNum;
        snprintf(bridge->characterLabels[bridge->count], sizeof(bridge->characterLabels[bridge->count]), "%s", label);
        bridge->count++;
        lua_pop(L, 1);
    }

    s32 selectedCharacter = requestedCharacter;
    if (!mxui_char_select_bridge_contains(bridge, selectedCharacter)) {
        selectedCharacter = bridge->currentCharacter;
    }
    if (!mxui_char_select_bridge_contains(bridge, selectedCharacter) && bridge->count > 0) {
        selectedCharacter = bridge->characterIndices[0];
    }
    if (selectedCharacter < 0 || bridge->count <= 0) {
        lua_settop(L, baseTop);
        bridge->available = false;
        return false;
    }

    lua_pushinteger(L, selectedCharacter);
    lua_gettable(L, fullTableIndex);
    if (!lua_istable(L, -1)) {
        lua_settop(L, baseTop);
        bridge->available = false;
        return false;
    }

    int charIndex = lua_absindex(L, -1);
    s32 selectedVariant = requestedVariant > 0 ? requestedVariant : bridge->currentVariant;
    s32 maxVariant = 1;
    for (s32 alt = 1; alt < MXUI_CHAR_SELECT_MAX_VARIANTS; alt++) {
        lua_pushinteger(L, alt);
        lua_gettable(L, charIndex);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        maxVariant = alt;
        lua_pop(L, 1);
    }
    selectedVariant = MAX(1, MIN(selectedVariant, maxVariant));

    mxui_char_select_copy_string_field(L, charIndex, "nickname", bridge->characterName, sizeof(bridge->characterName));
    if (bridge->characterName[0] == '\0') {
        s32 choiceIndex = mxui_char_select_bridge_choice_index(bridge, selectedCharacter);
        if (choiceIndex >= 0) {
            snprintf(bridge->characterName, sizeof(bridge->characterName), "%s", bridge->characterLabels[choiceIndex]);
        }
    }
    mxui_char_select_copy_string_field(L, charIndex, "category", bridge->characterCategory, sizeof(bridge->characterCategory));
    mxui_char_select_copy_string_field(L, charIndex, "saveName", bridge->characterSaveName, sizeof(bridge->characterSaveName));
    mxui_char_select_bridge_populate_variants(L, charIndex, bridge, selectedVariant);
    lua_settop(L, baseTop);
    return true;
}

bool mxui_open_character_select_menu(void) {
    if (mxui_is_main_menu_active()) {
        return false;
    }
    if (mxui_is_active()) {
        if (mxui_queue_deferred_action(MXUI_DEFERRED_OPEN_CHAR_SELECT, MXUI_SCREEN_BOOT, 0, 0, NULL, NULL, NULL)) {
            return true;
        }
        if (mxui_is_pause_active()) {
            mxui_close_pause_menu_with_mode_immediate(1);
        } else {
            mxui_clear_immediate();
        }
    }
    return mxui_char_select_call_bool0("dx_open_browser");
}

static bool mxui_open_character_select_menu_tab(s32 tab) {
    if (mxui_is_main_menu_active()) {
        return false;
    }
    if (mxui_is_active()) {
        if (mxui_queue_deferred_action(MXUI_DEFERRED_OPEN_CHAR_SELECT_TAB, MXUI_SCREEN_BOOT, tab, 0, NULL, NULL, NULL)) {
            return true;
        }
        if (mxui_is_pause_active()) {
            mxui_close_pause_menu_with_mode_immediate(1);
        } else {
            mxui_clear_immediate();
        }
    }
    return mxui_char_select_call_bool1("dx_open_browser_tab", tab);
}

static bool mxui_reset_character_colors(void) {
    return mxui_char_select_call_bool0("dx_reset_colors");
}

static void mxui_render_mod_card(struct MxuiRect card, struct Mod* mod, s32 modIndex, struct MxuiScreenState* screen) {
    struct MxuiTheme theme = mxui_theme();
    if (mod == NULL) { return; }

    bool hovered = false;
    bool focused = mxui_focusable(card, &hovered);
    if (hovered || focused) {
        screen->tag = modIndex;
    }

    mxui_skin_draw_panel(
        card,
        screen->tag == modIndex ? theme.panelAlt : theme.panel,
        hovered || focused ? theme.buttonHover : theme.border,
        hovered || focused ? theme.glow : mxui_color(0, 0, 0, 0),
        14.0f,
        (hovered || focused) ? 2.0f : 1.0f
    );

    char desc[160] = { 0 };
    char status[96] = { 0 };
    mxui_sanitize_description(mod->description, desc, sizeof(desc), true);
    snprintf(status, sizeof(status), "%s | %s",
        mod->category != NULL && mod->category[0] != '\0' ? mod->category : "Misc",
        mxui_mod_state_label(mod));

    mxui_draw_text_box_fitted(mod->name != NULL ? mod->name : "Unnamed Mod",
        (struct MxuiRect){ card.x + 14.0f, card.y + 12.0f, card.w - 28.0f, 24.0f },
        0.58f, 0.40f, FONT_MENU, theme.title, MXUI_TEXT_LEFT, true, 2);
    mxui_draw_text_box_fitted(status,
        (struct MxuiRect){ card.x + 14.0f, card.y + 38.0f, card.w - 28.0f, 16.0f },
        0.50f, 0.38f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, false, 1);
    mxui_draw_text_box_fitted(desc,
        (struct MxuiRect){ card.x + 14.0f, card.y + 58.0f, card.w - 28.0f, MAX(16.0f, card.h - 118.0f) },
        0.46f, 0.36f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 3);

    s32 primaryAction = mxui_mod_primary_action(mod);
    const char* primaryLabel = (primaryAction >= 0) ? "Open" : "Details";
    struct MxuiRowPair actionRow = {
        .left = { card.x + 14.0f, card.y + card.h - 38.0f, (card.w - 38.0f) * 0.5f, 24.0f },
        .right = { card.x + 20.0f + (card.w - 38.0f) * 0.5f, card.y + card.h - 38.0f, (card.w - 38.0f) * 0.5f, 24.0f },
    };
    if (mxui_button(actionRow.left, primaryLabel, false)) {
        if (primaryAction >= 0) {
            mxui_mod_invoke_button(primaryAction);
        } else {
            mxui_push_screen_with_tag(MXUI_SCREEN_MOD_DETAILS, modIndex);
        }
    }
    if (mxui_button(actionRow.right, mxui_mod_toggle_label(mod), mod->enabled)) {
        if (mxui_mod_toggle_enabled(mod)) {
            mxui_toast(mod->enabled ? "Mod enabled." : "Mod disabled.", 60);
        }
    }
}

static void mxui_render_mod_detail_panel(struct MxuiRect panel, struct Mod* mod, s32 modIndex) {
    struct MxuiTheme theme = mxui_theme();
    mxui_skin_draw_panel(panel, theme.panel, theme.border, mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 22), 14.0f, 1.0f);

    if (mod == NULL) {
        mxui_draw_text_box_fitted("Pick a mod card to see its details and actions.", (struct MxuiRect){ panel.x + 18.0f, panel.y + 24.0f, panel.w - 36.0f, panel.h - 48.0f }, 0.58f, 0.42f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 4);
        return;
    }

    char desc[MOD_DESCRIPTION_MAX_LENGTH + 32] = { 0 };
    char summary[256] = { 0 };
    s32 primaryAction = mxui_mod_primary_action(mod);
    s32 bindCount = 0;
    s32 settingCount = 0;
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        if (!mxui_mod_matches_hook(mod, hooked)) { continue; }
        settingCount++;
        if (hooked->element == MOD_MENU_ELEMENT_BIND) {
            bindCount++;
        }
    }

    mxui_sanitize_description(mod->description, desc, sizeof(desc), false);
    snprintf(summary, sizeof(summary), "%s | %s | %d settings | %d hotkeys",
        mod->category != NULL && mod->category[0] != '\0' ? mod->category : "Misc",
        mxui_mod_state_label(mod),
        settingCount,
        bindCount);

    mxui_draw_text_box_fitted(mod->name != NULL ? mod->name : "Unnamed Mod", (struct MxuiRect){ panel.x + 18.0f, panel.y + 18.0f, panel.w - 36.0f, 34.0f }, 0.68f, 0.42f, FONT_MENU, theme.title, MXUI_TEXT_LEFT, true, 2);
    mxui_draw_text_box_fitted(summary, (struct MxuiRect){ panel.x + 18.0f, panel.y + 54.0f, panel.w - 36.0f, 22.0f }, 0.52f, 0.38f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 2);
    mxui_draw_text_box_fitted(desc, (struct MxuiRect){ panel.x + 18.0f, panel.y + 84.0f, panel.w - 36.0f, MAX(80.0f, panel.h - 176.0f) }, 0.52f, 0.38f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 8);

    struct MxuiRect actionA = { panel.x + 18.0f, panel.y + panel.h - 82.0f, panel.w - 36.0f, 28.0f };
    struct MxuiRect actionB = { panel.x + 18.0f, panel.y + panel.h - 46.0f, panel.w - 36.0f, 28.0f };
    if (primaryAction >= 0) {
        if (mxui_button(actionA, "Open", false)) {
            mxui_mod_invoke_button(primaryAction);
        }
    } else {
        if (mxui_button(actionA, "Details", false)) {
            mxui_push_screen_with_tag(MXUI_SCREEN_MOD_DETAILS, modIndex);
        }
    }
    if (mxui_button(actionB, mxui_mod_toggle_label(mod), mod->enabled)) {
        if (mxui_mod_toggle_enabled(mod)) {
            mxui_toast(mod->enabled ? "Mod enabled." : "Mod disabled.", 60);
        }
    }
}

static void mxui_load_languages(void) {
    sMxui.languageChoiceCount = 0;

    char lpath[SYS_MAX_PATH] = { 0 };
    snprintf(lpath, sizeof(lpath), "%s/lang", sys_resource_path());

    DIR* dir = opendir(lpath);
    if (dir == NULL) {
        return;
    }

    struct dirent* entry = NULL;
    while ((entry = readdir(dir)) != NULL && sMxui.languageChoiceCount < MXUI_MAX_LANGUAGES) {
        char name[64] = { 0 };
        snprintf(name, sizeof(name), "%s", entry->d_name);
        char* dot = strchr(name, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        if (name[0] == '\0') { continue; }
        bool duplicate = false;
        for (s32 i = 0; i < sMxui.languageChoiceCount; i++) {
            if (strcmp(sMxui.languageChoices[i], name) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            snprintf(sMxui.languageChoices[sMxui.languageChoiceCount++], sizeof(sMxui.languageChoices[0]), "%s", name);
        }
    }
    closedir(dir);
}

static void mxui_set_language(const char* language) {
    if (language == NULL || language[0] == '\0') { return; }
    snprintf(configLanguage, MAX_CONFIG_STRING, "%s", language);
    if (!mxui_language_init(configLanguage)) {
        snprintf(configLanguage, MAX_CONFIG_STRING, "%s", "English");
        mxui_language_init(configLanguage);
    }
    smlua_call_event_hooks(HOOK_ON_LANGUAGE_CHANGED, configLanguage);
    newcam_init_settings();
}

static void mxui_render_settings_hub(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiRowPair row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Controls", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_CONTROLS); }
    if (mxui_button(row.right, "Accessibility", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_ACCESSIBILITY); }
    row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Display", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_DISPLAY); }
    if (mxui_button(row.right, "Sound", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_SOUND); }
    row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Camera", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_CAMERA); }
    if (mxui_button(row.right, "Mods", false)) { mxui_push_screen(MXUI_SCREEN_MODS); }
    row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Player", false)) { mxui_push_screen(MXUI_SCREEN_PLAYER); }
    if (mxui_button(row.right, "DynOS Packs", false)) { mxui_push_screen(MXUI_SCREEN_DYNOS); }
    if (mxui_button(mxui_stack_next_row(ctx, 42.0f), "Misc", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_MISC); }
}

static void mxui_render_settings_display(struct MxuiContext* ctx) {
    const char* framerateChoices[] = { "Auto", "Manual", "Unlimited" };
    const char* interpChoices[] = { "Fast", "Accurate" };
    const char* filterChoices[] = { "Nearest", "Linear", "Tripoint" };
    const char* drawChoices[] = { "0.5x", "1x", "1.5x", "3x", "10x", "100x" };

    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Window", "Video output and presentation options.", 4);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Fullscreen", &configWindow.fullscreen);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Force 4:3", &configForce4By3);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Show FPS", &configShowFPS);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "VSync", &configWindow.vsync);

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Framerate", "Frame cap and interpolation.", 3);
    if (mxui_select_u32(mxui_section_next_row(&section2, 46.0f), "Mode", framerateChoices, 3, (unsigned int*)&configFramerateMode)) {
        if (configFramerateMode == RRM_AUTO && configFrameLimit < 30) {
            configFrameLimit = 60;
        }
    }
    if (configFrameLimit < 30) { configFrameLimit = 30; }
    mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "Frame Limit", &configFrameLimit, 30, 300, 10);
    mxui_select_u32(mxui_section_next_row(&section2, 46.0f), "Interpolation", interpChoices, 2, &configInterpolationMode);

    struct MxuiSectionLayout section3 = mxui_section_begin(ctx, "Rendering", "Filtering, anti-aliasing, and draw distance.", 3);
    mxui_select_u32(mxui_section_next_row(&section3, 46.0f), "Filtering", filterChoices, 3, &configFiltering);
    {
        static const char* msaaChoices[] = { "Off", "2x", "4x", "8x", "16x" };
        unsigned int msaaSelection = 0;
        if (configWindow.msaa >= 16) msaaSelection = 4;
        else if (configWindow.msaa >= 8) msaaSelection = 3;
        else if (configWindow.msaa >= 4) msaaSelection = 2;
        else if (configWindow.msaa >= 2) msaaSelection = 1;
        if (mxui_select_u32(mxui_section_next_row(&section3, 46.0f), "Anti-Aliasing", msaaChoices, 5, &msaaSelection)) {
            configWindow.msaa = (msaaSelection == 0) ? 0 : (1u << msaaSelection);
            configWindow.settings_changed = true;
        }
    }
    mxui_select_u32(mxui_section_next_row(&section3, 46.0f), "Draw Distance", drawChoices, 6, &configDrawDistance);
}

static void mxui_render_settings_sound(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Levels", "Volume balance for music, effects, and ambience.", 4);
    if (mxui_slider_u32(mxui_section_next_row(&section1, 46.0f), "Master Volume", &configMasterVolume, 0, 127, 1)) { audio_custom_update_volume(); }
    if (mxui_slider_u32(mxui_section_next_row(&section1, 46.0f), "Music Volume", &configMusicVolume, 0, 127, 1)) { audio_custom_update_volume(); }
    if (mxui_slider_u32(mxui_section_next_row(&section1, 46.0f), "Sound Effects", &configSfxVolume, 0, 127, 1)) { audio_custom_update_volume(); }
    if (mxui_slider_u32(mxui_section_next_row(&section1, 46.0f), "Environment Volume", &configEnvVolume, 0, 127, 1)) { audio_custom_update_volume(); }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Playback", "Distance and background window handling.", 2);
    mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Fade Out Distant Sounds", &configFadeoutDistantSounds);
    mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Mute When Window Unfocused", &configMuteFocusLoss);
}

static void mxui_render_settings_controls(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiSectionLayout sec1 = mxui_section_begin(ctx, "Bindings", "Core controls and mod hotkeys.", 3);
    if (mxui_button(mxui_section_next_row(&sec1, 46.0f), "N64 Binds", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_CONTROLS_N64); }
    if (mxui_button(mxui_section_next_row(&sec1, 46.0f), "Hotkeys", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_HOTKEYS); }
    if (mxui_button(mxui_section_next_row(&sec1, 46.0f), "Camera / Analog", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_CAMERA); }

    struct MxuiSectionLayout sec2 = mxui_section_begin(ctx, "Devices", "Controller activation and pad selection.", 3);
    mxui_toggle(mxui_section_next_row(&sec2, 46.0f), "Background Gamepad", &configBackgroundGamepad);
#ifndef HANDHELD
    mxui_toggle(mxui_section_next_row(&sec2, 46.0f), "Disable Gamepads", &configDisableGamepads);
#else
    (void)sec2;
#endif
    mxui_slider_u32(mxui_section_next_row(&sec2, 46.0f), "Gamepad Number", &configGamepadNumber, 0, 8, 1);

    struct MxuiSectionLayout sec3 = mxui_section_begin(ctx, "Comfort", "Analog deadzone and rumble strength.", 2);
    if (mxui_slider_u32(mxui_section_next_row(&sec3, 46.0f), "Stick Deadzone", &configStickDeadzone, 0, 100, 1)) { controller_reconfigure(); }
    if (mxui_slider_u32(mxui_section_next_row(&sec3, 46.0f), "Rumble Strength", &configRumbleStrength, 0, 100, 1)) { controller_reconfigure(); }
}

static void mxui_render_settings_controls_n64(struct MxuiContext* ctx) {
    struct {
        const char* label;
        unsigned int* bind;
    } binds[] = {
        { "Move Up", configKeyStickUp }, { "Move Down", configKeyStickDown }, { "Move Left", configKeyStickLeft }, { "Move Right", configKeyStickRight },
        { "A Button", configKeyA }, { "B Button", configKeyB }, { "Start Button", configKeyStart }, { "L Trigger", configKeyL },
        { "R Trigger", configKeyR }, { "Z Trigger", configKeyZ }, { "C-Up", configKeyCUp }, { "C-Down", configKeyCDown }, { "C-Left", configKeyCLeft }, { "C-Right", configKeyCRight },
    };
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section = mxui_section_begin(ctx, "Gameplay Binds", "Edit the core gameplay button layout.", (s32)(sizeof(binds) / sizeof(binds[0])));
    for (s32 i = 0; i < (s32)(sizeof(binds) / sizeof(binds[0])); i++) {
        unsigned int defaults[MAX_BINDS];
        memcpy(defaults, binds[i].bind, sizeof(defaults));
        mxui_bind_row(mxui_section_next_row(&section, 48.0f), binds[i].label, binds[i].bind, defaults, NULL, "", -1);
    }
}

static s32 mxui_hotkey_conflicts(const unsigned int bindValue[MAX_BINDS], const unsigned int* ignoreBind) {
    s32 conflicts = 0;
    for (s32 i = 0; i < (s32)(sizeof(sCoreHotkeys) / sizeof(sCoreHotkeys[0])); i++) {
        if (sCoreHotkeys[i].configKey == ignoreBind) { continue; }
        if (mxui_bind_overlap(bindValue, sCoreHotkeys[i].configKey)) { conflicts++; }
    }
    for (s32 i = 0; i < (s32)(sizeof(sGameplayBindings) / sizeof(sGameplayBindings[0])); i++) {
        if (sGameplayBindings[i].configKey == ignoreBind) { continue; }
        if (mxui_bind_overlap(bindValue, sGameplayBindings[i].configKey)) { conflicts++; }
    }
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        if (hooked->element != MOD_MENU_ELEMENT_BIND || hooked->bindValue == ignoreBind) { continue; }
        if (mxui_bind_overlap(bindValue, hooked->bindValue)) { conflicts++; }
    }
    return conflicts;
}

static void mxui_render_settings_hotkeys(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    s32 conflictCount = 0;
    for (s32 i = 0; i < (s32)(sizeof(sCoreHotkeys) / sizeof(sCoreHotkeys[0])); i++) {
        if (mxui_hotkey_conflicts(sCoreHotkeys[i].configKey, sCoreHotkeys[i].configKey) > 0) {
            conflictCount++;
        }
    }
    char conflictText[160] = { 0 };
    if (conflictCount == 0) {
        snprintf(conflictText, sizeof(conflictText), "No hotkey conflicts detected.\nDX shortcuts and mod actions are clear of gameplay controls.");
    } else {
        snprintf(conflictText, sizeof(conflictText), "%d hotkey overlap%s detected.\nAdjust conflicting actions below.", conflictCount, conflictCount == 1 ? "" : "s");
    }
    mxui_section_begin(ctx, "Status", conflictText, 0);

    struct MxuiSectionLayout core = mxui_section_begin(ctx, "Core Hotkeys", "MXUI navigation, page shortcuts, and extra controller actions.", (s32)(sizeof(sCoreHotkeys) / sizeof(sCoreHotkeys[0])));
    for (s32 i = 0; i < (s32)(sizeof(sCoreHotkeys) / sizeof(sCoreHotkeys[0])); i++) {
        mxui_bind_row(mxui_section_next_row(&core, 48.0f), sCoreHotkeys[i].label, sCoreHotkeys[i].configKey, sCoreHotkeys[i].defaults, NULL, "", -1);
    }

    s32 modBindCount = 0;
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        if (hooked->element == MOD_MENU_ELEMENT_BIND && hooked->mod != NULL) {
            modBindCount++;
        }
    }
    if (modBindCount > 0) {
        struct MxuiSectionLayout mods = mxui_section_begin(ctx, "Mod Hotkeys", "Active mod actions that expose bindable shortcuts.", modBindCount);
        for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
            struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
            if (hooked->element != MOD_MENU_ELEMENT_BIND || hooked->mod == NULL) { continue; }
            char label[160] = { 0 };
            snprintf(label, sizeof(label), "%s: %s", hooked->mod->name, hooked->name);
            mxui_bind_row(mxui_section_next_row(&mods, 48.0f), label, hooked->bindValue, hooked->defaultBindValue, hooked->mod, hooked->configKey, i);
        }
    } else {
        mxui_section_begin(ctx, "Mod Hotkeys", "No active mods are currently exposing bindable hotkeys.", 0);
    }
}

static const char* sMenuLevelChoices[] = {
    "CG", "BOB", "WF", "WMOTR", "JRB", "SSL", "TTM", "SL", "BBH",
    "LLL", "THI", "HMC", "CCM", "RR", "BITDW", "PSS", "TTC", "WDW"
};

static const char* sMenuSoundChoices[] = {
    "Title Screen",
    "File Select",
    "Grass",
    "Water",
    "Snow",
    "Slide",
    "Bowser Stage",
    "Bowser Fight",
    "Spooky",
    "Hot",
    "Underground",
    "Bowser Finale",
    "Staff Roll",
    "Stage Music",
};

static void mxui_open_user_folder(void) {
#if defined(_WIN32) || defined(_WIN64)
    ShellExecuteA(NULL, "open", fs_get_write_path(""), NULL, NULL, SW_SHOWNORMAL);
#elif __linux__
    char command[512] = { 0 };
    snprintf(command, sizeof(command), "xdg-open \"%s\"", fs_get_write_path(""));
    system(command);
#elif __APPLE__
    char command[512] = { 0 };
    snprintf(command, sizeof(command), "open \"%s\"", fs_get_write_path(""));
    system(command);
#else
    UNUSED(caller);
#endif
}

static void mxui_mark_theme_changed(void) {
    smlua_call_event_hooks(HOOK_ON_DJUI_THEME_CHANGED);
}

static void mxui_mark_accessibility_custom(void) {
    configAccessibilityPreset = ACCESSIBILITY_PRESET_CUSTOM;
}

static void mxui_apply_accessibility_preset(void) {
    if (configAccessibilityPreset <= ACCESSIBILITY_PRESET_FOCUSED) {
        startup_experience_apply_accessibility_preset(configAccessibilityPreset);
    }
}

static void mxui_refresh_local_player_models(void) {
    for (s32 i = 0; i < MAX_PLAYERS; i++) {
        network_player_update_model(i);
    }
}

static struct Mod* mxui_current_mod_from_tag(s32 tag) {
    if (tag < 0 || tag >= gLocalMods.entryCount) {
        return NULL;
    }
    return gLocalMods.entries[tag];
}

static void mxui_render_settings_camera(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Modes", "Choose a camera style or open detailed tuning pages.", 2);
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Free Camera", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_FREE_CAMERA);
    }
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Romhack Camera", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_ROMHACK_CAMERA);
    }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Inversion", "Flip the global camera axes.", 2);
    mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Invert X", &configCameraInvertX);
    mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Invert Y", &configCameraInvertY);
}

static void mxui_render_settings_free_camera(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Mode", "Switch free camera on and control how it behaves.", 6);
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Enable Free Camera", &configEnableFreeCamera)) { newcam_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Analog Camera", &configFreeCameraAnalog)) { newcam_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "L-Centering", &configFreeCameraLCentering)) { newcam_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Use D-Pad", &configFreeCameraDPadBehavior)) { newcam_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Collision", &configFreeCameraHasCollision)) { newcam_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Mouse Look", &configFreeCameraMouse)) { newcam_init_settings(); }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Tuning", "Sensitivity and movement smoothing.", 5);
    if (mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "X Sensitivity", &configFreeCameraXSens, 1, 100, 1)) { newcam_init_settings(); }
    if (mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "Y Sensitivity", &configFreeCameraYSens, 1, 100, 1)) { newcam_init_settings(); }
    if (mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "Aggression", &configFreeCameraAggr, 0, 100, 1)) { newcam_init_settings(); }
    if (mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "Pan Level", &configFreeCameraPan, 0, 100, 1)) { newcam_init_settings(); }
    if (mxui_slider_u32(mxui_section_next_row(&section2, 46.0f), "Deceleration", &configFreeCameraDegrade, 0, 100, 1)) { newcam_init_settings(); }
}

static void mxui_render_settings_romhack_camera(struct MxuiContext* ctx) {
    const char* choices[] = { "Automatic", "On", "Off" };
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section = mxui_section_begin(ctx, "Rules", "Classic romhack camera behavior and stage overrides.", 7);
    if (mxui_select_u32(mxui_section_next_row(&section, 46.0f), "Camera Mode", choices, 3, &configEnableRomhackCamera)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "Bowser Fights", &configRomhackCameraBowserFights)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "Collision", &configRomhackCameraHasCollision)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "L-Centering", &configRomhackCameraHasCentering)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "Use D-Pad", &configRomhackCameraDPadBehavior)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "Slow Fall", &configRomhackCameraSlowFall)) { romhack_camera_init_settings(); }
    if (mxui_toggle(mxui_section_next_row(&section, 46.0f), "Camera Toxic Gas", &configCameraToxicGas)) { romhack_camera_init_settings(); }
}

static void mxui_render_settings_accessibility(struct MxuiContext* ctx) {
    const char* presetChoices[] = { "Classic", "Comfort", "Focused", "Custom" };
    const char* menuSizeChoices[] = { "Auto", "x0.5", "x0.85", "x1.0", "x1.5" };

    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Presets", "Quickly swap between curated comfort setups.", 2);
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Comfort Preset", presetChoices, 4, &configAccessibilityPreset)) {
        mxui_apply_accessibility_preset();
    }
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Menu Size", menuSizeChoices, 5, &configDjuiScale)) {
        mxui_mark_accessibility_custom();
    }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Comfort", "Reduce motion, visual noise, and interruptions.", 5);
    if (mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Reduce Camera Shake", &configReduceCameraShake)) { mxui_mark_accessibility_custom(); }
    if (mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Reduce HUD Flashing", &configReduceHudFlash)) { mxui_mark_accessibility_custom(); }
    if (mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Skip Intro Cutscenes", &configSkipIntro)) { mxui_mark_accessibility_custom(); }
    if (mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Allow Pause Anywhere", &configPauseAnywhere)) { mxui_mark_accessibility_custom(); }
    if (mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Reduce Popups", &configDisablePopups)) { mxui_mark_accessibility_custom(); }

    struct MxuiSectionLayout section3 = mxui_section_begin(ctx, "Input Comfort", "Tune analog sensitivity and rumble strength.", 2);
    if (mxui_slider_u32(mxui_section_next_row(&section3, 46.0f), "Stick Deadzone", &configStickDeadzone, 0, 100, 1)) { mxui_mark_accessibility_custom(); controller_reconfigure(); }
    if (mxui_slider_u32(mxui_section_next_row(&section3, 46.0f), "Rumble Strength", &configRumbleStrength, 0, 100, 1)) { mxui_mark_accessibility_custom(); controller_reconfigure(); }
}

static void mxui_render_settings_misc(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Session", "Small quality-of-life toggles for menus and popups.", 2);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Show Ping", &configShowPing);
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Disable Popups", &configDisablePopups);

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Tools", "Open secondary menus and support information.", 4);
    if (mxui_button(mxui_section_next_row(&section2, 46.0f), "Language", false)) { mxui_push_screen(MXUI_SCREEN_LANGUAGE); }
    if (mxui_button(mxui_section_next_row(&section2, 46.0f), "Menu Options", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_MENU_OPTIONS); }
    if (mxui_button(mxui_section_next_row(&section2, 46.0f), "Information", false)) { mxui_push_screen(MXUI_SCREEN_INFO); }
    if (mxui_button(mxui_section_next_row(&section2, 46.0f), "Open User Folder", false)) { mxui_open_user_folder(); }
}

static void mxui_render_settings_menu_options(struct MxuiContext* ctx) {
    const char* fontChoices[] = { "Normal", "Aliased" };
    const char* scaleChoices[] = { "Auto", "x0.5", "x0.85", "x1.0", "x1.5" };
    unsigned int levelChoice = configMenuLevel;
    unsigned int soundChoice = configMenuSound;
    bool themeChanged = false;
    const s32 themeCount = (s32)(sizeof(sMxuiThemeLabels) / sizeof(sMxuiThemeLabels[0]));
    if (configDjuiTheme >= (u32)themeCount) {
        configDjuiTheme = 0;
    }

    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Themes", "Pick a course-inspired shell, menu scale, gradients, and font rendering.", 6);
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Centered Shell", &configDjuiThemeCenter)) { themeChanged = true; }
    if (mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Gradients", &configDjuiThemeGradients)) { themeChanged = true; }
    mxui_toggle(mxui_section_next_row(&section1, 46.0f), "Smooth Scrolling", &configSmoothScrolling);
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Themes", sMxuiThemeLabels, themeCount, &configDjuiTheme)) { themeChanged = true; }
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Menu Size", scaleChoices, 5, &configDjuiScale)) { themeChanged = true; }
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Font", fontChoices, 2, &configDjuiThemeFont)) { themeChanged = true; }

    if (gDjuiInMainMenu) {
        s32 soundCount = (s32)(sizeof(sMenuSoundChoices) / sizeof(sMenuSoundChoices[0]));
        if (configMenuStaffRoll && soundChoice >= (u32)(soundCount - 1)) {
            soundChoice = 0;
            configMenuSound = 0;
        }

        struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Front End", "Choose the title backdrop and music.", 4);
        mxui_select_u32(mxui_section_next_row(&section2, 46.0f), "Background Stage", sMenuLevelChoices, 18, &levelChoice);
        mxui_select_u32(mxui_section_next_row(&section2, 46.0f), "Background Music", sMenuSoundChoices, soundCount - (configMenuStaffRoll ? 1 : 0), &soundChoice);
        mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Staff Roll Mode", &configMenuStaffRoll);
        mxui_toggle(mxui_section_next_row(&section2, 46.0f), "Random Stage", &configMenuRandom);
        configMenuLevel = MIN(levelChoice, 17);
        configMenuSound = soundChoice;
    }

    if (themeChanged) {
        mxui_mark_theme_changed();
    }
}

static void mxui_render_mod_details(struct MxuiContext* ctx, s32 modIndex) {
    struct Mod* mod = mxui_current_mod_from_tag(modIndex);
    struct MxuiTheme theme = mxui_theme();
    if (mod == NULL) {
        mxui_draw_text_box_fitted("That mod is no longer available.", (struct MxuiRect){ ctx->content.x, ctx->content.y + 20, ctx->content.w, 48 }, 0.56f, 0.38f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 2);
        return;
    }

    mxui_content_reset(ctx);
    char desc[MOD_DESCRIPTION_MAX_LENGTH + 32] = { 0 };
    mxui_sanitize_description(mod->description, desc, sizeof(desc), false);

    struct MxuiSectionLayout hero = mxui_section_begin(ctx, mod->name != NULL ? mod->name : "Mod", "Full mod details and supported actions.", 3);
    char summary[256] = { 0 };
    snprintf(summary, sizeof(summary), "%s | %s | %s",
        mod->category != NULL && mod->category[0] != '\0' ? mod->category : "Misc",
        mod->enabled ? "Enabled" : "Disabled",
        mod->selectable ? "Selectable" : "Required");
    struct MxuiRect summaryRect = mxui_section_next_row(&hero, 58.0f);
    mxui_draw_text_box_fitted(summary, summaryRect, 0.56f, 0.42f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 2);
    struct MxuiRect descRect = mxui_section_next_row(&hero, 72.0f);
    mxui_draw_text_box_fitted(desc, descRect, 0.54f, 0.40f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 4);

    struct MxuiRowPair actions = mxui_stack_next_split_row(ctx, 40.0f, 16.0f);
    if (mxui_button(actions.left, mod->enabled ? "Disable Mod" : "Enable Mod", mod->enabled)) {
        if (mxui_mod_toggle_enabled(mod)) {
            mods_update_selectable();
            mxui_toast(mod->enabled ? "Mod enabled." : "Mod disabled.", 60);
        }
    }
    if (mxui_mod_primary_action(mod) >= 0 && mxui_button(actions.right, "Open", false)) {
        mxui_mod_invoke_button(mxui_mod_primary_action(mod));
    }

    s32 controlCount = 0;
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        if (mxui_mod_matches_hook(mod, &gHookedModMenuElements[i])) {
            controlCount++;
        }
    }
    struct MxuiSectionLayout controls = mxui_section_begin(ctx, "Mod Controls", "Settings and actions exposed by this mod.", controlCount > 0 ? controlCount : 0);
    for (s32 i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        if (!mxui_mod_matches_hook(mod, hooked)) { continue; }

        struct MxuiRect row = mxui_section_next_row(&controls, hooked->element == MOD_MENU_ELEMENT_INPUTBOX ? 62.0f : 46.0f);
        switch (hooked->element) {
            case MOD_MENU_ELEMENT_BUTTON:
                if (mxui_button(row, hooked->name, false)) {
                    smlua_call_mod_menu_element_hook(hooked, i);
                }
                break;
            case MOD_MENU_ELEMENT_CHECKBOX: {
                bool value = hooked->boolValue;
                if (mxui_toggle(row, hooked->name, &value)) {
                    hooked->boolValue = value;
                    smlua_call_mod_menu_element_hook(hooked, i);
                }
                break;
            }
            case MOD_MENU_ELEMENT_SLIDER:
                if (mxui_slider_u32(row, hooked->name, &hooked->uintValue, hooked->sliderMin, hooked->sliderMax, 1)) {
                    smlua_call_mod_menu_element_hook(hooked, i);
                }
                break;
            case MOD_MENU_ELEMENT_BIND:
                mxui_bind_row(row, hooked->name, hooked->bindValue, hooked->defaultBindValue, mod, hooked->configKey, i);
                break;
            case MOD_MENU_ELEMENT_INPUTBOX: {
                mxui_skin_draw_panel(row, theme.panelAlt, theme.border, mxui_color(0, 0, 0, 0), 10.0f, 1.0f);
                mxui_draw_text_box_fitted(hooked->name, (struct MxuiRect){ row.x + 14.0f, row.y + 8.0f, row.w - 28.0f, 22.0f }, 0.60f, 0.42f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 1);
                mxui_draw_text_box_fitted(hooked->stringValue, (struct MxuiRect){ row.x + 14.0f, row.y + 32.0f, row.w - 28.0f, 22.0f }, 0.52f, 0.38f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 1);
                break;
            }
            default:
                break;
        }
    }
}

static void mxui_render_mods(struct MxuiContext* ctx) {
    struct MxuiScreenState* screen = mxui_current();
    struct MxuiModList list;
    mxui_build_mod_list(&list);

    if (list.count == 0) {
        sMxui.modCategory = 0;
        mxui_build_mod_list(&list);
    }

    bool selectedInList = false;
    for (s32 i = 0; i < list.count; i++) {
        if (list.indices[i] == screen->tag) {
            selectedInList = true;
            break;
        }
    }
    if (!selectedInList) {
        screen->tag = (list.count > 0) ? list.indices[0] : -1;
    }

    mxui_content_reset(ctx);
    const f32 gap = 12.0f;
    const f32 cardGapX = 16.0f;
    const f32 cardGapY = 16.0f;
    const f32 topX = ctx->content.x;
    const f32 topW = ctx->content.w;
    const f32 colW = (topW - gap) * 0.5f;
    f32 y = ctx->content.y;

    struct MxuiRect categoryRect = { topX, y, colW, 38.0f };
    struct MxuiRect profileRect = { topX + colW + gap, y, colW, 38.0f };
    bool categoryChanged = mxui_select_u32(categoryRect, "Category", sModCategoryLabels, (s32)(sizeof(sModCategoryLabels) / sizeof(sModCategoryLabels[0])), &sMxui.modCategory);
    bool profileChanged = mxui_select_u32(profileRect, "Profile", sModProfileIds, MXUI_PROFILE_COUNT, &sMxui.modProfile);
    if (categoryChanged || profileChanged) {
        screen->scroll = 0.0f;
        mxui_build_mod_list(&list);
        selectedInList = false;
    }

    y += 48.0f;
    struct MxuiRect saveProfileRect = { topX, y, colW, 34.0f };
    struct MxuiRect loadProfileRect = { topX + colW + gap, y, colW, 34.0f };
    if (mxui_button(saveProfileRect, "Save Profile", false)) {
        if (mods_profile_save(sModProfileIds[sMxui.modProfile])) {
            mxui_toast("Profile saved.", 60);
        }
    }
    if (mxui_button(loadProfileRect, "Load Profile", false)) {
        if (mods_profile_load(sModProfileIds[sMxui.modProfile])) {
            mxui_sync_local_mod_runtime();
            mxui_toast("Profile loaded.", 60);
        }
    }

    y += 44.0f;
    struct MxuiRect disableAllRect = { topX, y, colW, 34.0f };
    struct MxuiRect restoreRect = { topX + colW + gap, y, colW, 34.0f };
    if (mxui_button(disableAllRect, "Disable All", true)) {
        mods_disable_all();
        mxui_sync_local_mod_runtime();
        mod_cache_save();
        mxui_toast("All selectable mods disabled.", 60);
    }
    if (mxui_button(restoreRect, "Restore Last Session", false)) {
        if (mods_profile_load(MOD_PROFILE_LAST_SESSION)) {
            mxui_sync_local_mod_runtime();
            mod_cache_save();
            mxui_toast("Restored last session.", 60);
        }
    }

    y += 50.0f;
    struct MxuiRect bodyArea = {
        ctx->content.x,
        y,
        ctx->content.w,
        MAX(120.0f, ctx->content.h - (y - ctx->content.y))
    };
    struct MxuiRect gridPane = {
        bodyArea.x,
        bodyArea.y,
        MAX(320.0f, bodyArea.w * 0.58f),
        bodyArea.h
    };
    struct MxuiRect detailPane = {
        gridPane.x + gridPane.w + gap,
        bodyArea.y,
        bodyArea.w - gridPane.w - gap,
        bodyArea.h
    };
    struct MxuiRect gridViewport = mxui_layout_inset(gridPane, 10.0f, 10.0f);

    if (list.count <= 0) {
        struct MxuiRect empty = {
            gridPane.x,
            gridPane.y,
            gridPane.w,
            MIN(140.0f, gridPane.h)
        };
        mxui_skin_draw_panel(empty, mxui_theme().panel, mxui_theme().border, mxui_color(mxui_theme().glow.r, mxui_theme().glow.g, mxui_theme().glow.b, 20), 14.0f, 1.0f);
        mxui_draw_text_box_fitted("No mods match the current category.", (struct MxuiRect){ empty.x + 18.0f, empty.y + 18.0f, empty.w - 36.0f, empty.h - 36.0f }, 0.52f, 0.34f, FONT_NORMAL, mxui_theme().textDim, MXUI_TEXT_CENTER, true, 3);
        mxui_render_mod_detail_panel(detailPane, NULL, -1);
        ctx->contentHeight = MAX(ctx->contentHeight, ctx->content.h);
        return;
    }

    {
        mxui_skin_draw_panel(gridPane, mxui_theme().panel, mxui_theme().border, mxui_color(mxui_theme().glow.r, mxui_theme().glow.g, mxui_theme().glow.b, 18), 14.0f, 1.0f);

        s32 totalRows = (list.count + 1) / 2;
        f32 cardWidth = (gridViewport.w - cardGapX) * 0.5f;
        f32 cardHeight = mxui_clampf((gridViewport.h - cardGapY * 2.0f) / 3.0f, 152.0f, 194.0f);
        f32 totalGridHeight = totalRows * cardHeight + MAX(0, totalRows - 1) * cardGapY;
        f32 maxScroll = MAX(0.0f, totalGridHeight - gridViewport.h);
        screen->scroll = mxui_clampf(screen->scroll, 0.0f, maxScroll);
        ctx->contentHeight = MAX(ctx->contentHeight, ctx->content.h + maxScroll);

        for (s32 rowIndex = 0; rowIndex < totalRows; rowIndex++) {
            f32 cardY = gridViewport.y + rowIndex * (cardHeight + cardGapY) - screen->scroll;
            if (cardY + cardHeight < gridViewport.y - 8.0f || cardY > gridViewport.y + gridViewport.h + 8.0f) {
                continue;
            }

            s32 leftIndex = rowIndex * 2;
            struct MxuiRect leftCard = { gridViewport.x, cardY, cardWidth, cardHeight };
            mxui_render_mod_card(leftCard, mxui_local_mod(list.indices[leftIndex]), list.indices[leftIndex], screen);

            if (leftIndex + 1 < list.count) {
                struct MxuiRect rightCard = { gridViewport.x + cardWidth + cardGapX, cardY, cardWidth, cardHeight };
                mxui_render_mod_card(rightCard, mxui_local_mod(list.indices[leftIndex + 1]), list.indices[leftIndex + 1], screen);
            }
        }
    }

    mxui_render_mod_detail_panel(detailPane, mxui_current_mod_from_tag(screen->tag), screen->tag);
}

static void mxui_render_dynos(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    for (s32 i = 0; i < dynos_pack_get_count(); i++) {
        if (!dynos_pack_get_exists(i)) { continue; }
        bool enabled = dynos_pack_get_enabled(i);
        if (mxui_toggle(mxui_stack_next_row(ctx, 46.0f), dynos_pack_get_name(i), &enabled)) {
            dynos_pack_set_enabled(i, enabled);
        }
    }

    if (mxui_toggle(mxui_stack_next_row(ctx, 46.0f), "Local Player Model Only", &configDynosLocalPlayerModelOnly)) {
        mxui_refresh_local_player_models();
    }
}

static void mxui_render_player(struct MxuiContext* ctx) {
    struct MxuiTheme theme = mxui_theme();
    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Character", "Launch Character Select, open the color browser, or manage character-related packs.", 5);
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Open Character Select", false)) {
        if (!mxui_open_character_select_menu()) {
            mxui_toast(mxui_is_main_menu_active() ? "Start an adventure first." : "Character Select is unavailable right now.", 75);
        }
    }
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Open Character Colors", false)) {
        if (!mxui_open_character_select_menu_tab(3)) {
            mxui_toast(mxui_is_main_menu_active() ? "Start an adventure first." : "Character colors are unavailable right now.", 75);
        }
    }
    if (mxui_button(mxui_section_next_row(&section1, 38.0f), "Reset Mario Colors", true)) {
        if (mxui_reset_character_colors()) {
            mxui_toast("Mario colors reset.", 60);
        } else {
            mxui_toast("Mario colors could not be reset right now.", 75);
        }
    }
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Open Mods", false)) {
        mxui_push_screen(MXUI_SCREEN_MODS);
    }
    if (mxui_button(mxui_section_next_row(&section1, 46.0f), "Open DynOS Packs", false)) {
        mxui_push_screen(MXUI_SCREEN_DYNOS);
    }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Profile", "Current player identity for offline play.", 2);
    char profile[192] = { 0 };
    snprintf(profile, sizeof(profile), "Player Name: %s\nSave Slot: File %d", configPlayerName[0] != '\0' ? configPlayerName : "Player", configHostSaveSlot > 0 ? configHostSaveSlot : 1);
    mxui_draw_text_box_fitted(profile, mxui_section_next_row(&section2, 56.0f), 0.56f, 0.40f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 3);
    mxui_toggle(mxui_section_next_row(&section2, 46.0f), "DynOS Local Player Model Only", &configDynosLocalPlayerModelOnly);
}

static void mxui_render_language(struct MxuiContext* ctx) {
    struct MxuiTheme theme = mxui_theme();
    if (sMxui.languageChoiceCount == 0) {
        mxui_load_languages();
    }

    mxui_content_reset(ctx);
    bool selected = false;
    for (s32 i = 0; i < sMxui.languageChoiceCount; i++) {
        const char* key = sMxui.languageChoices[i];
        const char* displayName = mxui_language_get("LANGUAGE", key);
        char label[96] = { 0 };
        snprintf(label, sizeof(label), "%s%s", strcmp(configLanguage, key) == 0 ? "[Current] " : "", displayName != NULL ? displayName : key);
        if (mxui_button(mxui_stack_next_row(ctx, 46.0f), label, false)) {
            mxui_set_language(key);
            selected = true;
        }
    }

    if (selected && sMxui.depth > 1 && sMxui.stack[sMxui.depth - 2].id == MXUI_SCREEN_BOOT) {
        gPanelLanguageOnStartup = false;
        mxui_pop_screen();
        if (startup_experience_should_show_first_run()) {
            mxui_push_screen(MXUI_SCREEN_FIRST_RUN);
        }
        mxui_toast("Language applied.", 60);
    } else if (selected) {
        mxui_toast("Language applied.", 60);
    }

    if (sMxui.languageChoiceCount == 0) {
        mxui_draw_text_box_fitted("No language files were found.", (struct MxuiRect){ ctx->content.x, ctx->content.y + 20, ctx->content.w, 36 }, 0.52f, 0.36f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 2);
    }
}

static void mxui_render_info(struct MxuiContext* ctx) {
    struct MxuiTheme theme = mxui_theme();
    char info[640] = { 0 };
    snprintf(info, sizeof(info),
        "SM64 DX is an offline-first Super Mario 64 PC fork focused on singleplayer, local Lua mods, custom characters, and a cleaner front end.\n\n"
        "Your saves and configuration stay on this machine.\n"
        "The game keeps a backup save and can boot in safe mode if the previous session did not close cleanly.\n\n"
        "Version: %s",
        get_version());
    mxui_content_reset(ctx);
    mxui_draw_text_box_fitted(info, (struct MxuiRect){ ctx->content.x, ctx->content.y + 10, ctx->content.w, ctx->content.h - 20 }, 0.58f, 0.42f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 10);
    ctx->contentHeight = mxui_measure_text_box_height(info, ctx->content.w, 0.58f, FONT_NORMAL, true, 10) + 20.0f;
}

static void mxui_render_first_run(struct MxuiContext* ctx) {
    const char* presetChoices[] = { "Classic", "Comfort", "Focused", "Custom" };
    char saveChoices[NUM_SAVE_FILES][64];
    const char* saveChoicePtrs[NUM_SAVE_FILES];
    unsigned int saveChoice = (configHostSaveSlot > 0 ? configHostSaveSlot : 1) - 1;

    for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
        snprintf(saveChoices[i], sizeof(saveChoices[i]), "Slot %d | %d Stars | %s", i + 1, save_file_get_total_star_count(i, 0, 24), configSaveNames[i][0] != '\0' ? configSaveNames[i] : "SM64 DX");
        saveChoicePtrs[i] = saveChoices[i];
    }

    mxui_content_reset(ctx);
    struct MxuiSectionLayout section1 = mxui_section_begin(ctx, "Starter Setup", "Pick a default save slot and comfort preset.", 2);
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Default Save Slot", saveChoicePtrs, NUM_SAVE_FILES, &saveChoice)) {
        configHostSaveSlot = saveChoice + 1;
    }
    if (mxui_select_u32(mxui_section_next_row(&section1, 46.0f), "Starter Preset", presetChoices, 4, &configAccessibilityPreset)) {
        mxui_apply_accessibility_preset();
    }

    struct MxuiSectionLayout section2 = mxui_section_begin(ctx, "Quick Access", "Open the main setup pages before starting.", 2);
    struct MxuiRowPair row = mxui_section_next_split_row(&section2, 38.0f, 12.0f);
    if (mxui_button(row.left, "Accessibility", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_ACCESSIBILITY); }
    if (mxui_button(row.right, "Controls", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_CONTROLS); }
    row = mxui_section_next_split_row(&section2, 38.0f, 12.0f);
    if (mxui_button(row.left, "Display", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_DISPLAY); }
    if (mxui_button(row.right, "Sound", false)) { mxui_push_screen(MXUI_SCREEN_SETTINGS_SOUND); }

    struct MxuiSectionLayout section3 = mxui_section_begin(ctx, "Safety", "Local saves, backup recovery, and safe mode.", 2);
    mxui_draw_text_box_fitted("SM64 DX keeps a backup save and can boot in safe mode if the last session crashes.", mxui_section_next_row(&section3, 56.0f), 0.56f, 0.40f, FONT_NORMAL, mxui_theme().textDim, MXUI_TEXT_LEFT, true, 3);
    if (mxui_button(mxui_section_next_row(&section3, 38.0f), "Finish Setup", false)) {
        configFirstBootCompleted = true;
        configfile_save(configfile_name());
        mxui_pop_screen();
    }
}

static void mxui_render_current_screen(struct MxuiContext* ctx) {
    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL) { return; }

    switch (screen->id) {
        case MXUI_SCREEN_BOOT: mxui_render_boot(ctx); break;
        case MXUI_SCREEN_SAVE_SELECT: mxui_render_save_select(ctx); break;
        case MXUI_SCREEN_HOME: mxui_render_home(ctx); break;
        case MXUI_SCREEN_PAUSE: mxui_render_pause(ctx); break;
        case MXUI_SCREEN_SETTINGS_HUB: mxui_render_settings_hub(ctx); break;
        case MXUI_SCREEN_SETTINGS_DISPLAY: mxui_render_settings_display(ctx); break;
        case MXUI_SCREEN_SETTINGS_SOUND: mxui_render_settings_sound(ctx); break;
        case MXUI_SCREEN_SETTINGS_CONTROLS: mxui_render_settings_controls(ctx); break;
        case MXUI_SCREEN_SETTINGS_CONTROLS_N64: mxui_render_settings_controls_n64(ctx); break;
        case MXUI_SCREEN_SETTINGS_HOTKEYS: mxui_render_settings_hotkeys(ctx); break;
        case MXUI_SCREEN_SETTINGS_CAMERA: mxui_render_settings_camera(ctx); break;
        case MXUI_SCREEN_SETTINGS_FREE_CAMERA: mxui_render_settings_free_camera(ctx); break;
        case MXUI_SCREEN_SETTINGS_ROMHACK_CAMERA: mxui_render_settings_romhack_camera(ctx); break;
        case MXUI_SCREEN_SETTINGS_ACCESSIBILITY: mxui_render_settings_accessibility(ctx); break;
        case MXUI_SCREEN_SETTINGS_MISC: mxui_render_settings_misc(ctx); break;
        case MXUI_SCREEN_SETTINGS_MENU_OPTIONS: mxui_render_settings_menu_options(ctx); break;
        case MXUI_SCREEN_MODS: mxui_render_mods(ctx); break;
        case MXUI_SCREEN_MOD_DETAILS: mxui_render_mod_details(ctx, screen->tag); break;
        case MXUI_SCREEN_DYNOS: mxui_render_dynos(ctx); break;
        case MXUI_SCREEN_PLAYER: mxui_render_player(ctx); break;
        case MXUI_SCREEN_LANGUAGE: mxui_render_language(ctx); break;
        case MXUI_SCREEN_INFO: mxui_render_info(ctx); break;
        case MXUI_SCREEN_FIRST_RUN: mxui_render_first_run(ctx); break;
        case MXUI_SCREEN_MANAGE_SAVES: mxui_render_manage_saves(ctx); break;
        case MXUI_SCREEN_MANAGE_SLOT: mxui_render_manage_slot(ctx, screen->tag); break;
        default: break;
    }
}

static void mxui_handle_back_action(void) {
    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL || sMxui.confirmOpen || sMxui.capturedBind != NULL) { return; }

    if (sMxui.input.menuToggle && sMxui.pauseMenu) {
        mxui_close_pause_menu_with_mode(1);
        return;
    }

    if (!sMxui.input.back) { return; }

    switch (screen->id) {
        case MXUI_SCREEN_BOOT:
            break;
            break;
        case MXUI_SCREEN_PAUSE:
            mxui_close_pause_menu_with_mode(1);
            break;
        default:
            mxui_pop_screen();
            break;
    }
}

static void mxui_render_post_screen_footer(const struct MxuiScreenConfig* config, struct MxuiScreenState* screen, struct MxuiContext* ctx) {
    if (config == NULL || screen == NULL || ctx == NULL) {
        return;
    }

    mxui_render_reset_scissor();
    mxui_render_reset_texture_clipping();
    sMxui.renderingFooter = true;

    bool backClicked = false;
    if (config->showBackFooter) {
        const char* backLabel = (config->backLabel != NULL) ? config->backLabel : "Back";
        mxui_footer_button(ctx->footer, false, backLabel, &backClicked);
    }

    switch (screen->id) {
        case MXUI_SCREEN_SAVE_SELECT: {
            bool manageClicked = false;
            mxui_footer_button(ctx->footer, true, "Manage Saves", &manageClicked);
            if (backClicked) {
                mxui_pop_screen();
            } else if (manageClicked) {
                mxui_push_screen(MXUI_SCREEN_MANAGE_SAVES);
            }
            break;
        }
        case MXUI_SCREEN_MODS: {
            char footerText[64] = { 0 };
            struct MxuiModList list;
            mxui_build_mod_list(&list);
            snprintf(footerText, sizeof(footerText), "%d %s", list.count, list.count == 1 ? "mod" : "mods");
            mxui_footer_center_text(ctx->footer, footerText);
            break;
        }
        case MXUI_SCREEN_DYNOS: {
            bool refreshClicked = false;
            mxui_footer_button(ctx->footer, true, "Refresh", &refreshClicked);
            if (refreshClicked) {
                dynos_gfx_init();
                mxui_toast("DynOS packs refreshed.", 60);
            }
            break;
        }
        default:
            break;
    }

    if (backClicked) {
        mxui_pop_screen();
    }
    sMxui.renderingFooter = false;
}

void mxui_render(void) {
    if (!mxui_is_active()) { return; }

    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL) { return; }
    const struct MxuiScreenConfig* config = mxui_screen_config(screen->id);

    mxui_apply_input();
    sMxui.shellAnim += (sMxui.shellTarget - sMxui.shellAnim) * 0.28f;
    if (fabsf(sMxui.shellTarget - sMxui.shellAnim) < 0.01f) {
        sMxui.shellAnim = sMxui.shellTarget;
    }
    sMxui.screenAnim += (sMxui.screenTarget - sMxui.screenAnim) * 0.32f;
    if (fabsf(sMxui.screenTarget - sMxui.screenAnim) < 0.01f) {
        sMxui.screenAnim = sMxui.screenTarget;
    }
    sMxui.confirmAnim += (sMxui.confirmTarget - sMxui.confirmAnim) * 0.34f;
    if (fabsf(sMxui.confirmTarget - sMxui.confirmAnim) < 0.01f) {
        sMxui.confirmAnim = sMxui.confirmTarget;
    }
    if (sMxui.pauseClosing) {
        sMxui.input.mousePressed = false;
        sMxui.input.mouseReleased = false;
        sMxui.input.accept = false;
        sMxui.input.back = false;
        sMxui.input.menuToggle = false;
        sMxui.input.up = false;
        sMxui.input.down = false;
        sMxui.input.left = false;
        sMxui.input.right = false;
    }
    mxui_handle_navigation();
    mxui_capture_bind_if_needed();

    sMxui.nextFocusIndex = 0;
    sMxui.focusedRectValid = false;
    sMxui.renderingModal = false;
    sMxui.renderingFooter = false;
    screen->focusCount = 0;
    sMxui.rendering = true;

    struct MxuiContext ctx = mxui_begin_screen_template(config);
    mxui_render_current_screen(&ctx);
    if (ctx.contentHeight <= 0.0f) {
        ctx.contentHeight = MAX(0.0f, ctx.cursorY - ctx.content.y);
    }

    screen->focusCount = sMxui.nextFocusIndex;
    mxui_clamp_focus(screen);
    mxui_sync_focus_feedback();
    mxui_scroll_into_view(screen);
    mxui_scroll_apply(&ctx);
    mxui_end_screen(&ctx);

    if (sMxui.deferredAction == MXUI_DEFERRED_NONE) {
        mxui_render_post_screen_footer(config, screen, &ctx);
    }

    if (sMxui.deferredAction == MXUI_DEFERRED_NONE) {
        mxui_handle_back_action();
    }
    if (sMxui.deferredAction == MXUI_DEFERRED_NONE) {
        mxui_render_confirm();
    }

    if (sMxui.toastTimer > 0) {
        sMxui.toastTimer--;
        struct MxuiRect toast = {
            (mxui_render_screen_width() - 420.0f) * 0.5f,
            mxui_render_screen_height() - 96.0f,
            420.0f,
            42.0f,
        };
        mxui_skin_draw_panel(toast, mxui_color(0, 0, 0, 176), mxui_theme().border, mxui_color(mxui_theme().glow.r, mxui_theme().glow.g, mxui_theme().glow.b, 18), 10.0f, 1.0f);
        mxui_draw_text(sMxui.toast, toast.x + toast.w * 0.5f, toast.y + 6, 0.52f, FONT_NORMAL, mxui_theme().text, true);
    }

    sMxui.rendering = false;

    if (sMxui.pauseClosing && sMxui.shellAnim <= 0.04f) {
        sMxui.pauseClosing = false;
        mxui_close_pause_menu_with_mode_immediate(sMxui.deferredPauseMode);
        return;
    }

    mxui_apply_deferred_action();

    if (sMxui.wantsConfigSave) {
        configfile_save(configfile_name());
        sMxui.wantsConfigSave = false;
    }
    if (sMxui.input.mouseReleased) {
        sMxui.mouseCaptureValid = false;
    }
}
