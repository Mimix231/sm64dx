#include <stdio.h>

#include "sm64.h"
#include "mxui_internal.h"

#include "pc/configfile.h"
#include "pc/network/network.h"
#include "pc/lua/smlua_hooks.h"
#include "pc/lua/utils/smlua_text_utils.h"
#include "pc/pc_main.h"
#include "pc/startup_experience.h"

#include "audio/external.h"
#include "engine/math_util.h"
#include "game/area.h"
#include "game/hud.h"
#include "game/hardcoded.h"
#include "game/level_update.h"
#include "game/save_file.h"
#include "sounds.h"

#include "mxui_popup.h"

extern s16 gChangeLevelTransition;
extern bool gDjuiInMainMenu;
void update_all_mario_stars(void);

void mxui_quit_game(void) {
    game_exit();
}

static void mxui_start_singleplayer_session(bool playSound) {
    stop_demo(NULL);
    mxui_clear();
    network_reset_reconnect_and_rehost();
    network_set_system(NS_OFFLINE);
    gCurrSaveFileNum = configHostSaveSlot;
    update_all_mario_stars();

    network_init(NT_SERVER, false);
    fake_lvl_init_from_save_file();
    gChangeLevelTransition = gLevelValues.entryLevel;

    if (gMarioState != NULL && gMarioState->marioObj != NULL) {
        vec3f_copy(gMarioState->marioObj->header.gfx.cameraToObject, gGlobalSoundSource);
    }
    if (playSound) {
        gDelayedInitSound = CHAR_SOUND_OKEY_DOKEY;
    }

    play_transition(WARP_TRANSITION_FADE_INTO_STAR, 0x14, 0x00, 0x00, 0x00);
}

static void mxui_return_to_title(void) {
    gPauseMenuHidden = false;
    network_reset_reconnect_and_rehost();
    network_shutdown(true, false, false, false);
}

static void mxui_open_manage_slot(s32 slot) {
    mxui_push_screen_with_tag(MXUI_SCREEN_MANAGE_SLOT, slot);
}

static void mxui_select_save_slot(s32 slot, bool goHome) {
    configHostSaveSlot = slot + 1;
    if (goHome) {
        mxui_push_screen(MXUI_SCREEN_HOME);
    }
}

static bool mxui_pause_can_exit_level(void) {
    return COURSE_IS_VALID_COURSE(gCurrCourseNum)
        && (gLevelValues.pauseExitAnywhere || (gMarioStates[0].action & ACT_FLAG_PAUSE_EXIT));
}

static void mxui_pause_exit_level(bool exitToCastle) {
    bool allowExit = true;
    smlua_call_event_hooks(HOOK_ON_PAUSE_EXIT, exitToCastle, &allowExit);
    if (!allowExit) {
        mxui_popup_create("\\#ffa0a0\\This level blocked exiting from pause.", 2);
        return;
    }
    mxui_close_pause_menu_with_mode(exitToCastle ? 3 : 2);
}

void mxui_render_boot(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    bool startClicked = mxui_button(mxui_stack_next_row(ctx, 54.0f), "[Start]", false);
    struct MxuiRowPair row = mxui_stack_next_split_row(ctx, 50.0f, 20.0f);
    bool settingsClicked = mxui_button(row.left, "Settings", false);
    bool quitClicked = mxui_button(row.right, "Quit", true);

    if (startup_experience_is_safe_mode()) {
        struct MxuiRect status = mxui_stack_next_row(ctx, 104.0f);
        mxui_skin_draw_panel(status, mxui_theme().panel, mxui_theme().border, mxui_color(mxui_theme().glow.r, mxui_theme().glow.g, mxui_theme().glow.b, 24), 12.0f, 1.0f);
        mxui_draw_text_box_fitted(startup_experience_get_status_text(), (struct MxuiRect){ status.x + 16.0f, status.y + 18.0f, status.w - 32.0f, status.h - 36.0f }, 0.58f, 0.42f, FONT_NORMAL, mxui_theme().textDim, MXUI_TEXT_LEFT, true, 4);
    }

    if (startClicked) {
        mxui_push_screen(MXUI_SCREEN_SAVE_SELECT);
    } else if (settingsClicked) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_HUB);
    } else if (quitClicked) {
        mxui_open_confirm("Quit", "Close SM64 DX?", mxui_quit_game);
    }
}

void mxui_render_save_select(struct MxuiContext* ctx) {
    const f32 gap = 20.0f;
    const f32 cardW = (ctx->content.w - gap) * 0.5f;
    const f32 cardH = MIN(186.0f, (ctx->content.h - gap - 56.0f) * 0.5f);

    for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
        s32 row = i / 2;
        s32 col = i % 2;
        struct MxuiRect card = {
            ctx->content.x + col * (cardW + gap),
            ctx->content.y + row * (cardH + 18),
            cardW,
            cardH,
        };
        struct MxuiTheme theme = mxui_theme();
        bool active = i == (s32)((configHostSaveSlot > 0 ? configHostSaveSlot : 1) - 1);
        mxui_skin_draw_panel(card, active ? theme.panelAlt : theme.panel, theme.border, active ? theme.glow : mxui_color(0, 0, 0, 0), 14.0f, 1.0f);

        char title[128] = { 0 };
        char body[160] = { 0 };
        char action[32] = { 0 };
        if (save_file_exists(i)) {
            snprintf(title, sizeof(title), "File %d  |  %d Stars%s", i + 1, save_file_get_total_star_count(i, 0, 24), active ? "  |  Active" : "");
            snprintf(body, sizeof(body), "%s\n%d stars collected\n%s", configSaveNames[i][0] != '\0' ? configSaveNames[i] : "SM64 DX", save_file_get_total_star_count(i, 0, 24), active ? "Current adventure file" : "Ready to continue");
            snprintf(action, sizeof(action), "%s", active ? "Continue" : "Adventure");
        } else {
            snprintf(title, sizeof(title), "File %d  |  New Save%s", i + 1, active ? "  |  Active" : "");
            snprintf(body, sizeof(body), "%s\nFresh save slot\n%s", configSaveNames[i][0] != '\0' ? configSaveNames[i] : "SM64 DX", active ? "Currently selected for play" : "Pick Adventure to start here");
            snprintf(action, sizeof(action), "%s", active ? "Start Here" : "New Game");
        }
        mxui_draw_text_box_fitted(title, (struct MxuiRect){ card.x + 14.0f, card.y + 14.0f, card.w - 28.0f, 30.0f }, 0.56f, 0.36f, FONT_MENU, theme.title, MXUI_TEXT_LEFT, true, 2);
        mxui_draw_text_box_fitted(body, (struct MxuiRect){ card.x + 14.0f, card.y + 48.0f, card.w - 28.0f, card.h - 102.0f }, 0.56f, 0.40f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 4);

        if (mxui_button((struct MxuiRect){ card.x + 14, card.y + card.h - 48, card.w * 0.58f, 34 }, action, false)) {
            mxui_select_save_slot(i, true);
        }
        if (mxui_button((struct MxuiRect){ card.x + card.w - 154, card.y + card.h - 48, 140, 34 }, "Manage", false)) {
            mxui_open_manage_slot(i);
        }
    }

}

void mxui_render_manage_saves(struct MxuiContext* ctx) {
    mxui_content_reset(ctx);
    for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
        char label[128] = { 0 };
        if (save_file_exists(i)) {
            snprintf(label, sizeof(label), "File %d  |  %d Stars%s", i + 1, save_file_get_total_star_count(i, 0, 24), i == (s32)((configHostSaveSlot > 0 ? configHostSaveSlot : 1) - 1) ? "  |  Active" : "");
        } else {
            snprintf(label, sizeof(label), "File %d  |  New Save%s", i + 1, i == (s32)((configHostSaveSlot > 0 ? configHostSaveSlot : 1) - 1) ? "  |  Active" : "");
        }
        if (mxui_button(mxui_stack_next_row(ctx, 48.0f), label, false)) {
            mxui_open_manage_slot(i);
        }
    }
}

void mxui_render_manage_slot(struct MxuiContext* ctx, s32 slot) {
    struct MxuiTheme theme = mxui_theme();
    mxui_content_reset(ctx);
    char title[160] = { 0 };
    if (save_file_exists(slot)) {
        snprintf(title, sizeof(title), "File %d  |  %d Stars\n%s", slot + 1, save_file_get_total_star_count(slot, 0, 24), configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX");
    } else {
        snprintf(title, sizeof(title), "File %d  |  New Save\n%s", slot + 1, configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX");
    }
    struct MxuiRect card = mxui_stack_next_row(ctx, 112.0f);
    mxui_skin_draw_panel(card, theme.panel, theme.border, mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 22), 14.0f, 1.0f);
    mxui_draw_text_box_fitted(title, (struct MxuiRect){ card.x + 16.0f, card.y + 20.0f, card.w - 32.0f, card.h - 40.0f }, 0.56f, 0.36f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, 3);

    if (mxui_button(mxui_stack_next_row(ctx, 44.0f), "Use This Save", false)) {
        configHostSaveSlot = slot + 1;
        mxui_toast("Selected save updated.", 90);
    }
    struct MxuiRowPair row = mxui_stack_next_split_row(ctx, 44.0f, 14.0f);
    if (mxui_button(row.left, "Copy To Next Slot", false)) {
        s32 dst = (slot + 1) % NUM_SAVE_FILES;
        save_file_copy(slot, dst);
        save_file_reload(1);
        snprintf(configSaveNames[dst], sizeof(configSaveNames[dst]), "%s", configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX");
        mxui_toast("Save copied.", 90);
    }
    if (mxui_button(row.right, "Erase Save", true)) {
        save_file_erase(slot);
        if (configHostSaveSlot == (u32)(slot + 1)) {
            configHostSaveSlot = 1;
        }
        mxui_toast("Save erased.", 90);
    }
}

void mxui_render_home(struct MxuiContext* ctx) {
    struct MxuiTheme theme = mxui_theme();
    mxui_content_reset(ctx);
    s32 slot = (configHostSaveSlot > 0 ? configHostSaveSlot : 1) - 1;
    char badge[128] = { 0 };
    if (save_file_exists(slot)) {
        snprintf(badge, sizeof(badge), "Selected Save: File %d  |  %s  |  %d %s", slot + 1, configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX", save_file_get_total_star_count(slot, 0, 24), save_file_get_total_star_count(slot, 0, 24) == 1 ? "Star" : "Stars");
    } else {
        snprintf(badge, sizeof(badge), "Selected Save: File %d  |  New Save  |  Ready to Start", slot + 1);
    }
    struct MxuiRect badgeRect = mxui_stack_next_row(ctx, 46.0f);
    mxui_skin_draw_panel(badgeRect, theme.panelAlt, theme.border, mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 28), 12.0f, 1.0f);
    mxui_draw_text_box_fitted(badge, badgeRect, 0.56f, 0.34f, FONT_NORMAL, theme.text, MXUI_TEXT_CENTER, true, 2);

    if (mxui_button(mxui_stack_next_row(ctx, 46.0f), "Play", false)) {
        gDjuiInMainMenu = false;
        mxui_start_singleplayer_session(true);
        return;
    }
    struct MxuiRowPair row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Settings", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_HUB);
    }
    if (mxui_button(row.right, "Mods", false)) {
        mxui_push_screen(MXUI_SCREEN_MODS);
    }
    row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Player", false)) {
        mxui_push_screen(MXUI_SCREEN_PLAYER);
    }
    if (mxui_button(row.right, "DynOS Packs", false)) {
        mxui_push_screen(MXUI_SCREEN_DYNOS);
    }
    row = mxui_stack_next_split_row(ctx, 46.0f, 16.0f);
    if (mxui_button(row.left, "Change Save", false)) {
        mxui_pop_screen();
    }
    if (mxui_button(row.right, "Manage Saves", false)) {
        mxui_push_screen(MXUI_SCREEN_MANAGE_SAVES);
    }
    if (mxui_button(mxui_stack_next_row(ctx, 42.0f), "Quit", true)) {
        mxui_open_confirm("Quit", "Close SM64 DX?", mxui_quit_game);
    }
}

void mxui_render_pause(struct MxuiContext* ctx) {
    struct MxuiTheme theme = mxui_theme();
    mxui_content_reset(ctx);
    s32 slot = gCurrSaveFileNum > 0 ? (gCurrSaveFileNum - 1) : 0;
    bool inCourse = COURSE_IS_VALID_COURSE(gCurrCourseNum);
    struct MxuiRect snap = mxui_stack_next_row(ctx, 126.0f);
    mxui_skin_draw_panel(snap, theme.panel, theme.border, mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 28), 14.0f, 1.0f);
    mxui_draw_text_box_fitted(inCourse ? "Snapshot" : "Castle Snapshot", (struct MxuiRect){ snap.x + 16.0f, snap.y + 12.0f, snap.w - 32.0f, 30.0f }, 0.58f, 0.40f, FONT_MENU, theme.title, MXUI_TEXT_LEFT, true, 1);

    char details[256] = { 0 };
    if (inCourse) {
        const char* courseName = smlua_text_utils_course_name_get(gCurrCourseNum);
        snprintf(details, sizeof(details), "%s\nAct %d  |  Coins %d  |  Best %d\nSave: %s  |  Total Stars: %d",
            courseName != NULL ? courseName : "Current Course",
            (gCurrActNum >= 1 && gCurrActNum <= 6) ? gCurrActNum : 1,
            gHudDisplay.coins,
            save_file_get_course_coin_score(slot, gCurrCourseNum - 1),
            configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX",
            save_file_get_total_star_count(slot, 0, 24));
    } else {
        snprintf(details, sizeof(details), "Secret Stars: %d\nSave: %s\nFile Total: %d Stars",
            save_file_get_course_star_count(slot, -1),
            configSaveNames[slot][0] != '\0' ? configSaveNames[slot] : "SM64 DX",
            save_file_get_total_star_count(slot, 0, 24));
    }
    mxui_draw_text_box_fitted(details, (struct MxuiRect){ snap.x + 16.0f, snap.y + 44.0f, snap.w - 32.0f, snap.h - 54.0f }, 0.50f, 0.32f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 4);

    if (mxui_button(mxui_stack_next_row(ctx, 44.0f), "Resume", false)) {
        mxui_close_pause_menu_with_mode(1);
        return;
    }
    struct MxuiRowPair row = mxui_stack_next_split_row(ctx, 44.0f, 16.0f);
    if (mxui_button(row.left, "Settings", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_HUB);
    }
    if (mxui_button(row.right, "Controls", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_CONTROLS);
    }
    row = mxui_stack_next_split_row(ctx, 44.0f, 16.0f);
    if (mxui_button(row.left, "Hotkeys", false)) {
        mxui_push_screen(MXUI_SCREEN_SETTINGS_HOTKEYS);
    }
    if (mxui_button(row.right, "Player", false)) {
        mxui_push_screen(MXUI_SCREEN_PLAYER);
    }
    row = mxui_stack_next_split_row(ctx, 44.0f, 16.0f);
    if (mxui_button(row.left, "Mods", false)) {
        mxui_push_screen(MXUI_SCREEN_MODS);
    }
    if (mxui_button(row.right, "DynOS Packs", false)) {
        mxui_push_screen(MXUI_SCREEN_DYNOS);
    }
    if (mxui_pause_can_exit_level()) {
        row = mxui_stack_next_split_row(ctx, 42.0f, 16.0f);
        if (mxui_button(row.left, "Exit Course", false)) {
            mxui_pause_exit_level(false);
            return;
        }
        if (mxui_button(row.right, "Exit to Castle", false)) {
            mxui_pause_exit_level(true);
            return;
        }
    }
    if (mxui_button(mxui_stack_next_row(ctx, 42.0f), "Return to Title", true)) {
        mxui_open_confirm("Return to Title", "Leave gameplay and go back to the title screen?", mxui_return_to_title);
    }
}
