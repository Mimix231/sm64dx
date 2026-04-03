#include <stdio.h>

#include "sm64.h"
#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_course_info.h"
#include "djui_panel_menu.h"
#include "djui_panel_confirm.h"
#include "djui_panel_moonos.h"
#include "djui_panel_options.h"
#include "djui_panel_pause.h"
#include "djui_panel_player.h"
#include "djui_sm64dx.h"

#include "game/area.h"
#include "game/hardcoded.h"
#include "game/level_info.h"
#include "game/level_update.h"
#include "game/mario.h"
#include "game/object_helpers.h"
#include "game/save_file.h"
#include "pc/configfile.h"
#include "pc/lua/utils/smlua_level_utils.h"
#include "pc/rom_checker.h"

bool gDjuiPanelPauseCreated = false;

static int sDjuiPauseResult = 0;

static float djui_panel_pause_measure_text_height(const struct DjuiFont *font, float fontScale, int lineCount, float padding) {
    if (font == NULL) {
        return 0.0f;
    }
    if (lineCount < 1) {
        lineCount = 1;
    }
    return ((font->charHeight + ((float) (lineCount - 1) * font->lineHeight)) * fontScale) + padding;
}

static struct DjuiText *djui_panel_pause_text(struct DjuiBase *parent, const char *message, const struct DjuiFont *font,
                                              float fontScale, int lineCount, u8 r, u8 g, u8 b) {
    struct DjuiText *text = djui_text_create(parent, message);
    float height = djui_panel_pause_measure_text_height(font, fontScale, lineCount, 4.0f);
    djui_base_set_size_type(&text->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&text->base, 1.0f, height);
    djui_base_set_color(&text->base, r, g, b, 255);
    djui_text_set_drop_shadow(text, 48, 48, 48, 100);
    djui_text_set_font(text, (struct DjuiFont *) font);
    djui_text_set_font_scale(text, fontScale);
    return text;
}

static void djui_panel_pause_style_panel(struct DjuiThreePanel *panel) {
    struct DjuiFlowLayout *body = (struct DjuiFlowLayout *) djui_three_panel_get_body(panel);
    djui_base_set_size_type(&panel->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&panel->base, 760.0f, 0.78f);
    djui_base_set_alignment(&panel->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_base_set_color(&panel->base, 16, 20, 32, 236);
    djui_base_set_border_color(&panel->base, 218, 180, 90, 255);
    djui_base_set_border_width(&panel->base, 6);
    djui_base_set_gradient(&panel->base, false);
    djui_flow_layout_set_margin(body, 12);
}

static bool djui_panel_pause_allow_exit_level(void) {
    return gLevelValues.pauseExitAnywhere || (gMarioStates[0].action & ACT_FLAG_PAUSE_EXIT);
}

static bool djui_panel_pause_is_course_context(void) {
    return gCurrCourseNum >= COURSE_MIN && gCurrCourseNum <= COURSE_MAX;
}

int djui_panel_pause_consume_result(void) {
    int result = sDjuiPauseResult;
    sDjuiPauseResult = 0;
    return result;
}

void djui_panel_pause_resume(UNUSED struct DjuiBase* caller) {
    level_set_transition(0, NULL);
    gPauseScreenMode = 1;
    gDialogBoxState = 0;
    gMenuMode = -1;
    gDjuiInPlayerMenu = false;
    sDjuiPauseResult = 1;
    djui_panel_shutdown();
}

void djui_panel_pause_quit_yes(UNUSED struct DjuiBase* caller) {
    level_set_transition(0, NULL);
    gDialogBoxState = 0;
    gMenuMode = -1;
    gDjuiInPlayerMenu = false;
    sDjuiPauseResult = 3;
    djui_panel_shutdown();
}

void djui_panel_pause_disconnect_key_update(UNUSED int scancode) {
}

static void djui_panel_pause_exit_level(struct DjuiBase* caller) {
    djui_panel_confirm_create(caller,
                              "Exit Level",
                              djui_panel_pause_is_course_context()
                                  ? "Return to the castle from the current course."
                                  : "Leave the current scene and return to the castle.",
                              djui_panel_pause_quit_yes);
}

static void djui_panel_pause_player(struct DjuiBase* caller) {
    configHostSaveSlot = gCurrSaveFileNum;
    sm64dx_apply_save_setup(configHostSaveSlot);
    djui_panel_player_create(caller);
}

static void djui_panel_pause_moonos(struct DjuiBase* caller) {
    configHostSaveSlot = gCurrSaveFileNum;
    sm64dx_apply_save_setup(configHostSaveSlot);
    djui_panel_moonos_create(caller);
}

static void djui_panel_pause_build_context(char *titleBuffer, size_t titleSize,
                                           char *objectiveBuffer, size_t objectiveSize,
                                           char *progressBuffer, size_t progressSize,
                                           char *coinsBuffer, size_t coinsSize) {
    int slotIndex = (gCurrSaveFileNum > 0) ? (gCurrSaveFileNum - 1) : 0;
    int courseIndex = (gCurrCourseNum >= COURSE_MIN && gCurrCourseNum <= COURSE_MAX) ? (gCurrCourseNum - 1) : -1;
    int starsCollected = (courseIndex >= 0) ? save_file_get_course_star_count(slotIndex, courseIndex) : 0;
    int bestCoins = (courseIndex >= 0) ? save_file_get_course_coin_score(slotIndex, courseIndex) : 0;
    int redCoinsCollected = 0;

    snprintf(titleBuffer, titleSize, "%s",
             djui_panel_pause_is_course_context()
                 ? get_level_name_ascii(gCurrCourseNum, gCurrLevelNum, gCurrAreaIndex, 1)
                 : "Castle / Hub");

    if (djui_panel_pause_is_course_context() && gCurrActNum >= 1 && gCurrActNum <= 6) {
        snprintf(objectiveBuffer, objectiveSize, "Objective: %s",
                 get_star_name_ascii(gCurrCourseNum, gCurrActNum, 1));
    } else if (djui_panel_pause_is_course_context()) {
        snprintf(objectiveBuffer, objectiveSize, "Objective: Explore and collect more stars.");
    } else {
        snprintf(objectiveBuffer, objectiveSize, "Objective: Review file progress and prepare the next route.");
    }

    if (djui_panel_pause_is_course_context()) {
        snprintf(progressBuffer, progressSize, "Course Progress: %d stars | Best Coins %d", starsCollected, bestCoins);
    } else {
        snprintf(progressBuffer, progressSize, "Castle Progress: Browse unlocked routes, stars, and file status.");
    }

    if (gCurrentArea != NULL && gCurrentArea->numRedCoins > 0) {
        redCoinsCollected = gCurrentArea->numRedCoins - count_objects_with_behavior(bhvRedCoin);
        if (redCoinsCollected < 0) {
            redCoinsCollected = 0;
        }
        snprintf(coinsBuffer, coinsSize, "Red Coins: %d/%d", redCoinsCollected, gCurrentArea->numRedCoins);
    } else if (djui_panel_pause_is_course_context()) {
        snprintf(coinsBuffer, coinsSize, "100-Coin Star: %s", bestCoins >= 100 ? "Collected" : "Available");
    } else {
        snprintf(coinsBuffer, coinsSize, "Scene: Castle hub pause view");
    }
}

void djui_panel_pause_create(struct DjuiBase* caller) {
    struct DjuiBase *defaultBase = NULL;
    struct Sm64dxSaveSummary summary = { 0 };
    struct DjuiThreePanel* panel = NULL;
    struct DjuiBase* body = NULL;
    struct DjuiRect *content = NULL;
    struct DjuiRect *actionsCard = NULL;
    struct DjuiRect *summaryCard = NULL;
    struct DjuiRect *infoCard = NULL;
    struct DjuiFlowLayout *actionsLayout = NULL;
    struct DjuiFlowLayout *summaryLayout = NULL;
    struct DjuiFlowLayout *infoLayout = NULL;
    char titleBuffer[128] = { 0 };
    char objectiveBuffer[160] = { 0 };
    char progressBuffer[160] = { 0 };
    char coinsBuffer[96] = { 0 };
    char saveBuffer[160] = { 0 };
    char moonosBuffer[160] = { 0 };
    char defaultBuffer[160] = { 0 };
    char mapBuffer[160] = { 0 };
    char controlsBuffer[128] = { 0 };
    const float titleScale = gDjuiFonts[2]->defaultFontScale * 0.74f;
    const float detailScale = gDjuiFonts[0]->defaultFontScale * 0.52f;
    const float compactScale = gDjuiFonts[0]->defaultFontScale * 0.48f;

    if (gDjuiPanelPauseCreated) { return; }
    if (gDjuiChatBoxFocus) { djui_chat_box_toggle(); }

    sm64dx_build_save_summary(gCurrSaveFileNum, &summary);
    djui_panel_pause_build_context(titleBuffer, sizeof(titleBuffer),
                                   objectiveBuffer, sizeof(objectiveBuffer),
                                   progressBuffer, sizeof(progressBuffer),
                                   coinsBuffer, sizeof(coinsBuffer));

    snprintf(saveBuffer, sizeof(saveBuffer), "%s | %s | %s",
             summary.title,
             summary.name,
             summary.progressionLine);
    snprintf(moonosBuffer, sizeof(moonosBuffer), "Active MoonOS: %s", summary.moonosLine);
    snprintf(defaultBuffer, sizeof(defaultBuffer), "Default Character Setup: %s",
             sm64dx_has_global_default_moonos() ? sm64dx_get_global_default_pack_name() : "Mario");
    {
        struct CustomLevelInfo *customLevel = smlua_level_util_get_info(gCurrLevelNum);
        if (customLevel != NULL) {
            snprintf(mapBuffer, sizeof(mapBuffer), "Custom Map: %s",
                     customLevel->fullName ? customLevel->fullName : titleBuffer);
        } else if (rom_is_using_custom_hack()) {
            snprintf(mapBuffer, sizeof(mapBuffer), "ROM Hack: %s", rom_get_active_display_name());
        } else {
            snprintf(mapBuffer, sizeof(mapBuffer), "Map Source: Main sm64dx adventure");
        }
    }
    snprintf(controlsBuffer, sizeof(controlsBuffer), "Played %s | %s | Back resumes gameplay",
             summary.lastPlayedLine,
             summary.playTimeLine);

    panel = djui_panel_menu_create(DLANG(PAUSE, PAUSE_TITLE), true);
    djui_panel_pause_style_panel(panel);
    body = djui_three_panel_get_body(panel);

    content = djui_rect_create(body);
    djui_base_set_size_type(&content->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&content->base, 1.0f, 308.0f);
    djui_base_set_color(&content->base, 0, 0, 0, 0);

    actionsCard = djui_rect_create(&content->base);
    djui_base_set_size_type(&actionsCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&actionsCard->base, 0.28f, 1.0f);
    djui_base_set_color(&actionsCard->base, 26, 30, 44, 210);
    djui_base_set_border_width(&actionsCard->base, 2);
    djui_base_set_border_color(&actionsCard->base, 194, 156, 82, 255);
    djui_base_set_padding(&actionsCard->base, 12, 12, 12, 12);

    actionsLayout = djui_flow_layout_create(&actionsCard->base);
    djui_base_set_size_type(&actionsLayout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&actionsLayout->base, 1.0f, 1.0f);
    djui_base_set_color(&actionsLayout->base, 0, 0, 0, 0);
    djui_flow_layout_set_margin(actionsLayout, 8);
    djui_flow_layout_set_flow_direction(actionsLayout, DJUI_FLOW_DIR_DOWN);

    djui_panel_pause_text(&actionsLayout->base, "Pause Hub", gDjuiFonts[2], titleScale, 1, 255, 236, 178);
    {
        struct DjuiButton *resumeButton = djui_button_create(&actionsLayout->base, DLANG(PAUSE, RESUME), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_resume);
        defaultBase = &resumeButton->base;
        djui_button_create(&actionsLayout->base, "Course Info", DJUI_BUTTON_STYLE_NORMAL, djui_panel_course_info_create);
        djui_button_create(&actionsLayout->base, DLANG(PAUSE, OPTIONS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_options_create);
        djui_button_create(&actionsLayout->base, DLANG(PAUSE, PLAYER), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_player);
        djui_button_create(&actionsLayout->base, "MoonOS", DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_moonos);

        struct DjuiButton *exitButton = djui_button_create(&actionsLayout->base, "Exit Level", DJUI_BUTTON_STYLE_BACK, djui_panel_pause_exit_level);
        djui_base_set_enabled(&exitButton->base, djui_panel_pause_allow_exit_level());
    }

    summaryCard = djui_rect_create(&content->base);
    djui_base_set_location_type(&summaryCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_location(&summaryCard->base, 0.31f, 0.0f);
    djui_base_set_size_type(&summaryCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&summaryCard->base, 0.40f, 1.0f);
    djui_base_set_color(&summaryCard->base, 20, 24, 38, 210);
    djui_base_set_border_width(&summaryCard->base, 2);
    djui_base_set_border_color(&summaryCard->base, 110, 164, 230, 255);
    djui_base_set_padding(&summaryCard->base, 14, 14, 14, 14);

    summaryLayout = djui_flow_layout_create(&summaryCard->base);
    djui_base_set_size_type(&summaryLayout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&summaryLayout->base, 1.0f, 1.0f);
    djui_base_set_color(&summaryLayout->base, 0, 0, 0, 0);
    djui_flow_layout_set_margin(summaryLayout, 8);
    djui_flow_layout_set_flow_direction(summaryLayout, DJUI_FLOW_DIR_DOWN);

    djui_panel_pause_text(&summaryLayout->base, titleBuffer, gDjuiFonts[2], titleScale, 1, 255, 240, 170);
    djui_panel_pause_text(&summaryLayout->base, objectiveBuffer, gDjuiFonts[0], detailScale, 2, 220, 220, 220);
    djui_panel_pause_text(&summaryLayout->base, progressBuffer, gDjuiFonts[0], detailScale, 1, 196, 224, 255);
    djui_panel_pause_text(&summaryLayout->base, coinsBuffer, gDjuiFonts[0], detailScale, 1, 182, 235, 188);
    djui_panel_pause_text(&summaryLayout->base, saveBuffer, gDjuiFonts[0], detailScale, 2, 220, 220, 220);

    infoCard = djui_rect_create(&content->base);
    djui_base_set_alignment(&infoCard->base, DJUI_HALIGN_RIGHT, DJUI_VALIGN_TOP);
    djui_base_set_size_type(&infoCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&infoCard->base, 0.25f, 1.0f);
    djui_base_set_color(&infoCard->base, 24, 28, 44, 210);
    djui_base_set_border_width(&infoCard->base, 2);
    djui_base_set_border_color(&infoCard->base, 176, 126, 214, 255);
    djui_base_set_padding(&infoCard->base, 12, 12, 12, 12);

    infoLayout = djui_flow_layout_create(&infoCard->base);
    djui_base_set_size_type(&infoLayout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&infoLayout->base, 1.0f, 1.0f);
    djui_base_set_color(&infoLayout->base, 0, 0, 0, 0);
    djui_flow_layout_set_margin(infoLayout, 8);
    djui_flow_layout_set_flow_direction(infoLayout, DJUI_FLOW_DIR_DOWN);

    djui_panel_pause_text(&infoLayout->base, "Run Identity", gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.62f, 1, 255, 230, 175);
    djui_panel_pause_text(&infoLayout->base, moonosBuffer, gDjuiFonts[0], detailScale, 2, 200, 235, 255);
    djui_panel_pause_text(&infoLayout->base, defaultBuffer, gDjuiFonts[0], detailScale, 2, 224, 210, 255);
    djui_panel_pause_text(&infoLayout->base, mapBuffer, gDjuiFonts[0], detailScale, 2, 186, 216, 255);
    djui_panel_pause_text(&infoLayout->base, summary.unlocksLine, gDjuiFonts[0], detailScale, 1, 220, 220, 220);
    djui_panel_pause_text(&infoLayout->base, controlsBuffer, gDjuiFonts[0], compactScale, 3, 186, 186, 186);

    djui_panel_pause_text(body,
                          "DJUI now owns pause. Resume, MoonOS, options, and exit actions all route through this menu.",
                          gDjuiFonts[0],
                          compactScale,
                          2,
                          188,
                          188,
                          188);

    djui_panel_add(caller, panel, defaultBase);
    gInteractableOverridePad = true;
    gDjuiPanelPauseCreated = true;
}
