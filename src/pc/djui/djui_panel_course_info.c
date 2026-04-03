#include <stdio.h>
#include <string.h>

#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_course_info.h"
#include "djui_panel_menu.h"
#include "djui_sm64dx.h"

#include "game/area.h"
#include "game/level_info.h"
#include "game/save_file.h"
#include "pc/lua/utils/smlua_level_utils.h"
#include "pc/rom_checker.h"

static struct DjuiText *djui_panel_course_info_text(struct DjuiBase *parent, const char *message, float height, u8 r, u8 g, u8 b) {
    struct DjuiText *text = djui_text_create(parent, message);
    djui_base_set_size_type(&text->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&text->base, 1.0f, height);
    djui_base_set_color(&text->base, r, g, b, 255);
    djui_text_set_drop_shadow(text, 48, 48, 48, 100);
    return text;
}

void djui_panel_course_info_create(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create("Course Info", false);
    struct DjuiBase *body = djui_three_panel_get_body(panel);

    char courseBuffer[128] = { 0 };
    snprintf(courseBuffer, sizeof(courseBuffer), "%s",
             get_level_name_ascii(gCurrCourseNum, gCurrLevelNum, gCurrAreaIndex, 1));

    int slotIndex = (gCurrSaveFileNum > 0) ? (gCurrSaveFileNum - 1) : 0;
    int courseIndex = (gCurrCourseNum > 0) ? (gCurrCourseNum - 1) : -1;
    int starsCollected = (courseIndex >= 0) ? save_file_get_course_star_count(slotIndex, courseIndex) : save_file_get_course_star_count(slotIndex, -1);
    int bestCoins = (courseIndex >= 0) ? save_file_get_course_coin_score(slotIndex, courseIndex) : 0;

    char statsBuffer[128] = { 0 };
    snprintf(statsBuffer, sizeof(statsBuffer), "Stars Collected: %d | Best Coins: %d", starsCollected, bestCoins);
    char mapBuffer[160] = { 0 };
    struct CustomLevelInfo *customLevel = smlua_level_util_get_info(gCurrLevelNum);

    if (customLevel != NULL) {
        snprintf(mapBuffer, sizeof(mapBuffer), "Custom Map: %s | Id: %s",
                 customLevel->fullName ? customLevel->fullName : courseBuffer,
                 customLevel->shortName ? customLevel->shortName : "custom");
    } else if (rom_is_using_custom_hack()) {
        snprintf(mapBuffer, sizeof(mapBuffer), "ROM Hack: %s", rom_get_active_display_name());
    } else {
        snprintf(mapBuffer, sizeof(mapBuffer), "Map Source: Main sm64dx adventure");
    }

    char starListBuffer[512] = { 0 };
    if (courseIndex >= 0) {
        u32 starFlags = save_file_get_star_flags(slotIndex, courseIndex);
        bool first = true;
        for (int star = 1; star <= 6; star++) {
            if ((starFlags & (1 << (star - 1))) == 0) {
                continue;
            }
            if (!first) {
                strncat(starListBuffer, "\n", sizeof(starListBuffer) - strlen(starListBuffer) - 1);
            }
            strncat(starListBuffer, get_star_name_ascii(gCurrCourseNum, star, 1), sizeof(starListBuffer) - strlen(starListBuffer) - 1);
            first = false;
        }

        if (bestCoins >= 100) {
            if (!first) {
                strncat(starListBuffer, "\n", sizeof(starListBuffer) - strlen(starListBuffer) - 1);
            }
            strncat(starListBuffer, "100 Coins", sizeof(starListBuffer) - strlen(starListBuffer) - 1);
        }

        if (first) {
            snprintf(starListBuffer, sizeof(starListBuffer), "No stars collected on this course yet.");
        }
    } else {
        snprintf(starListBuffer, sizeof(starListBuffer), "Castle and secret star progress is tracked in the file summary.");
    }

    struct Sm64dxSaveSummary summary = { 0 };
    sm64dx_build_save_summary(gCurrSaveFileNum, &summary);

    {
        struct DjuiText *courseTitle = djui_panel_course_info_text(body, courseBuffer, 40, 255, 240, 170);
        djui_text_set_font(courseTitle, gDjuiFonts[2]);
        djui_text_set_alignment(courseTitle, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

        djui_panel_course_info_text(body, statsBuffer, 24, 220, 220, 220);
        djui_panel_course_info_text(body, mapBuffer, 24, 186, 216, 255);
        djui_panel_course_info_text(body, starListBuffer, 132, 220, 220, 220);
        djui_panel_course_info_text(body, summary.progressionLine, 24, 220, 220, 220);
        djui_panel_course_info_text(body, summary.unlocksLine, 24, 220, 220, 220);
        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
