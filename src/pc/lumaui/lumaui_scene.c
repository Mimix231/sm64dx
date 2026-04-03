#include "lumaui_scene.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lumaui_assets.h"
#include "lumaui_core.h"
#include "lumaui_render.h"
#include "lumaui_theme.h"

#include "game/save_file.h"
#include "pc/configfile.h"
#include "pc/djui/djui_sm64dx.h"
#include "pc/network/version.h"
#include "pc/pc_main.h"

#define LUMAUI_ARRAY_LEN(arr) ((int) (sizeof(arr) / sizeof((arr)[0])))

static const char *sLumaUITitleButtons[] = {
    "Start",
    "Continue Last File",
    "Options",
    "Quit",
};

static void lumaui_scene_update_vertical_selection(int count, int *selection,
                                                   const struct LumaUIInputState *input) {
    if (count <= 0) {
        *selection = 0;
        return;
    }

    if (*selection < 0 || *selection >= count) {
        *selection = 0;
    }

    if (input->upPressed) {
        *selection = (*selection + count - 1) % count;
    } else if (input->downPressed) {
        *selection = (*selection + 1) % count;
    }
}

static bool lumaui_scene_setup_required(void) {
    return configLanguage[0] == '\0';
}

static int lumaui_scene_language_count(void) {
#ifdef VERSION_EU
    return LANGUAGE_MAX;
#else
    return 1;
#endif
}

static const char *lumaui_scene_language_name(int index) {
#ifdef VERSION_EU
    static const char *sLanguages[] = {
        "English",
        "French",
        "German",
    };

    if (index < 0 || index >= LUMAUI_ARRAY_LEN(sLanguages)) {
        return sLanguages[0];
    }
    return sLanguages[index];
#else
    (void) index;
    return "English";
#endif
}

static int lumaui_scene_get_language_index(void) {
#ifdef VERSION_EU
    if (configLanguage[0] != '\0') {
        for (int i = 0; i < lumaui_scene_language_count(); i++) {
            if (strcmp(configLanguage, lumaui_scene_language_name(i)) == 0) {
                return i;
            }
        }
    }

    {
        int language = (int) eu_get_language();
        if (language >= 0 && language < lumaui_scene_language_count()) {
            return language;
        }
    }
#endif

    return 0;
}

static void lumaui_scene_save_config(void) {
    configfile_save(configfile_name());
}

static void lumaui_scene_apply_language_index(int index, bool persist) {
    int clamped = index;

    if (clamped < 0) {
        clamped = 0;
    }
    if (clamped >= lumaui_scene_language_count()) {
        clamped = lumaui_scene_language_count() - 1;
    }

    snprintf(configLanguage, MAX_CONFIG_STRING, "%s", lumaui_scene_language_name(clamped));
#ifdef VERSION_EU
    eu_set_language((u16) clamped);
#endif

    if (persist) {
        lumaui_scene_save_config();
    }
}

static void lumaui_scene_toggle_show_fps(void) {
    configShowFPS = !configShowFPS;
    lumaui_scene_save_config();
}

static void lumaui_scene_finish_setup(struct LumaUIState *state) {
    (void) state;
    lumaui_scene_apply_language_index(lumaui_scene_get_language_index(), true);
    lumaui_core_pop_scene();
}

static int lumaui_scene_default_save_slot_index(void) {
    int recentSlot = sm64dx_get_recent_save_slot();
    if (recentSlot >= 1 && recentSlot <= NUM_SAVE_FILES) {
        return recentSlot - 1;
    }

    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        struct Sm64dxSaveSummary summary = { 0 };
        sm64dx_build_save_summary(i + 1, &summary);
        if (summary.exists) {
            return i;
        }
    }

    return 0;
}

static struct LumaUIRect lumaui_title_button_rect(int index) {
    struct LumaUIRect rect = { 150, (s16) (96 + index * 24), 110, 20 };
    return rect;
}

static struct LumaUIRect lumaui_save_card_rect(int index) {
    s16 x = (index % 2 == 0) ? 30 : 164;
    s16 y = (index < 2) ? 74 : 126;
    struct LumaUIRect rect = { x, y, 126, 46 };
    return rect;
}

static struct LumaUIRect lumaui_save_button_rect(bool primary) {
    struct LumaUIRect rect = {
        196,
        primary ? 176 : 198,
        74,
        18,
    };
    return rect;
}

static struct LumaUIRect lumaui_option_button_rect(int index) {
    struct LumaUIRect rect = { 72, (s16) (118 + index * 24), 176, 20 };
    return rect;
}

static struct LumaUIRect lumaui_pause_button_rect(void) {
    struct LumaUIRect rect = { 62, 104, 96, 20 };
    return rect;
}

static void lumaui_scene_build_recent_summary(char *buffer, size_t size,
                                              const struct Sm64dxSaveSummary *summary) {
    if (summary == NULL || !summary->exists) {
        snprintf(buffer, size,
                 "No recent file.\n"
                 "Press Start to open\n"
                 "the save selector.");
        return;
    }

    snprintf(buffer, size,
             "%s\n"
             "%s\n"
             "%s\n"
             "%s",
             summary->title,
             summary->name,
             summary->starsLine,
             summary->lastPlayedLine);
}

static void lumaui_scene_build_selected_save_details(char *buffer, size_t size,
                                                     const struct Sm64dxSaveSummary *summary) {
    if (summary == NULL) {
        buffer[0] = '\0';
        return;
    }

    snprintf(buffer, size,
             "%s\n"
             "%s",
             summary->progressionLine,
             summary->playTimeLine);
}

static void lumaui_title_activate(struct LumaUIState *state, int index) {
    (void) state;

    switch (index) {
        case 0:
            lumaui_core_push_scene(LUMAUI_SCENE_SAVE_SELECT);
            break;
        case 1: {
            int slot = sm64dx_get_recent_save_slot();
            if (slot >= 1 && slot <= NUM_SAVE_FILES) {
                lumaui_core_queue_start_save_slot(slot, true);
            } else {
                lumaui_core_open_modal("No Recent File",
                                       "A recent file has not been recorded yet.\n"
                                       "Use Start to choose a save slot first.");
            }
            break;
        }
        case 2:
            lumaui_core_push_scene(LUMAUI_SCENE_OPTIONS);
            break;
        case 3:
            game_exit();
            break;
        default:
            break;
    }
}

static void lumaui_save_select_activate(struct LumaUIState *state, int slotIndex) {
    state->selectedIndex[LUMAUI_SCENE_SAVE_SELECT] = slotIndex;
    lumaui_core_queue_start_save_slot(slotIndex + 1, true);
}

static void lumaui_options_activate(struct LumaUIState *state, int index) {
    int languageIndex = lumaui_scene_get_language_index();
    (void) state;

    switch (index) {
        case 0:
            languageIndex = (languageIndex + 1) % lumaui_scene_language_count();
            lumaui_scene_apply_language_index(languageIndex, true);
            break;
        case 1:
            lumaui_scene_toggle_show_fps();
            break;
        case 2:
            if (lumaui_scene_setup_required()) {
                lumaui_scene_apply_language_index(languageIndex, true);
            }
            lumaui_core_pop_scene();
            break;
        default:
            break;
    }
}

static void lumaui_pause_activate(void) {
    lumaui_core_request_pause_resume();
}

static void lumaui_scene_update_title(struct LumaUIState *state) {
    const struct LumaUIInputState *input = &state->input;
    int *selection = &state->selectedIndex[LUMAUI_SCENE_TITLE];
    const int count = LUMAUI_ARRAY_LEN(sLumaUITitleButtons);

    if (lumaui_scene_setup_required() && !lumaui_core_scene_is_active(LUMAUI_SCENE_OPTIONS)) {
        state->selectedIndex[LUMAUI_SCENE_OPTIONS] = 0;
        lumaui_core_push_scene(LUMAUI_SCENE_OPTIONS);
        return;
    }

    lumaui_scene_update_vertical_selection(count, selection, input);

    for (int i = 0; i < count; i++) {
        struct LumaUIRect rect = lumaui_title_button_rect(i);
        if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &rect)) {
            *selection = i;
            if (input->pointerPressed) {
                lumaui_title_activate(state, i);
                return;
            }
        }
    }

    if (input->confirmPressed) {
        lumaui_title_activate(state, *selection);
    } else if (input->backPressed) {
        game_exit();
    }
}

static void lumaui_scene_update_save_select(struct LumaUIState *state) {
    const struct LumaUIInputState *input = &state->input;
    int *selection = &state->selectedIndex[LUMAUI_SCENE_SAVE_SELECT];

    if (*selection < 0 || *selection >= NUM_SAVE_FILES) {
        *selection = lumaui_scene_default_save_slot_index();
    }

    if (input->leftPressed && (*selection % 2) == 1) {
        (*selection)--;
    } else if (input->rightPressed && (*selection % 2) == 0 && *selection + 1 < NUM_SAVE_FILES) {
        (*selection)++;
    } else if (input->upPressed && *selection >= 2) {
        *selection -= 2;
    } else if (input->downPressed && *selection + 2 < NUM_SAVE_FILES) {
        *selection += 2;
    }

    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        struct LumaUIRect rect = lumaui_save_card_rect(i);
        if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &rect)) {
            *selection = i;
            if (input->pointerPressed) {
                lumaui_save_select_activate(state, i);
                return;
            }
        }
    }

    {
        struct LumaUIRect primary = lumaui_save_button_rect(true);
        struct LumaUIRect secondary = lumaui_save_button_rect(false);

        if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &primary) && input->pointerPressed) {
            lumaui_save_select_activate(state, *selection);
            return;
        }
        if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &secondary) && input->pointerPressed) {
            lumaui_core_pop_scene();
            return;
        }
    }

    if (input->confirmPressed) {
        lumaui_save_select_activate(state, *selection);
    } else if (input->backPressed) {
        lumaui_core_pop_scene();
    }
}

static void lumaui_scene_update_options(struct LumaUIState *state) {
    const struct LumaUIInputState *input = &state->input;
    int *selection = &state->selectedIndex[LUMAUI_SCENE_OPTIONS];
    const int count = 3;

    lumaui_scene_update_vertical_selection(count, selection, input);

    if (*selection == 0 && (input->leftPressed || input->rightPressed)) {
        int languageIndex = lumaui_scene_get_language_index();
        int direction = input->leftPressed ? -1 : 1;
        int next = (languageIndex + direction + lumaui_scene_language_count()) % lumaui_scene_language_count();
        lumaui_scene_apply_language_index(next, true);
    } else if (*selection == 1 && (input->leftPressed || input->rightPressed)) {
        lumaui_scene_toggle_show_fps();
    }

    for (int i = 0; i < count; i++) {
        struct LumaUIRect rect = lumaui_option_button_rect(i);
        if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &rect)) {
            *selection = i;
            if (input->pointerPressed) {
                lumaui_options_activate(state, i);
                return;
            }
        }
    }

    if (input->confirmPressed) {
        lumaui_options_activate(state, *selection);
    } else if (input->backPressed) {
        if (lumaui_scene_setup_required()) {
            lumaui_scene_finish_setup(state);
        } else {
            lumaui_core_pop_scene();
        }
    }
}

static void lumaui_scene_update_pause(struct LumaUIState *state) {
    const struct LumaUIInputState *input = &state->input;
    struct LumaUIRect button = lumaui_pause_button_rect();

    state->selectedIndex[LUMAUI_SCENE_PAUSE] = 0;

    if (input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &button) && input->pointerPressed) {
        lumaui_pause_activate();
        return;
    }

    if (input->confirmPressed || input->backPressed) {
        lumaui_pause_activate();
    }
}

void lumaui_scene_update(struct LumaUIState *state) {
    if (state->modal.active) {
        if (state->input.confirmPressed || state->input.backPressed || state->input.pointerPressed) {
            lumaui_core_close_modal();
        }
        return;
    }

    switch (lumaui_core_get_active_scene()) {
        case LUMAUI_SCENE_TITLE:
            lumaui_scene_update_title(state);
            break;
        case LUMAUI_SCENE_SAVE_SELECT:
            lumaui_scene_update_save_select(state);
            break;
        case LUMAUI_SCENE_OPTIONS:
            lumaui_scene_update_options(state);
            break;
        case LUMAUI_SCENE_PAUSE:
            lumaui_scene_update_pause(state);
            break;
        default:
            break;
    }
}

static void lumaui_scene_render_title(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIInputState *input = &state->input;
    struct LumaUIRect shell = { 22, 14, 276, 208 };
    struct LumaUIRect headerCard = { 34, 24, 230, 26 };
    struct LumaUIRect infoCard = { 34, 66, 96, 124 };
    struct LumaUIRect menuCard = { 142, 66, 126, 130 };
    struct LumaUIRect badge = { 42, 29, 56, 16 };
    struct LumaUIRect infoBadge = { infoCard.x + 8, infoCard.y + 8, 60, 16 };
    struct LumaUIRect menuBadge = { menuCard.x + 8, menuCard.y + 8, 44, 16 };
    struct LumaUIRect infoClip = { infoCard.x + 8, infoCard.y + 30, infoCard.w - 16, infoCard.h - 38 };
    struct Sm64dxSaveSummary summary = { 0 };
    char infoText[256] = { 0 };

    lumaui_render_backdrop();
    lumaui_render_panel(&shell, &theme->panel, &theme->panelBorder);
    lumaui_render_card(&headerCard, true);
    lumaui_render_card(&infoCard, false);
    lumaui_render_card(&menuCard, true);
    lumaui_render_badge(&badge, "Offline");

    lumaui_render_text_centered(149, headerCard.y + 4, lumaui_assets_brand_name(), &theme->text);
    lumaui_render_text_centered(199, headerCard.y + 4, "Phase 3", &theme->mutedText);
    lumaui_render_text_centered(160, 54, "Offline-first adventure.", &theme->mutedText);

    if (sm64dx_has_recent_save()) {
        int slot = sm64dx_get_recent_save_slot();
        sm64dx_build_save_summary(slot, &summary);
    } else {
        memset(&summary, 0, sizeof(summary));
    }
    lumaui_scene_build_recent_summary(infoText, sizeof(infoText), &summary);

    lumaui_render_badge(&infoBadge, "Recent");
    lumaui_render_badge(&menuBadge, "Play");
    lumaui_render_push_clip(&infoClip);
    lumaui_render_text_block_wrapped(infoCard.x + 8, infoCard.y + 32, infoCard.w - 16, infoText, &theme->text);
    lumaui_render_pop_clip();

    for (int i = 0; i < LUMAUI_ARRAY_LEN(sLumaUITitleButtons); i++) {
        struct LumaUIRect rect = lumaui_title_button_rect(i);
        struct LumaUIButtonSpec button = {
            .rect = rect,
            .label = sLumaUITitleButtons[i],
            .primary = (i == 0),
            .selected = (state->selectedIndex[LUMAUI_SCENE_TITLE] == i),
            .hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &rect),
        };
        lumaui_render_button(&button);
    }

    if (lumaui_scene_setup_required()) {
        lumaui_render_text(150, 196, "Finish setup in Options.", &theme->accent);
    }

    lumaui_render_action_bar("A Select  B Quit", get_version());
}

static void lumaui_scene_render_save_select(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIInputState *input = &state->input;
    struct LumaUIRect shell = { 18, 12, 284, 212 };
    struct LumaUIRect headerCard = { 30, 24, 240, 26 };
    struct LumaUIRect badge = { 30, 56, 54, 16 };
    struct LumaUIRect detailCard = { 30, 178, 150, 34 };
    struct LumaUIRect detailClip = { detailCard.x + 8, detailCard.y + 8, detailCard.w - 16, detailCard.h - 12 };
    int selection = state->selectedIndex[LUMAUI_SCENE_SAVE_SELECT];
    struct Sm64dxSaveSummary selectedSummary = { 0 };
    char detailText[160] = { 0 };

    if (selection < 0 || selection >= NUM_SAVE_FILES) {
        selection = lumaui_scene_default_save_slot_index();
    }
    sm64dx_build_save_summary(selection + 1, &selectedSummary);
    lumaui_scene_build_selected_save_details(detailText, sizeof(detailText), &selectedSummary);

    lumaui_render_backdrop();
    lumaui_render_panel(&shell, &theme->panel, &theme->panelBorder);
    lumaui_render_card(&headerCard, true);
    lumaui_render_badge(&badge, "Saves");
    lumaui_render_text_centered(160, headerCard.y + 4, lumaui_assets_scene_name(LUMAUI_SCENE_SAVE_SELECT), &theme->text);
    lumaui_render_text_centered(160, 58, "Pick a file and launch.", &theme->mutedText);

    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        struct Sm64dxSaveSummary summary = { 0 };
        struct LumaUIRect card = lumaui_save_card_rect(i);
        bool hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &card);

        sm64dx_build_save_summary(i + 1, &summary);
        lumaui_render_card(&card, selection == i || hovered);
        lumaui_render_text(card.x + 6, card.y + 7, summary.title, &theme->accent);
        lumaui_render_text(card.x + 6, card.y + 26, summary.exists ? summary.starsLine : summary.action,
                           summary.exists ? &theme->text : &theme->mutedText);
    }

    lumaui_render_card(&detailCard, true);
    lumaui_render_push_clip(&detailClip);
    lumaui_render_text(detailCard.x + 8, detailCard.y + 8, selectedSummary.name, &theme->accent);
    lumaui_render_text_block_wrapped(detailCard.x + 8, detailCard.y + 20, detailCard.w - 16, detailText, &theme->text);
    lumaui_render_pop_clip();

    {
        struct LumaUIRect primary = lumaui_save_button_rect(true);
        struct LumaUIRect secondary = lumaui_save_button_rect(false);
        struct LumaUIButtonSpec startButton = {
            .rect = primary,
            .label = "Launch",
            .primary = true,
            .selected = true,
            .hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &primary),
        };
        struct LumaUIButtonSpec backButton = {
            .rect = secondary,
            .label = "Back",
            .primary = false,
            .selected = false,
            .hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &secondary),
        };

        lumaui_render_button(&startButton);
        lumaui_render_button(&backButton);
    }

    lumaui_render_action_bar("A Launch  B Back", get_version());
}

static void lumaui_scene_render_options(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIInputState *input = &state->input;
    struct LumaUIRect shell = { 44, 22, 232, 194 };
    struct LumaUIRect headerCard = { 58, 34, 204, 24 };
    struct LumaUIRect badge = { 60, 66, 64, 16 };
    char languageLabel[64] = { 0 };
    char fpsLabel[64] = { 0 };
    const bool firstRun = lumaui_scene_setup_required();

    snprintf(languageLabel, sizeof(languageLabel), "Language: %s", lumaui_scene_language_name(lumaui_scene_get_language_index()));
    snprintf(fpsLabel, sizeof(fpsLabel), "Show FPS: %s", configShowFPS ? "On" : "Off");

    lumaui_render_backdrop();
    lumaui_render_panel(&shell, &theme->panel, &theme->panelBorder);
    lumaui_render_card(&headerCard, true);
    lumaui_render_badge(&badge, firstRun ? "Setup" : "Options");
    lumaui_render_text_centered(160, headerCard.y + 4, lumaui_assets_scene_name(LUMAUI_SCENE_OPTIONS), &theme->text);
    lumaui_render_text_centered(160, 68, firstRun ? "Select language and confirm." : "Core startup preferences.", &theme->mutedText);

    lumaui_render_text_block_wrapped(60, 86, shell.w - 32,
                             firstRun
                                 ? "Choose the interface language now.\nThe rest of the settings stack\nmoves into LumaUI later."
                                 : "Use left or right on the selected row.\nThis scene will grow into the full\nsettings stack in later phases.",
                             &theme->text);

    for (int i = 0; i < 3; i++) {
        const char *label = (i == 0) ? languageLabel : (i == 1) ? fpsLabel : (firstRun ? "Finish Setup" : "Back To Title");
        struct LumaUIRect rect = lumaui_option_button_rect(i);
        struct LumaUIButtonSpec button = {
            .rect = rect,
            .label = label,
            .primary = (i == 2),
            .selected = (state->selectedIndex[LUMAUI_SCENE_OPTIONS] == i),
            .hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &rect),
        };
        lumaui_render_button(&button);
    }

    lumaui_render_action_bar("Left/Right Change  A Select  B Back", firstRun ? "First Run" : get_version());
}

static void lumaui_scene_render_pause(struct LumaUIState *state) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIInputState *input = &state->input;
    struct LumaUIRect shell = { 56, 30, 208, 162 };
    struct LumaUIRect headerCard = { 72, 42, 176, 24 };
    struct LumaUIRect infoCard = { 72, 78, 176, 64 };
    struct LumaUIRect infoBadge = { infoCard.x + 8, infoCard.y + 8, 72, 16 };
    struct LumaUIRect buttonRect = { 114, 152, 92, 20 };
    struct LumaUIRect infoClip = { infoCard.x + 8, infoCard.y + 28, infoCard.w - 16, infoCard.h - 36 };
    struct LumaUIButtonSpec button = {
        .rect = buttonRect,
        .label = "Resume",
        .primary = true,
        .selected = true,
        .hovered = input->cursorVisible && lumaui_render_point_in_rect(input->cursorX, input->cursorY, &buttonRect),
    };

    (void) state;

    lumaui_render_backdrop();
    lumaui_render_panel(&shell, &theme->panel, &theme->panelBorder);
    lumaui_render_card(&headerCard, true);
    lumaui_render_card(&infoCard, true);
    lumaui_render_badge(&infoBadge, "Gameplay");
    lumaui_render_text_centered(160, headerCard.y + 4, lumaui_assets_scene_name(LUMAUI_SCENE_PAUSE), &theme->text);
    lumaui_render_text_centered(160, 72, "Resume or go back.", &theme->mutedText);
    lumaui_render_push_clip(&infoClip);
    lumaui_render_text_block_wrapped(infoCard.x + 8, infoCard.y + 30, infoCard.w - 16,
                                     "Course info, player, and MoonOS move into this pause flow next.",
                                     &theme->text);
    lumaui_render_pop_clip();
    lumaui_render_button(&button);
    lumaui_render_action_bar("A Resume  B Resume", "Pause");
}

void lumaui_scene_render(struct LumaUIState *state) {
    switch (lumaui_core_get_active_scene()) {
        case LUMAUI_SCENE_TITLE:
            lumaui_scene_render_title(state);
            break;
        case LUMAUI_SCENE_SAVE_SELECT:
            lumaui_scene_render_save_select(state);
            break;
        case LUMAUI_SCENE_OPTIONS:
            lumaui_scene_render_options(state);
            break;
        case LUMAUI_SCENE_PAUSE:
            lumaui_scene_render_pause(state);
            break;
        default:
            break;
    }
}
