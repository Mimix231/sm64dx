#include <stdio.h>

#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_confirm.h"
#include "djui_panel_main.h"
#include "djui_panel_menu.h"
#include "djui_panel_player.h"
#include "djui_panel_start.h"
#include "djui_paginated.h"
#include "djui_sm64dx.h"

#include "game/characters.h"
#include "game/save_file.h"
#include "pc/configfile.h"

static int sManageSlot = 0;
static int sCopySourceSlot = 0;

static float djui_panel_start_measure_text_height(const struct DjuiFont *font, float fontScale, int lineCount, float padding) {
    if (font == NULL) {
        return 0.0f;
    }
    if (lineCount < 1) {
        lineCount = 1;
    }
    return ((font->charHeight + ((float)(lineCount - 1) * font->lineHeight)) * fontScale) + padding;
}

static void djui_panel_start_style_panel(struct DjuiThreePanel *panel, float height) {
    struct DjuiFlowLayout *body = (struct DjuiFlowLayout *) djui_three_panel_get_body(panel);
    djui_base_set_size_type(&panel->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&panel->base, 620.0f, height);
    djui_base_set_alignment(&panel->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_base_set_color(&panel->base, 20, 18, 30, 242);
    djui_base_set_border_color(&panel->base, 212, 176, 92, 255);
    djui_base_set_border_width(&panel->base, 6);
    djui_base_set_gradient(&panel->base, false);
    djui_flow_layout_set_margin(body, 10);
}

static void djui_panel_start_refresh(void) {
    djui_panel_shutdown();
    gDjuiInMainMenu = true;
    djui_panel_main_create(NULL);
    djui_panel_start_create(NULL);
}

static struct DjuiText *djui_panel_start_card_text(struct DjuiBase *parent, const char *message, float fontScale, int lineCount, u8 r, u8 g, u8 b) {
    struct DjuiText *text = djui_text_create(parent, message);
    float height = djui_panel_start_measure_text_height(gDjuiFonts[0], fontScale, lineCount, 4.0f);
    djui_base_set_size_type(&text->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&text->base, 1.0f, height);
    djui_base_set_color(&text->base, r, g, b, 255);
    djui_text_set_drop_shadow(text, 48, 48, 48, 100);
    djui_text_set_font_scale(text, fontScale);
    return text;
}

static void djui_panel_start_add_summary_row(struct DjuiBase *parent, const char *message, float fontScale) {
    struct DjuiText *summaryText = djui_text_create(parent, message);
    float height = djui_panel_start_measure_text_height(gDjuiFonts[0], fontScale, 1, 3.0f);
    djui_base_set_size_type(&summaryText->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&summaryText->base, 1.0f, height);
    djui_base_set_color(&summaryText->base, 220, 220, 220, 255);
    djui_text_set_drop_shadow(summaryText, 48, 48, 48, 100);
    djui_text_set_font_scale(summaryText, fontScale);
}

static void djui_panel_start_add_meta_row(struct DjuiBase *parent, const char *played, const char *time, float fontScale) {
    float rowHeight = djui_panel_start_measure_text_height(gDjuiFonts[0], fontScale, 1, 3.0f);
    struct DjuiRect *metaRow = djui_rect_container_create(parent, rowHeight);

    char playedBuffer[96] = { 0 };
    snprintf(playedBuffer, sizeof(playedBuffer), "Played %s", played);

    struct DjuiText *playedText = djui_text_create(&metaRow->base, playedBuffer);
    djui_base_set_size_type(&playedText->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&playedText->base, 0.74f, rowHeight);
    djui_base_set_color(&playedText->base, 190, 190, 190, 255);
    djui_text_set_drop_shadow(playedText, 48, 48, 48, 100);
    djui_text_set_font_scale(playedText, fontScale);

    struct DjuiText *timeText = djui_text_create(&metaRow->base, time);
    djui_base_set_size_type(&timeText->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&timeText->base, 0.22f, rowHeight);
    djui_base_set_alignment(&timeText->base, DJUI_HALIGN_RIGHT, DJUI_VALIGN_TOP);
    djui_base_set_color(&timeText->base, 190, 190, 190, 255);
    djui_text_set_drop_shadow(timeText, 48, 48, 48, 100);
    djui_text_set_alignment(timeText, DJUI_HALIGN_RIGHT, DJUI_VALIGN_CENTER);
    djui_text_set_font_scale(timeText, fontScale);
}

static void djui_panel_start_begin(struct DjuiBase *caller) {
    int slot = (int) caller->tag;
    configHostSaveSlot = slot;
    sm64dx_start_save_slot(slot, true);
}

static void djui_panel_start_player_setup(struct DjuiBase *caller) {
    int slot = (int) caller->tag;
    configHostSaveSlot = slot;
    sm64dx_apply_save_setup(slot);
    djui_panel_player_create(caller);
}

static void djui_panel_start_copy_target(struct DjuiBase *caller) {
    int targetSlot = (int) caller->tag;
    sm64dx_copy_save_slot(sCopySourceSlot, targetSlot);
    djui_panel_start_refresh();
}

static void djui_panel_start_copy_create(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create("Copy Save", false);
    djui_panel_start_style_panel(panel, 0.56f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    {
        char infoBuffer[96] = { 0 };
        snprintf(infoBuffer, sizeof(infoBuffer), "Copy File %c into another slot.", 'A' + (sCopySourceSlot - 1));
        struct DjuiText *info = djui_panel_start_card_text(body, infoBuffer, gDjuiFonts[0]->defaultFontScale * 0.60f, 1, 220, 220, 220);
        djui_text_set_alignment(info, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

        for (int slot = 1; slot <= NUM_SAVE_FILES; slot++) {
            char label[96] = { 0 };
            snprintf(label, sizeof(label), "Copy To File %c", 'A' + (slot - 1));
            struct DjuiButton *button = djui_button_create(body, label, DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_copy_target);
            button->base.tag = slot;
            djui_base_set_enabled(&button->base, slot != sCopySourceSlot);
        }

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_start_copy(struct DjuiBase *caller) {
    sCopySourceSlot = (int) caller->tag;
    djui_panel_start_copy_create(caller);
}

static void djui_panel_start_erase_yes(UNUSED struct DjuiBase *caller) {
    sm64dx_erase_save_slot(sManageSlot);
    djui_panel_start_refresh();
}

static void djui_panel_start_erase(struct DjuiBase *caller) {
    sManageSlot = (int) caller->tag;
    djui_panel_confirm_create(caller,
                              "Erase Save",
                              "This will permanently erase the selected file.",
                              djui_panel_start_erase_yes);
}

static void djui_panel_start_manage(struct DjuiBase *caller) {
    sManageSlot = (int) caller->tag;

    struct Sm64dxSaveSummary summary = { 0 };
    sm64dx_build_save_summary(sManageSlot, &summary);

    char header[64] = { 0 };
    snprintf(header, sizeof(header), "%s", summary.title);

    struct DjuiThreePanel *panel = djui_panel_menu_create(header, false);
    djui_panel_start_style_panel(panel, 0.78f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    {
        struct DjuiText *nameText = djui_panel_start_card_text(body, summary.name, gDjuiFonts[0]->defaultFontScale * 0.68f, 1, 255, 255, 255);
        djui_text_set_font(nameText, gDjuiFonts[2]);
        djui_text_set_alignment(nameText, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_font_scale(nameText, gDjuiFonts[2]->defaultFontScale * 0.88f);

        djui_panel_start_card_text(body, summary.starsLine, gDjuiFonts[0]->defaultFontScale * 0.56f, 1, 255, 225, 140);
        djui_panel_start_card_text(body, summary.progressionLine, gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 220, 220, 220);
        djui_panel_start_card_text(body, summary.unlocksLine, gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 220, 220, 220);

        char moonosBuffer[160] = { 0 };
        snprintf(moonosBuffer, sizeof(moonosBuffer), "MoonOS Pack: %s", summary.moonosLine);
        djui_panel_start_card_text(body, moonosBuffer, gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 200, 235, 255);

        djui_panel_start_add_meta_row(body, summary.lastPlayedLine, summary.playTimeLine, gDjuiFonts[0]->defaultFontScale * 0.50f);

        struct DjuiButton *startButton = djui_button_create(body, summary.action, DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_begin);
        startButton->base.tag = sManageSlot;

        struct DjuiButton *playerButton = djui_button_create(body, "Player Setup", DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_player_setup);
        playerButton->base.tag = sManageSlot;

        struct DjuiRect *actionsRow = djui_rect_container_create(body, 64);
        {
            struct DjuiButton *copyButton = djui_button_left_create(&actionsRow->base, "Copy", DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_copy);
            copyButton->base.tag = sManageSlot;

            struct DjuiButton *eraseButton = djui_button_right_create(&actionsRow->base, "Erase", DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_erase);
            eraseButton->base.tag = sManageSlot;
        }

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_start_add_card(struct DjuiBase *parent, int slot) {
    struct Sm64dxSaveSummary summary = { 0 };
    sm64dx_build_save_summary(slot, &summary);

    const float titleScale = gDjuiFonts[2]->defaultFontScale * 0.72f;
    const float nameScale = gDjuiFonts[0]->defaultFontScale * 0.60f;
    const float detailScale = gDjuiFonts[0]->defaultFontScale * 0.52f;
    const float titleRowHeight = djui_panel_start_measure_text_height(gDjuiFonts[2], titleScale, 1, 6.0f);
    const float nameHeight = djui_panel_start_measure_text_height(gDjuiFonts[0], nameScale, 1, 4.0f);
    const float detailHeight = djui_panel_start_measure_text_height(gDjuiFonts[0], detailScale, 1, 3.0f);
    const float buttonRowHeight = 30.0f;
    const float cardHeight = 20.0f + 8.0f
                           + titleRowHeight + nameHeight + detailHeight + detailHeight + detailHeight + buttonRowHeight
                           + (5.0f * 4.0f);

    struct DjuiRect *card = djui_rect_create(parent);
    djui_base_set_size_type(&card->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&card->base, 1.0f, cardHeight);
    djui_base_set_color(&card->base, 18, 22, 34, 214);
    djui_base_set_gradient(&card->base, false);
    djui_base_set_border_width(&card->base, 2);
    djui_base_set_border_color(&card->base, 200, 162, 80, 255);
    djui_base_set_padding(&card->base, 10, 10, 10, 10);

    struct DjuiFlowLayout *layout = djui_flow_layout_create(&card->base);
    djui_base_set_size_type(&layout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&layout->base, 1.0f, 1.0f);
    djui_base_set_color(&layout->base, 0, 0, 0, 0);
    djui_base_set_padding(&layout->base, 4, 4, 4, 4);
    djui_flow_layout_set_margin(layout, 4);
    djui_flow_layout_set_flow_direction(layout, DJUI_FLOW_DIR_DOWN);

    struct DjuiRect *titleRow = djui_rect_container_create(&layout->base, titleRowHeight);
    {
        struct DjuiText *title = djui_text_create(&titleRow->base, summary.title);
        djui_base_set_size_type(&title->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&title->base, 1.0f, titleRowHeight);
        djui_base_set_color(&title->base, 255, 240, 170, 255);
        djui_text_set_font(title, gDjuiFonts[2]);
        djui_text_set_drop_shadow(title, 48, 48, 48, 100);
        djui_text_set_font_scale(title, titleScale);
        djui_base_set_alignment(&title->base, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
    }

    djui_panel_start_card_text(&layout->base, summary.name, nameScale, 1, 255, 255, 255);
    djui_panel_start_card_text(&layout->base, summary.starsLine, detailScale, 1, 255, 225, 140);

    char completionLine[256] = { 0 };
    snprintf(completionLine, sizeof(completionLine), "%s | %s", summary.progressionLine, summary.unlocksLine);
    djui_panel_start_add_summary_row(&layout->base, completionLine, detailScale);

    djui_panel_start_add_meta_row(&layout->base, summary.lastPlayedLine, summary.playTimeLine, detailScale);

    struct DjuiRect *buttonRow = djui_rect_container_create(&layout->base, buttonRowHeight);
    {
        struct DjuiButton *startButton = djui_button_left_create(&buttonRow->base, summary.action, DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_begin);
        startButton->base.tag = slot;
        djui_base_set_size(&startButton->base, 0.485f, buttonRowHeight);

        struct DjuiButton *manageButton = djui_button_right_create(&buttonRow->base, "Manage", DJUI_BUTTON_STYLE_NORMAL, djui_panel_start_manage);
        manageButton->base.tag = slot;
        djui_base_set_size(&manageButton->base, 0.485f, buttonRowHeight);
    }
}

void djui_panel_start_create(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create("Select File", false);
    djui_panel_start_style_panel(panel, 0.9f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    {
        struct DjuiText *intro = djui_panel_start_card_text(body, "Choose a save file and launch directly into the adventure.", gDjuiFonts[0]->defaultFontScale * 0.56f, 1, 220, 220, 220);
        djui_text_set_alignment(intro, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

        struct DjuiPaginated *paginated = djui_paginated_create(body, 2);
        paginated->showMaxCount = false;
        for (int slot = 1; slot <= NUM_SAVE_FILES; slot++) {
            djui_panel_start_add_card(&paginated->layout->base, slot);
        }
        djui_paginated_calculate_height(paginated);
        panel->bodySize.value = intro->base.height.value + paginated->base.height.value + 16.0f + 64.0f;

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
