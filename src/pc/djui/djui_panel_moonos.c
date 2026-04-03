#include <stdio.h>
#include <string.h>

#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_main.h"
#include "djui_panel_menu.h"
#include "djui_panel_moonos.h"
#include "djui_panel_options.h"
#include "djui_panel_pause.h"
#include "djui_panel_player.h"
#include "djui_paginated.h"
#include "djui_sm64dx.h"

#include "game/characters.h"
#include "game/save_file.h"
#include "pc/configfile.h"

enum Sm64dxMoonosFilter {
    SM64DX_MOONOS_FILTER_INSTALLED,
    SM64DX_MOONOS_FILTER_NATIVE,
    SM64DX_MOONOS_FILTER_MOONOS,
    SM64DX_MOONOS_FILTER_DYNOS,
    SM64DX_MOONOS_FILTER_FAVORITES,
    SM64DX_MOONOS_FILTER_RECENT,
    SM64DX_MOONOS_FILTER_COUNT,
};

enum Sm64dxMoonosBrowserMode {
    SM64DX_MOONOS_BROWSER_SHOWCASE,
    SM64DX_MOONOS_BROWSER_DENSE,
    SM64DX_MOONOS_BROWSER_COUNT,
};

enum Sm64dxMoonosSortMode {
    SM64DX_MOONOS_SORT_CURATED,
    SM64DX_MOONOS_SORT_ALPHABETICAL,
    SM64DX_MOONOS_SORT_SOURCE,
    SM64DX_MOONOS_SORT_COUNT,
};

enum Sm64dxMoonosPreviewMode {
    SM64DX_MOONOS_PREVIEW_SELECTED,
    SM64DX_MOONOS_PREVIEW_ACTIVE,
    SM64DX_MOONOS_PREVIEW_COUNT,
};

static int sMoonosSelectedPackIndex = -1;
static int sMoonosActionPackIndex = -1;
static int sMoonosPageIndex = 0;
static unsigned int sMoonosFilter = SM64DX_MOONOS_FILTER_INSTALLED;
static unsigned int sMoonosBrowserMode = SM64DX_MOONOS_BROWSER_SHOWCASE;
static unsigned int sMoonosSortMode = SM64DX_MOONOS_SORT_CURATED;
static unsigned int sMoonosPreviewMode = SM64DX_MOONOS_PREVIEW_SELECTED;

static char *sMoonosFilterChoices[SM64DX_MOONOS_FILTER_COUNT] = {
    "Installed",
    "Native",
    "MoonOS",
    "DynOS",
    "Favorites",
    "Recent",
};

static char *sMoonosBrowserChoices[SM64DX_MOONOS_BROWSER_COUNT] = {
    "Showcase",
    "Dense",
};

static char *sMoonosSortChoices[SM64DX_MOONOS_SORT_COUNT] = {
    "Curated",
    "A-Z",
    "By Source",
};

static char *sMoonosPreviewChoices[SM64DX_MOONOS_PREVIEW_COUNT] = {
    "Selected Pack",
    "Active Pack",
};

static float djui_panel_moonos_measure_text_height(const struct DjuiFont *font, float fontScale, int lineCount, float padding) {
    if (font == NULL) {
        return 0.0f;
    }
    if (lineCount < 1) {
        lineCount = 1;
    }
    return ((font->charHeight + ((float) (lineCount - 1) * font->lineHeight)) * fontScale) + padding;
}

static int djui_panel_moonos_target_slot(void) {
    int slot = gDjuiInMainMenu ? (int) configHostSaveSlot : (int) gCurrSaveFileNum;
    if (slot < 1 || slot > NUM_SAVE_FILES) {
        slot = 1;
    }
    return slot;
}

static void djui_panel_moonos_trimmed_text(const char *source, char *buffer, size_t bufferSize, size_t maxChars) {
    size_t length = 0;

    if (buffer == NULL || bufferSize == 0) {
        return;
    }

    buffer[0] = '\0';
    if (source == NULL) {
        return;
    }

    length = strlen(source);
    if (length <= maxChars || maxChars + 4 >= bufferSize) {
        snprintf(buffer, bufferSize, "%s", source);
        return;
    }

    snprintf(buffer, bufferSize, "%.*s...", (int) maxChars, source);
}

static struct DjuiText *djui_panel_moonos_text(struct DjuiBase *parent, const char *message, const struct DjuiFont *font,
                                               float fontScale, int lineCount, u8 r, u8 g, u8 b) {
    struct DjuiText *text = djui_text_create(parent, message);
    float height = djui_panel_moonos_measure_text_height(font, fontScale, lineCount, 4.0f);
    djui_base_set_size_type(&text->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&text->base, 1.0f, height);
    djui_base_set_color(&text->base, r, g, b, 255);
    djui_text_set_drop_shadow(text, 48, 48, 48, 100);
    djui_text_set_font(text, (struct DjuiFont *) font);
    djui_text_set_font_scale(text, fontScale);
    return text;
}

static void djui_panel_moonos_style_panel(struct DjuiThreePanel *panel, float width, float height) {
    struct DjuiFlowLayout *body = (struct DjuiFlowLayout *) djui_three_panel_get_body(panel);
    djui_base_set_size_type(&panel->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&panel->base, width, height);
    djui_base_set_alignment(&panel->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_base_set_color(&panel->base, 16, 18, 30, 242);
    djui_base_set_border_color(&panel->base, 214, 176, 94, 255);
    djui_base_set_border_width(&panel->base, 6);
    djui_base_set_gradient(&panel->base, false);
    djui_flow_layout_set_margin(body, 10);
}

static void djui_panel_moonos_refresh(bool reopenMoonos) {
    bool wasMainMenu = gDjuiInMainMenu;
    bool wasPlayerMenu = gDjuiInPlayerMenu;

    djui_panel_shutdown();
    if (wasMainMenu) {
        gDjuiInMainMenu = true;
        djui_panel_main_create(NULL);
        if (wasPlayerMenu) {
            djui_panel_player_create(NULL);
        }
        if (reopenMoonos) {
            djui_panel_moonos_create(NULL);
        }
    } else {
        djui_panel_pause_create(NULL);
        if (wasPlayerMenu) {
            djui_panel_player_create(NULL);
        }
        if (reopenMoonos) {
            djui_panel_moonos_create(NULL);
        }
    }
}

static const char *djui_panel_moonos_source_name(const struct Sm64dxMoonosPack *pack) {
    if (pack == NULL) {
        return "Native";
    }
    if (pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS) {
        return "DynOS";
    }
    if (pack->hasLuaScript) {
        return "MoonOS";
    }
    return "Native";
}

static int djui_panel_moonos_preview_index(const struct Sm64dxMoonosPack *pack) {
    int index = CT_MARIO;

    if (pack == NULL) {
        return index;
    }

    index = pack->previewCharacterIndex;
    if (index < 0 || index >= CT_MAX) {
        index = pack->nativeCharacterIndex;
    }
    if (index < 0 || index >= CT_MAX) {
        index = CT_MARIO;
    }
    return index;
}

static bool djui_panel_moonos_pack_has_gameplay(const struct Sm64dxMoonosPack *pack) {
    return pack != NULL && (pack->hasMoveset || pack->hasAnimations || pack->hasAttacks || pack->hasVoices);
}

static bool djui_panel_moonos_matches_filter(const struct Sm64dxMoonosPack *pack) {
    if (pack == NULL) {
        return false;
    }

    switch (sMoonosFilter) {
        case SM64DX_MOONOS_FILTER_INSTALLED:
            return true;
        case SM64DX_MOONOS_FILTER_NATIVE:
            return pack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE && !pack->hasLuaScript;
        case SM64DX_MOONOS_FILTER_MOONOS:
            return pack->hasLuaScript;
        case SM64DX_MOONOS_FILTER_DYNOS:
            return pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS;
        case SM64DX_MOONOS_FILTER_FAVORITES:
            return pack->favorite;
        case SM64DX_MOONOS_FILTER_RECENT:
            return pack->recent;
    }

    return true;
}

static void djui_panel_moonos_append_badge(char *buffer, size_t bufferSize, const char *badge) {
    if (buffer == NULL || bufferSize == 0 || badge == NULL || badge[0] == '\0') {
        return;
    }

    if (buffer[0] != '\0') {
        strncat(buffer, " | ", bufferSize - strlen(buffer) - 1);
    }
    strncat(buffer, badge, bufferSize - strlen(buffer) - 1);
}

static void djui_panel_moonos_build_feature_line(const struct Sm64dxMoonosPack *pack, char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return;
    }

    buffer[0] = '\0';
    if (pack == NULL) {
        return;
    }

    if (pack->hasLuaScript) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Lua");
    }
    if (pack->hasDynosAssets) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "DynOS");
    }
    if (pack->hasVoices) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Voices");
    }
    if (pack->hasLifeIcon) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Life Icon");
    }
    if (pack->hasHealthMeter) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Life Meter");
    }
    if (pack->hasMoveset) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Moveset");
    }
    if (pack->hasAnimations) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Animations");
    }
    if (pack->hasAttacks) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Attacks");
    }

    if (buffer[0] == '\0') {
        snprintf(buffer, bufferSize, "Base character profile");
    }
}

static void djui_panel_moonos_build_status_line(int slot, int packIndex, const struct Sm64dxMoonosPack *pack, char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0 || pack == NULL) {
        return;
    }

    snprintf(buffer, bufferSize, "%s | %s | Base %s",
             djui_panel_moonos_source_name(pack),
             (pack->compatibility[0] != '\0') ? pack->compatibility : "Compatible",
             (pack->baseCharacter[0] != '\0') ? pack->baseCharacter : "Mario");

    if (sm64dx_is_active_moonos_pack(slot, packIndex)) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Active");
    }
    if (pack->favorite) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Favorite");
    }
    if (pack->recent) {
        djui_panel_moonos_append_badge(buffer, bufferSize, "Recent");
    }
}

static int djui_panel_moonos_source_sort_value(const struct Sm64dxMoonosPack *pack) {
    if (pack == NULL) {
        return 0;
    }
    if (pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS) {
        return 2;
    }
    if (pack->hasLuaScript) {
        return 1;
    }
    return 0;
}

static int djui_panel_moonos_build_visible_pack_indices(int *indices, int maxCount) {
    int count = 0;

    for (int i = 0; i < sm64dx_get_moonos_pack_count() && count < maxCount; i++) {
        const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(i);
        if (!djui_panel_moonos_matches_filter(pack)) {
            continue;
        }
        indices[count++] = i;
    }

    if (sMoonosSortMode == SM64DX_MOONOS_SORT_ALPHABETICAL || sMoonosSortMode == SM64DX_MOONOS_SORT_SOURCE) {
        for (int i = 0; i < count; i++) {
            for (int j = i + 1; j < count; j++) {
                const struct Sm64dxMoonosPack *a = sm64dx_get_moonos_pack(indices[i]);
                const struct Sm64dxMoonosPack *b = sm64dx_get_moonos_pack(indices[j]);
                bool swap = false;

                if (sMoonosSortMode == SM64DX_MOONOS_SORT_SOURCE) {
                    int aSource = djui_panel_moonos_source_sort_value(a);
                    int bSource = djui_panel_moonos_source_sort_value(b);
                    if (aSource != bSource) {
                        swap = aSource > bSource;
                    }
                }

                if (!swap && a != NULL && b != NULL && sys_strcasecmp(a->name, b->name) > 0) {
                    swap = true;
                }

                if (swap) {
                    int temp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = temp;
                }
            }
        }
    }

    return count;
}

static int djui_panel_moonos_pick_selected_pack(const int *indices, int count, int slot) {
    int activeIndex = sm64dx_get_selected_moonos_pack_index(slot);

    if (count <= 0) {
        return -1;
    }

    if (sMoonosSelectedPackIndex >= 0) {
        for (int i = 0; i < count; i++) {
            if (indices[i] == sMoonosSelectedPackIndex) {
                return sMoonosSelectedPackIndex;
            }
        }
    }

    if (sMoonosPreviewMode == SM64DX_MOONOS_PREVIEW_ACTIVE && activeIndex >= 0) {
        for (int i = 0; i < count; i++) {
            if (indices[i] == activeIndex) {
                return activeIndex;
            }
        }
    }

    if (activeIndex >= 0) {
        for (int i = 0; i < count; i++) {
            if (indices[i] == activeIndex) {
                return activeIndex;
            }
        }
    }

    return indices[0];
}


static void djui_panel_moonos_apply_selected(UNUSED struct DjuiBase *caller) {
    if (sMoonosSelectedPackIndex < 0) {
        return;
    }
    sm64dx_apply_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex);
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_favorite_selected(UNUSED struct DjuiBase *caller) {
    if (sMoonosSelectedPackIndex < 0) {
        return;
    }
    sm64dx_toggle_moonos_favorite(sMoonosSelectedPackIndex);
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_select_card(struct DjuiBase *caller) {
    sMoonosSelectedPackIndex = (int) caller->tag;
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_apply_card(struct DjuiBase *caller) {
    sMoonosSelectedPackIndex = (int) caller->tag;
    djui_panel_moonos_apply_selected(caller);
}

static void djui_panel_moonos_filter_changed(UNUSED struct DjuiBase *caller) {
    sMoonosPageIndex = 0;
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_settings_changed(UNUSED struct DjuiBase *caller) {
    sMoonosPageIndex = 0;
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_style_player_palette(struct DjuiBase *caller) {
    if (sMoonosSelectedPackIndex >= 0) {
        sm64dx_apply_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex);
    }
    configHostSaveSlot = djui_panel_moonos_target_slot();
    djui_panel_player_create(caller);
}

static void djui_panel_moonos_binding_apply(UNUSED struct DjuiBase *caller) {
    if (sMoonosSelectedPackIndex < 0) {
        return;
    }
    sm64dx_apply_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex);
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_binding_set_default(UNUSED struct DjuiBase *caller) {
    if (sMoonosSelectedPackIndex < 0) {
        return;
    }
    sm64dx_set_global_default_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex);
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_binding_reset_default(UNUSED struct DjuiBase *caller) {
    sm64dx_reset_save_to_global_default(djui_panel_moonos_target_slot());
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_open_details(struct DjuiBase *caller);
static void djui_panel_moonos_open_style(struct DjuiBase *caller);
static void djui_panel_moonos_open_gameplay(struct DjuiBase *caller);
static void djui_panel_moonos_open_binding(struct DjuiBase *caller);
static void djui_panel_moonos_open_settings(struct DjuiBase *caller);

static void djui_panel_moonos_add_browser_card(struct DjuiBase *parent, int slot, int packIndex) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(packIndex);
    char authorLine[96] = { 0 };
    char detailLine[160] = { 0 };
    char featureLine[160] = { 0 };
    char titleLine[96] = { 0 };
    char descriptionLine[96] = { 0 };
    float cardHeight = (sMoonosBrowserMode == SM64DX_MOONOS_BROWSER_DENSE) ? 92.0f : 118.0f;
    float titleScale = gDjuiFonts[2]->defaultFontScale * 0.56f;
    float detailScale = gDjuiFonts[0]->defaultFontScale * 0.46f;

    if (pack == NULL) {
        return;
    }

    snprintf(authorLine, sizeof(authorLine), "By %s", (pack->author[0] != '\0') ? pack->author : "Unknown");
    djui_panel_moonos_build_status_line(slot, packIndex, pack, detailLine, sizeof(detailLine));
    djui_panel_moonos_build_feature_line(pack, featureLine, sizeof(featureLine));
    djui_panel_moonos_trimmed_text(pack->name, titleLine, sizeof(titleLine), 34);
    djui_panel_moonos_trimmed_text(pack->description, descriptionLine, sizeof(descriptionLine),
                                   (sMoonosBrowserMode == SM64DX_MOONOS_BROWSER_DENSE) ? 56 : 84);

    struct DjuiRect *card = djui_rect_create(parent);
    djui_base_set_size_type(&card->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&card->base, 1.0f, cardHeight);
    djui_base_set_color(&card->base, 20, 24, 36, 210);
    djui_base_set_gradient(&card->base, false);
    djui_base_set_border_width(&card->base, 2);
    djui_base_set_border_color(&card->base,
                               sm64dx_is_active_moonos_pack(slot, packIndex) ? 238 : 112,
                               sm64dx_is_active_moonos_pack(slot, packIndex) ? 204 : 154,
                               sm64dx_is_active_moonos_pack(slot, packIndex) ? 122 : 236,
                               255);
    djui_base_set_padding(&card->base, 10, 10, 10, 10);

    struct DjuiImage *preview = djui_image_create(&card->base,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.texture,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.width,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.height,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.format,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.size);
    djui_base_set_size_type(&preview->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&preview->base, 42, 42);
    djui_base_set_alignment(&preview->base, DJUI_HALIGN_RIGHT, DJUI_VALIGN_TOP);

    struct DjuiFlowLayout *layout = djui_flow_layout_create(&card->base);
    djui_base_set_size_type(&layout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&layout->base, 1.0f, 1.0f);
    djui_base_set_color(&layout->base, 0, 0, 0, 0);
    djui_base_set_padding(&layout->base, 0, 48, 0, 0);
    djui_flow_layout_set_margin(layout, 4);
    djui_flow_layout_set_flow_direction(layout, DJUI_FLOW_DIR_DOWN);

    djui_panel_moonos_text(&layout->base, titleLine, gDjuiFonts[2], titleScale, 1, 255, 236, 172);
    djui_panel_moonos_text(&layout->base, authorLine, gDjuiFonts[0], detailScale, 1, 228, 228, 228);
    if (sMoonosBrowserMode == SM64DX_MOONOS_BROWSER_SHOWCASE) {
        djui_panel_moonos_text(&layout->base, descriptionLine, gDjuiFonts[0], detailScale, 2, 208, 208, 208);
    }
    djui_panel_moonos_text(&layout->base, detailLine, gDjuiFonts[0], detailScale, 2, 188, 216, 255);
    djui_panel_moonos_text(&layout->base, featureLine, gDjuiFonts[0], detailScale, 2, 180, 235, 185);

    struct DjuiRect *buttonRow = djui_rect_container_create(&layout->base, 30.0f);
    {
        struct DjuiButton *selectButton = djui_button_left_create(&buttonRow->base, "Select", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_select_card);
        selectButton->base.tag = packIndex;
        djui_base_set_size(&selectButton->base, 0.485f, 30.0f);

        struct DjuiButton *applyButton = djui_button_right_create(&buttonRow->base,
                                                                  sm64dx_is_active_moonos_pack(slot, packIndex) ? "Active" : "Apply",
                                                                  DJUI_BUTTON_STYLE_NORMAL,
                                                                  djui_panel_moonos_apply_card);
        applyButton->base.tag = packIndex;
        djui_base_set_size(&applyButton->base, 0.485f, 30.0f);
        djui_base_set_enabled(&applyButton->base, !sm64dx_is_active_moonos_pack(slot, packIndex));
    }
}

static void djui_panel_moonos_add_preview_panel(struct DjuiBase *parent, int slot, int packIndex) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(packIndex);
    char authorLine[96] = { 0 };
    char statusLine[192] = { 0 };
    char featureLine[192] = { 0 };
    char descriptionLine[160] = { 0 };
    char tagsLine[128] = { 0 };
    char targetLine[96] = { 0 };
    char pathLine[160] = { 0 };
    char defaultLine[128] = { 0 };
    const float titleScale = gDjuiFonts[2]->defaultFontScale * 0.66f;
    const float detailScale = gDjuiFonts[0]->defaultFontScale * 0.50f;

    struct DjuiRect *previewCard = djui_rect_create(parent);
    djui_base_set_size_type(&previewCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&previewCard->base, 0.44f, 1.0f);
    djui_base_set_color(&previewCard->base, 20, 24, 38, 214);
    djui_base_set_border_width(&previewCard->base, 2);
    djui_base_set_border_color(&previewCard->base, 104, 162, 232, 255);
    djui_base_set_padding(&previewCard->base, 14, 14, 14, 14);

    if (pack == NULL) {
        djui_panel_moonos_text(&previewCard->base, "No pack selected.", gDjuiFonts[0], detailScale, 1, 255, 200, 200);
        return;
    }

    snprintf(authorLine, sizeof(authorLine), "By %s", (pack->author[0] != '\0') ? pack->author : "Unknown");
    djui_panel_moonos_build_status_line(slot, packIndex, pack, statusLine, sizeof(statusLine));
    djui_panel_moonos_build_feature_line(pack, featureLine, sizeof(featureLine));
    djui_panel_moonos_trimmed_text(pack->description, descriptionLine, sizeof(descriptionLine), 126);
    snprintf(tagsLine, sizeof(tagsLine), "Tags: %s", (pack->tags[0] != '\0') ? pack->tags : "character, playable");
    snprintf(targetLine, sizeof(targetLine), "Target Save: File %c", 'A' + (slot - 1));
    snprintf(pathLine, sizeof(pathLine), "Path: %s",
             (pack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE && !pack->hasLuaScript && !pack->hasDynosAssets)
                 ? "Built-in character"
                 : pack->id);
    snprintf(defaultLine, sizeof(defaultLine), "Global Default: %s",
             sm64dx_has_global_default_moonos() ? sm64dx_get_global_default_pack_name() : "Mario");

    struct DjuiImage *preview = djui_image_create(&previewCard->base,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.texture,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.width,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.height,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.format,
                                                  gCharacters[djui_panel_moonos_preview_index(pack)].hudHeadTexture.size);
    djui_base_set_size_type(&preview->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&preview->base, 96, 96);
    djui_base_set_alignment(&preview->base, DJUI_HALIGN_RIGHT, DJUI_VALIGN_TOP);

    struct DjuiFlowLayout *layout = djui_flow_layout_create(&previewCard->base);
    djui_base_set_size_type(&layout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&layout->base, 1.0f, 1.0f);
    djui_base_set_color(&layout->base, 0, 0, 0, 0);
    djui_base_set_padding(&layout->base, 0, 104, 0, 0);
    djui_flow_layout_set_margin(layout, 6);
    djui_flow_layout_set_flow_direction(layout, DJUI_FLOW_DIR_DOWN);

    djui_panel_moonos_text(&layout->base, pack->name, gDjuiFonts[2], titleScale, 1, 255, 240, 170);
    djui_panel_moonos_text(&layout->base, authorLine, gDjuiFonts[0], detailScale, 1, 230, 230, 230);
    djui_panel_moonos_text(&layout->base, descriptionLine, gDjuiFonts[0], detailScale, 3, 212, 212, 212);
    djui_panel_moonos_text(&layout->base, statusLine, gDjuiFonts[0], detailScale, 2, 188, 216, 255);
    djui_panel_moonos_text(&layout->base, featureLine, gDjuiFonts[0], detailScale, 2, 180, 235, 185);
    djui_panel_moonos_text(&layout->base, tagsLine, gDjuiFonts[0], detailScale, 2, 218, 202, 255);
    djui_panel_moonos_text(&layout->base, targetLine, gDjuiFonts[0], detailScale, 1, 220, 220, 220);
    djui_panel_moonos_text(&layout->base, pathLine, gDjuiFonts[0], detailScale, 2, 186, 186, 186);
    djui_panel_moonos_text(&layout->base, defaultLine, gDjuiFonts[0], detailScale, 2, 200, 235, 255);
}

static void djui_panel_moonos_open_details(struct DjuiBase *caller) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(sMoonosSelectedPackIndex);
    char sourceLine[128] = { 0 };
    char statusLine[192] = { 0 };
    char featureLine[192] = { 0 };
    char pathLine[160] = { 0 };
    char targetLine[96] = { 0 };

    if (pack == NULL) {
        return;
    }

    struct DjuiThreePanel *panel = djui_panel_menu_create("Pack Details", false);
    djui_panel_moonos_style_panel(panel, 720.0f, 0.78f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    int slot = djui_panel_moonos_target_slot();

    snprintf(sourceLine, sizeof(sourceLine), "Source: %s | Base %s",
             djui_panel_moonos_source_name(pack),
             (pack->baseCharacter[0] != '\0') ? pack->baseCharacter : "Mario");
    djui_panel_moonos_build_status_line(slot, sMoonosSelectedPackIndex, pack, statusLine, sizeof(statusLine));
    djui_panel_moonos_build_feature_line(pack, featureLine, sizeof(featureLine));
    snprintf(pathLine, sizeof(pathLine), "Install Path: %s",
             (pack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE && !pack->hasLuaScript && !pack->hasDynosAssets)
                 ? "Built-in character profile"
                 : pack->id);
    snprintf(targetLine, sizeof(targetLine), "Target Save: File %c", 'A' + (slot - 1));

    djui_panel_moonos_text(body, pack->name, gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.78f, 1, 255, 240, 170);
    djui_panel_moonos_text(body, pack->author[0] != '\0' ? pack->author : "Unknown", gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 228, 228, 228);
    djui_panel_moonos_text(body, pack->description, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 4, 212, 212, 212);
    djui_panel_moonos_text(body, statusLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 2, 188, 216, 255);
    djui_panel_moonos_text(body, featureLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 2, 180, 235, 185);
    djui_panel_moonos_text(body, sourceLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 220, 220, 220);
    djui_panel_moonos_text(body, pathLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.48f, 2, 186, 186, 186);
    djui_panel_moonos_text(body, targetLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 220, 220, 220);

    struct DjuiRect *row = djui_rect_container_create(body, 36.0f);
    {
        struct DjuiButton *applyButton = djui_button_left_create(&row->base, "Apply", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_apply_selected);
        djui_base_set_enabled(&applyButton->base, !sm64dx_is_active_moonos_pack(slot, sMoonosSelectedPackIndex));
        djui_button_right_create(&row->base, pack->favorite ? "Unfavorite" : "Favorite", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_favorite_selected);
    }

    djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_moonos_open_style(struct DjuiBase *caller) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(sMoonosSelectedPackIndex);
    char paletteLine[128] = { 0 };
    char hudLine[128] = { 0 };
    char presentationLine[160] = { 0 };

    if (pack == NULL) {
        return;
    }

    struct DjuiThreePanel *panel = djui_panel_menu_create("Style", false);
    djui_panel_moonos_style_panel(panel, 700.0f, 0.72f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);

    snprintf(paletteLine, sizeof(paletteLine), "Palette Preset: %s",
             (pack->paletteName[0] != '\0') ? pack->paletteName : "Uses the current save palette");
    snprintf(hudLine, sizeof(hudLine), "Life Icon: %s | Life Meter: %s",
             pack->hasLifeIcon ? "Custom" : "Default",
             pack->hasHealthMeter ? "Custom" : "Default");
    snprintf(presentationLine, sizeof(presentationLine),
             "This screen owns visual presentation for %s. Apply the pack, then open the player palette editor to fine-tune colors for this save.",
             pack->name);

    djui_panel_moonos_text(body, pack->name, gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.74f, 1, 255, 240, 170);
    djui_panel_moonos_text(body, paletteLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 2, 220, 220, 220);
    djui_panel_moonos_text(body, hudLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 2, 188, 216, 255);
    djui_panel_moonos_text(body, presentationLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 4, 210, 210, 210);

    struct DjuiRect *row1 = djui_rect_container_create(body, 36.0f);
    {
        struct DjuiButton *applyButton = djui_button_left_create(&row1->base, "Apply", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_apply_selected);
        djui_base_set_enabled(&applyButton->base, !sm64dx_is_active_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex));
        djui_button_right_create(&row1->base, "Apply + Edit Palette", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_style_player_palette);
    }

    struct DjuiRect *row2 = djui_rect_container_create(body, 36.0f);
    {
        djui_button_left_create(&row2->base, "Save Binding", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_binding);
        djui_button_right_create(&row2->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_moonos_open_gameplay(struct DjuiBase *caller) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(sMoonosSelectedPackIndex);
    char movesLine[96] = { 0 };
    char animLine[96] = { 0 };
    char attackLine[96] = { 0 };
    char voiceLine[96] = { 0 };
    char warningLine[192] = { 0 };

    if (pack == NULL) {
        return;
    }

    struct DjuiThreePanel *panel = djui_panel_menu_create("Gameplay", false);
    djui_panel_moonos_style_panel(panel, 700.0f, 0.72f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);

    snprintf(movesLine, sizeof(movesLine), "Moveset: %s", pack->hasMoveset ? "Custom" : "Default");
    snprintf(animLine, sizeof(animLine), "Animations: %s", pack->hasAnimations ? "Custom" : "Default");
    snprintf(attackLine, sizeof(attackLine), "Attacks: %s", pack->hasAttacks ? "Custom" : "Default");
    snprintf(voiceLine, sizeof(voiceLine), "Voices: %s", pack->hasVoices ? "Custom" : "Default");

    if (djui_panel_moonos_pack_has_gameplay(pack)) {
        snprintf(warningLine, sizeof(warningLine),
                 "%s changes more than appearance. Visual changes apply immediately; gameplay-heavy differences may feel best after re-entering a level.",
                 pack->name);
    } else {
        snprintf(warningLine, sizeof(warningLine),
                 "%s is presentation-first. It keeps Mario's default gameplay profile and only changes identity, visuals, or runtime hooks that the pack actually provides.",
                 pack->name);
    }

    djui_panel_moonos_text(body, pack->name, gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.74f, 1, 255, 240, 170);
    djui_panel_moonos_text(body, movesLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 180, 235, 185);
    djui_panel_moonos_text(body, animLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 180, 235, 185);
    djui_panel_moonos_text(body, attackLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 180, 235, 185);
    djui_panel_moonos_text(body, voiceLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 180, 235, 185);
    djui_panel_moonos_text(body, warningLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 4, 220, 220, 220);

    struct DjuiRect *row = djui_rect_container_create(body, 36.0f);
    {
        struct DjuiButton *applyButton = djui_button_left_create(&row->base, "Apply", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_apply_selected);
        djui_base_set_enabled(&applyButton->base, !sm64dx_is_active_moonos_pack(djui_panel_moonos_target_slot(), sMoonosSelectedPackIndex));
        djui_button_right_create(&row->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_moonos_open_binding(struct DjuiBase *caller) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(sMoonosSelectedPackIndex);
    int slot = djui_panel_moonos_target_slot();
    char currentLine[128] = { 0 };
    char defaultLine[128] = { 0 };
    char descriptionLine[160] = { 0 };

    if (pack == NULL) {
        return;
    }

    struct DjuiThreePanel *panel = djui_panel_menu_create("Save Binding", false);
    djui_panel_moonos_style_panel(panel, 700.0f, 0.72f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);

    snprintf(currentLine, sizeof(currentLine), "Current Save: File %c", 'A' + (slot - 1));
    snprintf(defaultLine, sizeof(defaultLine), "Global Default: %s",
             sm64dx_has_global_default_moonos() ? sm64dx_get_global_default_pack_name() : "Mario");
    snprintf(descriptionLine, sizeof(descriptionLine),
             "MoonOS keeps character identity save-bound by default. Use this screen to apply %s to the current file, make it the new default character profile, or reset the current save to the default profile.",
             pack->name);

    djui_panel_moonos_text(body, pack->name, gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.74f, 1, 255, 240, 170);
    djui_panel_moonos_text(body, currentLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 220, 220, 220);
    djui_panel_moonos_text(body, defaultLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.52f, 1, 200, 235, 255);
    djui_panel_moonos_text(body, descriptionLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 4, 212, 212, 212);

    djui_button_create(body, "Apply To Current Save", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_binding_apply);
    djui_button_create(body, "Set As Global Default", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_binding_set_default);
    djui_button_create(body, "Reset Save To Default", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_binding_reset_default);
    djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_moonos_open_settings(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create("MoonOS Settings", false);
    djui_panel_moonos_style_panel(panel, 660.0f, 0.68f);
    struct DjuiBase *body = djui_three_panel_get_body(panel);

    djui_panel_moonos_text(body,
                           "MoonOS browser settings control how installed characters are curated in the browser. Changes apply as soon as you select them and return you to the updated browser.",
                           gDjuiFonts[0],
                           gDjuiFonts[0]->defaultFontScale * 0.50f,
                           4,
                           220,
                           220,
                           220);
    djui_selectionbox_create(body, "Browser Layout", sMoonosBrowserChoices, SM64DX_MOONOS_BROWSER_COUNT, &sMoonosBrowserMode, djui_panel_moonos_settings_changed);
    djui_selectionbox_create(body, "Sort Mode", sMoonosSortChoices, SM64DX_MOONOS_SORT_COUNT, &sMoonosSortMode, djui_panel_moonos_settings_changed);
    djui_selectionbox_create(body, "Preview Focus", sMoonosPreviewChoices, SM64DX_MOONOS_PREVIEW_COUNT, &sMoonosPreviewMode, djui_panel_moonos_settings_changed);
    djui_panel_moonos_text(body,
                           "Template folders beginning with '_' remain hidden from the runtime loaders. Keep those for authoring or scratch work, then copy them into a live pack folder when they are ready.",
                           gDjuiFonts[0],
                           gDjuiFonts[0]->defaultFontScale * 0.48f,
                           4,
                           186,
                           186,
                           186);
    djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    djui_panel_add(caller, panel, NULL);
}

static int djui_panel_moonos_get_page_size(void) {
    return 8;
}

static void djui_panel_moonos_prev_page(UNUSED struct DjuiBase *caller) {
    if (sMoonosPageIndex > 0) {
        sMoonosPageIndex--;
        djui_panel_moonos_refresh(true);
    }
}

static void djui_panel_moonos_next_page(UNUSED struct DjuiBase *caller) {
    sMoonosPageIndex++;
    djui_panel_moonos_refresh(true);
}

static void djui_panel_moonos_style_browser_panel(struct DjuiThreePanel *panel) {
    struct DjuiFlowLayout *body = (struct DjuiFlowLayout *) djui_three_panel_get_body(panel);
    struct DjuiBase *header = djui_three_panel_get_header(panel);

    djui_three_panel_set_min_header_size(panel, 0.0f);
    djui_three_panel_set_min_footer_size(panel, 0.0f);
    djui_three_panel_set_body_size_type(panel, DJUI_SVT_RELATIVE);
    djui_three_panel_set_body_size(panel, 1.0f);

    djui_base_set_size_type(&panel->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&panel->base, 0.965f, 0.94f);
    djui_base_set_alignment(&panel->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_base_set_color(&panel->base, 8, 22, 52, 252);
    djui_base_set_border_color(&panel->base, 48, 168, 255, 255);
    djui_base_set_border_width(&panel->base, 4);
    djui_base_set_padding(&panel->base, 16, 18, 16, 18);
    djui_base_set_gradient(&panel->base, false);
    djui_flow_layout_set_margin(body, 10);

    if (header != NULL) {
        djui_base_set_visible(header, false);
        djui_base_set_size_type(header, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(header, 1.0f, 0.0f);
    }
}

static void djui_panel_moonos_tile_style(struct DjuiBase *base) {
    int slot = djui_panel_moonos_target_slot();
    int packIndex = (int) base->tag;
    bool selected = (packIndex == sMoonosSelectedPackIndex);
    bool active = sm64dx_is_active_moonos_pack(slot, packIndex);
    bool hovered = (gDjuiHovered == base || gInteractableFocus == base);
    bool pressed = (gDjuiCursorDownOn == base);

    u8 fillR = 18;
    u8 fillG = 40;
    u8 fillB = 88;
    u8 borderR = active ? 251 : 76;
    u8 borderG = active ? 214 : 188;
    u8 borderB = active ? 102 : 255;

    if (selected) {
        fillR = 30;
        fillG = 64;
        fillB = 124;
        borderR = 255;
        borderG = 246;
        borderB = 140;
    }
    if (hovered) {
        fillR = (u8) MIN(fillR + 18, 255);
        fillG = (u8) MIN(fillG + 18, 255);
        fillB = (u8) MIN(fillB + 18, 255);
    }
    if (pressed) {
        fillR = (u8) MAX(fillR - 10, 0);
        fillG = (u8) MAX(fillG - 10, 0);
        fillB = (u8) MAX(fillB - 10, 0);
    }

    djui_base_set_color(base, fillR, fillG, fillB, 232);
    djui_base_set_border_color(base, borderR, borderG, borderB, 255);
}

static void djui_panel_moonos_add_grid_tile(struct DjuiBase *parent, int slot, int packIndex,
                                            float relX, float absY, float relW, float absH) {
    const struct Sm64dxMoonosPack *pack = sm64dx_get_moonos_pack(packIndex);
    char titleLine[80] = { 0 };
    char sourceLine[80] = { 0 };
    char badgeLine[96] = { 0 };
    int previewIndex = 0;

    if (pack == NULL) {
        return;
    }

    djui_panel_moonos_trimmed_text(pack->name, titleLine, sizeof(titleLine), 20);
    snprintf(sourceLine, sizeof(sourceLine), "%s", djui_panel_moonos_source_name(pack));
    djui_panel_moonos_build_feature_line(pack, badgeLine, sizeof(badgeLine));
    {
        char badgeTrimmed[96] = { 0 };
        djui_panel_moonos_trimmed_text(badgeLine, badgeTrimmed, sizeof(badgeTrimmed), 20);
        snprintf(badgeLine, sizeof(badgeLine), "%s", badgeTrimmed);
    }

    struct DjuiRect *tile = djui_rect_create(parent);
    tile->base.tag = packIndex;
    djui_base_set_location_type(&tile->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_location(&tile->base, relX, absY);
    djui_base_set_size_type(&tile->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&tile->base, relW, absH);
    djui_base_set_border_width(&tile->base, 2);
    djui_base_set_padding(&tile->base, 8, 8, 8, 8);
    djui_interactable_create(&tile->base, djui_panel_moonos_tile_style);
    djui_interactable_hook_click(&tile->base, djui_panel_moonos_select_card);
    djui_panel_moonos_tile_style(&tile->base);

    previewIndex = djui_panel_moonos_preview_index(pack);
    struct DjuiImage *preview = djui_image_create(&tile->base,
                                                  gCharacters[previewIndex].hudHeadTexture.texture,
                                                  gCharacters[previewIndex].hudHeadTexture.width,
                                                  gCharacters[previewIndex].hudHeadTexture.height,
                                                  gCharacters[previewIndex].hudHeadTexture.format,
                                                  gCharacters[previewIndex].hudHeadTexture.size);
    djui_base_set_size_type(&preview->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&preview->base, 62, 62);
    djui_base_set_alignment(&preview->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_TOP);
    djui_base_set_location_type(&preview->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_location(&preview->base, 0.0f, 8.0f);

    struct DjuiFlowLayout *layout = djui_flow_layout_create(&tile->base);
    djui_base_set_size_type(&layout->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&layout->base, 1.0f, 1.0f);
    djui_base_set_padding(&layout->base, 76, 0, 0, 0);
    djui_base_set_color(&layout->base, 0, 0, 0, 0);
    djui_flow_layout_set_margin(layout, 2);

    struct DjuiText *title = djui_text_create(&layout->base, titleLine);
    djui_base_set_size_type(&title->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&title->base, 1.0f, 20);
    djui_base_set_color(&title->base, 255, 255, 255, 255);
    djui_text_set_alignment(title, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_text_set_font(title, gDjuiFonts[2]);
    djui_text_set_font_scale(title, gDjuiFonts[2]->defaultFontScale * 0.46f);
    djui_text_set_drop_shadow(title, 40, 40, 40, 120);

    struct DjuiText *source = djui_text_create(&layout->base, sourceLine);
    djui_base_set_size_type(&source->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&source->base, 1.0f, 18);
    djui_base_set_color(&source->base, 174, 226, 255, 255);
    djui_text_set_alignment(source, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    djui_text_set_font_scale(source, gDjuiFonts[0]->defaultFontScale * 0.42f);
    djui_text_set_drop_shadow(source, 40, 40, 40, 120);

    struct DjuiText *badges = djui_text_create(&layout->base, badgeLine);
    djui_base_set_size_type(&badges->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&badges->base, 1.0f, 34);
    djui_base_set_color(&badges->base, 196, 235, 186, 255);
    djui_text_set_alignment(badges, DJUI_HALIGN_CENTER, DJUI_VALIGN_TOP);
    djui_text_set_font_scale(badges, gDjuiFonts[0]->defaultFontScale * 0.34f);
    djui_text_set_drop_shadow(badges, 40, 40, 40, 120);

    if (sm64dx_is_active_moonos_pack(slot, packIndex) || pack->favorite) {
        const char *state = sm64dx_is_active_moonos_pack(slot, packIndex)
            ? (pack->favorite ? "ACTIVE  FAVORITE" : "ACTIVE")
            : "FAVORITE";
        struct DjuiText *tag = djui_text_create(&tile->base, state);
        djui_base_set_size_type(&tag->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&tag->base, 1.0f, 18);
        djui_base_set_location_type(&tag->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_location(&tag->base, 0.0f, absH - 24.0f);
        djui_base_set_color(&tag->base, 255, 244, 156, 255);
        djui_text_set_alignment(tag, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_font_scale(tag, gDjuiFonts[0]->defaultFontScale * 0.32f);
        djui_text_set_drop_shadow(tag, 40, 40, 40, 120);
    }
}

void djui_panel_moonos_create(struct DjuiBase *caller) {
    struct DjuiThreePanel *panel = djui_panel_menu_create("MoonOS", false);
    struct DjuiBase *body = djui_three_panel_get_body(panel);
    struct DjuiRect *content = NULL;
    struct DjuiRect *heroCard = NULL;
    struct DjuiRect *gridCard = NULL;
    int slot = djui_panel_moonos_target_slot();
    int visibleIndices[SM64DX_MAX_MOONOS_PACKS] = { 0 };
    int visibleCount = 0;
    int selectedIndex = -1;
    int pageSize = 0;
    int pageCount = 1;
    int pageStart = 0;
    int pageEnd = 0;
    char subtitle[224] = { 0 };
    char activeLine[128] = { 0 };
    char defaultLine[128] = { 0 };
    char pageBuffer[64] = { 0 };

    djui_panel_moonos_style_browser_panel(panel);

    snprintf(subtitle, sizeof(subtitle),
             "MoonOS is the character locker for File %c. Browse native, MoonOS, and DynOS packs as one catalog, then apply, favorite, or open details for the selected character.",
             'A' + (slot - 1));
    snprintf(activeLine, sizeof(activeLine), "Active Pack: %s", sm64dx_get_save_pack_name(slot));
    snprintf(defaultLine, sizeof(defaultLine), "Global Default: %s",
             sm64dx_has_global_default_moonos() ? sm64dx_get_global_default_pack_name() : "Mario");

    visibleCount = djui_panel_moonos_build_visible_pack_indices(visibleIndices, SM64DX_MAX_MOONOS_PACKS);
    selectedIndex = djui_panel_moonos_pick_selected_pack(visibleIndices, visibleCount, slot);
    sMoonosSelectedPackIndex = selectedIndex;

    pageSize = djui_panel_moonos_get_page_size();
    if (visibleCount <= 0) {
        sMoonosPageIndex = 0;
    }
    pageCount = (visibleCount <= 0) ? 1 : ((visibleCount + pageSize - 1) / pageSize);
    if (sMoonosPageIndex >= pageCount) {
        sMoonosPageIndex = pageCount - 1;
    }
    if (sMoonosPageIndex < 0) {
        sMoonosPageIndex = 0;
    }
    pageStart = sMoonosPageIndex * pageSize;
    pageEnd = MIN(visibleCount, pageStart + pageSize);
    snprintf(pageBuffer, sizeof(pageBuffer), "Page %d / %d", sMoonosPageIndex + 1, pageCount);

    {
        struct DjuiText *title = djui_panel_moonos_text(body, "CHARACTER LOCKER", gDjuiFonts[2], gDjuiFonts[2]->defaultFontScale * 0.78f, 1, 255, 255, 255);
        struct DjuiText *brand = djui_panel_moonos_text(body, "MoonOS", gDjuiFonts[1], gDjuiFonts[1]->defaultFontScale * 0.80f, 1, 110, 212, 255);
        struct DjuiText *subtitleText = djui_panel_moonos_text(body, subtitle, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.46f, 3, 218, 228, 240);
        struct DjuiText *activeText = djui_panel_moonos_text(body, activeLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.50f, 1, 154, 226, 255);
        struct DjuiText *defaultText = djui_panel_moonos_text(body, defaultLine, gDjuiFonts[0], gDjuiFonts[0]->defaultFontScale * 0.44f, 1, 224, 214, 255);
        djui_text_set_alignment(title, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_alignment(brand, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_alignment(subtitleText, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_alignment(activeText, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_alignment(defaultText, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
    }

    djui_selectionbox_create(body, "Collection", sMoonosFilterChoices, SM64DX_MOONOS_FILTER_COUNT, &sMoonosFilter, djui_panel_moonos_filter_changed);
    djui_selectionbox_create(body, "Sort", sMoonosSortChoices, SM64DX_MOONOS_SORT_COUNT, &sMoonosSortMode, djui_panel_moonos_filter_changed);

    content = djui_rect_create(body);
    djui_base_set_size_type(&content->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&content->base, 1.0f, 420.0f);
    djui_base_set_color(&content->base, 0, 0, 0, 0);

    heroCard = djui_rect_create(&content->base);
    djui_base_set_size_type(&heroCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&heroCard->base, 0.36f, 1.0f);
    djui_base_set_color(&heroCard->base, 12, 33, 74, 238);
    djui_base_set_border_width(&heroCard->base, 2);
    djui_base_set_border_color(&heroCard->base, 91, 206, 255, 255);
    djui_base_set_padding(&heroCard->base, 14, 14, 14, 14);

    gridCard = djui_rect_create(&content->base);
    djui_base_set_alignment(&gridCard->base, DJUI_HALIGN_RIGHT, DJUI_VALIGN_TOP);
    djui_base_set_size_type(&gridCard->base, DJUI_SVT_RELATIVE, DJUI_SVT_RELATIVE);
    djui_base_set_size(&gridCard->base, 0.60f, 1.0f);
    djui_base_set_color(&gridCard->base, 8, 28, 67, 240);
    djui_base_set_border_width(&gridCard->base, 2);
    djui_base_set_border_color(&gridCard->base, 64, 178, 255, 255);
    djui_base_set_padding(&gridCard->base, 12, 12, 12, 12);

    if (selectedIndex >= 0) {
        const struct Sm64dxMoonosPack *selectedPack = sm64dx_get_moonos_pack(selectedIndex);
        char authorLine[96] = { 0 };
        char statusLine[192] = { 0 };
        char featureLine[192] = { 0 };
        char pathLine[160] = { 0 };
        int previewIndex = djui_panel_moonos_preview_index(selectedPack);

        snprintf(authorLine, sizeof(authorLine), "By %s", (selectedPack->author[0] != '\0') ? selectedPack->author : "Unknown");
        djui_panel_moonos_build_status_line(slot, selectedIndex, selectedPack, statusLine, sizeof(statusLine));
        djui_panel_moonos_build_feature_line(selectedPack, featureLine, sizeof(featureLine));
        snprintf(pathLine, sizeof(pathLine), "Source: %s | Path: %s",
                 djui_panel_moonos_source_name(selectedPack),
                 (selectedPack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE && !selectedPack->hasLuaScript && !selectedPack->hasDynosAssets)
                     ? "Built-in"
                     : selectedPack->id);

        struct DjuiRect *previewStage = djui_rect_create(&heroCard->base);
        djui_base_set_size_type(&previewStage->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&previewStage->base, 1.0f, 214.0f);
        djui_base_set_color(&previewStage->base, 20, 48, 104, 255);
        djui_base_set_border_width(&previewStage->base, 2);
        djui_base_set_border_color(&previewStage->base, 120, 220, 255, 255);

        {
            struct DjuiImage *preview = djui_image_create(&previewStage->base,
                                                          gCharacters[previewIndex].hudHeadTexture.texture,
                                                          gCharacters[previewIndex].hudHeadTexture.width,
                                                          gCharacters[previewIndex].hudHeadTexture.height,
                                                          gCharacters[previewIndex].hudHeadTexture.format,
                                                          gCharacters[previewIndex].hudHeadTexture.size);
            djui_base_set_size_type(&preview->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&preview->base, 170, 170);
            djui_base_set_alignment(&preview->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        }

        {
            struct DjuiText *name = djui_text_create(&heroCard->base, selectedPack->name);
            struct DjuiText *author = djui_text_create(&heroCard->base, authorLine);
            struct DjuiText *description = djui_text_create(&heroCard->base, selectedPack->description);
            struct DjuiText *status = djui_text_create(&heroCard->base, statusLine);
            struct DjuiText *features = djui_text_create(&heroCard->base, featureLine);
            struct DjuiText *path = djui_text_create(&heroCard->base, pathLine);

            djui_base_set_size_type(&name->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&name->base, 1.0f, 28);
            djui_base_set_location_type(&name->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&name->base, 0.0f, 226.0f);
            djui_base_set_color(&name->base, 255, 255, 255, 255);
            djui_text_set_alignment(name, DJUI_HALIGN_LEFT, DJUI_VALIGN_CENTER);
            djui_text_set_font(name, gDjuiFonts[2]);
            djui_text_set_font_scale(name, gDjuiFonts[2]->defaultFontScale * 0.62f);
            djui_text_set_drop_shadow(name, 48, 48, 48, 120);

            djui_base_set_size_type(&author->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&author->base, 1.0f, 20);
            djui_base_set_location_type(&author->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&author->base, 0.0f, 258.0f);
            djui_base_set_color(&author->base, 184, 234, 255, 255);
            djui_text_set_alignment(author, DJUI_HALIGN_LEFT, DJUI_VALIGN_CENTER);
            djui_text_set_font_scale(author, gDjuiFonts[0]->defaultFontScale * 0.44f);
            djui_text_set_drop_shadow(author, 48, 48, 48, 120);

            djui_base_set_size_type(&description->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&description->base, 1.0f, 68);
            djui_base_set_location_type(&description->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&description->base, 0.0f, 286.0f);
            djui_base_set_color(&description->base, 222, 228, 236, 255);
            djui_text_set_alignment(description, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
            djui_text_set_font_scale(description, gDjuiFonts[0]->defaultFontScale * 0.40f);
            djui_text_set_drop_shadow(description, 48, 48, 48, 120);

            djui_base_set_size_type(&status->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&status->base, 1.0f, 30);
            djui_base_set_location_type(&status->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&status->base, 0.0f, 352.0f);
            djui_base_set_color(&status->base, 255, 243, 163, 255);
            djui_text_set_alignment(status, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
            djui_text_set_font_scale(status, gDjuiFonts[0]->defaultFontScale * 0.38f);
            djui_text_set_drop_shadow(status, 48, 48, 48, 120);

            djui_base_set_size_type(&features->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&features->base, 1.0f, 26);
            djui_base_set_location_type(&features->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&features->base, 0.0f, 380.0f);
            djui_base_set_color(&features->base, 184, 240, 194, 255);
            djui_text_set_alignment(features, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
            djui_text_set_font_scale(features, gDjuiFonts[0]->defaultFontScale * 0.38f);
            djui_text_set_drop_shadow(features, 48, 48, 48, 120);

            djui_base_set_size_type(&path->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_size(&path->base, 1.0f, 18);
            djui_base_set_location_type(&path->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&path->base, 0.0f, 398.0f);
            djui_base_set_color(&path->base, 186, 192, 210, 255);
            djui_text_set_alignment(path, DJUI_HALIGN_LEFT, DJUI_VALIGN_BOTTOM);
            djui_text_set_font_scale(path, gDjuiFonts[0]->defaultFontScale * 0.34f);
            djui_text_set_drop_shadow(path, 48, 48, 48, 120);
        }
    } else {
        struct DjuiText *empty = djui_text_create(&heroCard->base,
            "No packs match the current collection filter. Add packs under /moonos/packs or switch filters to browse another source.");
        djui_base_set_size_type(&empty->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&empty->base, 1.0f, 96.0f);
        djui_base_set_color(&empty->base, 232, 220, 206, 255);
        djui_text_set_alignment(empty, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
        djui_text_set_font_scale(empty, gDjuiFonts[0]->defaultFontScale * 0.46f);
        djui_text_set_drop_shadow(empty, 48, 48, 48, 120);
    }

    {
        struct DjuiText *gridTitle = djui_text_create(&gridCard->base, "PACK GRID");
        struct DjuiText *pageText = djui_text_create(&gridCard->base, pageBuffer);
        djui_base_set_size_type(&gridTitle->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&gridTitle->base, 1.0f, 26);
        djui_base_set_color(&gridTitle->base, 255, 255, 255, 255);
        djui_text_set_alignment(gridTitle, DJUI_HALIGN_LEFT, DJUI_VALIGN_CENTER);
        djui_text_set_font(gridTitle, gDjuiFonts[2]);
        djui_text_set_font_scale(gridTitle, gDjuiFonts[2]->defaultFontScale * 0.56f);
        djui_text_set_drop_shadow(gridTitle, 48, 48, 48, 120);

        djui_base_set_size_type(&pageText->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&pageText->base, 1.0f, 20);
        djui_base_set_color(&pageText->base, 170, 224, 255, 255);
        djui_text_set_alignment(pageText, DJUI_HALIGN_RIGHT, DJUI_VALIGN_CENTER);
        djui_text_set_font_scale(pageText, gDjuiFonts[0]->defaultFontScale * 0.42f);
        djui_text_set_drop_shadow(pageText, 48, 48, 48, 120);
    }

    if (visibleCount <= 0) {
        struct DjuiText *empty = djui_text_create(&gridCard->base, "This collection is empty right now.");
        djui_base_set_size_type(&empty->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&empty->base, 1.0f, 24);
        djui_base_set_location_type(&empty->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_location(&empty->base, 0.0f, 52.0f);
        djui_base_set_color(&empty->base, 255, 212, 180, 255);
        djui_text_set_alignment(empty, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
        djui_text_set_font_scale(empty, gDjuiFonts[0]->defaultFontScale * 0.46f);
        djui_text_set_drop_shadow(empty, 48, 48, 48, 120);
    } else {
        for (int index = pageStart; index < pageEnd; index++) {
            int localIndex = index - pageStart;
            int row = localIndex / 4;
            int col = localIndex % 4;
            djui_panel_moonos_add_grid_tile(&gridCard->base, slot, visibleIndices[index],
                                            0.01f + (float) col * 0.245f,
                                            38.0f + (float) row * 144.0f,
                                            0.23f,
                                            132.0f);
        }
    }

    {
        struct DjuiRect *pageRow = djui_rect_create(&gridCard->base);
        djui_base_set_alignment(&pageRow->base, DJUI_HALIGN_LEFT, DJUI_VALIGN_BOTTOM);
        djui_base_set_size_type(&pageRow->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&pageRow->base, 1.0f, 38.0f);
        djui_base_set_color(&pageRow->base, 0, 0, 0, 0);

        struct DjuiButton *prevButton = djui_button_left_create(&pageRow->base, "<", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_prev_page);
        struct DjuiButton *nextButton = djui_button_right_create(&pageRow->base, ">", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_next_page);
        struct DjuiText *pageHint = djui_text_create(&pageRow->base, visibleCount > 0 ? "Select a tile to focus it" : "No packs");

        djui_base_set_size(&prevButton->base, 0.18f, 38.0f);
        djui_base_set_size(&nextButton->base, 0.18f, 38.0f);
        djui_base_set_enabled(&prevButton->base, visibleCount > 0 && sMoonosPageIndex > 0);
        djui_base_set_enabled(&nextButton->base, visibleCount > 0 && (sMoonosPageIndex + 1) < pageCount);

        djui_base_set_size_type(&pageHint->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&pageHint->base, 1.0f, 38.0f);
        djui_base_set_color(&pageHint->base, 200, 222, 245, 255);
        djui_text_set_alignment(pageHint, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        djui_text_set_font_scale(pageHint, gDjuiFonts[0]->defaultFontScale * 0.38f);
        djui_text_set_drop_shadow(pageHint, 48, 48, 48, 120);
    }

    {
        const struct Sm64dxMoonosPack *selectedPack = sm64dx_get_moonos_pack(selectedIndex);
        bool hasSelection = selectedPack != NULL;
        bool activeSelected = hasSelection && sm64dx_is_active_moonos_pack(slot, selectedIndex);

        struct DjuiRect *row1 = djui_rect_container_create(body, 36.0f);
        struct DjuiButton *applyButton = djui_button_left_create(&row1->base, hasSelection && activeSelected ? "Active" : "Apply", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_apply_selected);
        struct DjuiButton *favoriteButton = djui_button_right_create(&row1->base,
                                                                     hasSelection && selectedPack->favorite ? "Unfavorite" : "Favorite",
                                                                     DJUI_BUTTON_STYLE_NORMAL,
                                                                     djui_panel_moonos_favorite_selected);
        djui_base_set_enabled(&applyButton->base, hasSelection && !activeSelected);
        djui_base_set_enabled(&favoriteButton->base, hasSelection);

        struct DjuiRect *row2 = djui_rect_container_create(body, 36.0f);
        struct DjuiButton *detailsButton = djui_button_left_create(&row2->base, "Details", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_details);
        struct DjuiButton *styleButton = djui_button_right_create(&row2->base, "Style", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_style);
        djui_base_set_enabled(&detailsButton->base, hasSelection);
        djui_base_set_enabled(&styleButton->base, hasSelection);

        struct DjuiRect *row3 = djui_rect_container_create(body, 36.0f);
        const char *gameplayLabel = (hasSelection && !djui_panel_moonos_pack_has_gameplay(selectedPack)) ? "Gameplay Info" : "Gameplay";
        struct DjuiButton *gameplayButton = djui_button_left_create(&row3->base, (char *) gameplayLabel, DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_gameplay);
        struct DjuiButton *bindingButton = djui_button_right_create(&row3->base, "Save Binding", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_binding);
        djui_base_set_enabled(&gameplayButton->base, hasSelection);
        djui_base_set_enabled(&bindingButton->base, hasSelection);
    }

    {
        struct DjuiRect *row4 = djui_rect_container_create(body, 36.0f);
        djui_button_left_create(&row4->base, "MoonOS Settings", DJUI_BUTTON_STYLE_NORMAL, djui_panel_moonos_open_settings);
        djui_button_right_create(&row4->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
