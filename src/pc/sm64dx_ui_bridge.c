#include "game/sm64dx_ui.h"

#include "sm64.h"
#include "seq_ids.h"

#include "pc/lumaui/lumaui.h"

static const struct Sm64dxMainMenuSound sSm64dxMainMenuSounds[] = {
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
    { "Stage Music", SM64DX_UI_STAGE_MUSIC },
};

bool sm64dx_ui_is_in_main_menu(void) {
    return lumaui_is_in_main_menu();
}

bool sm64dx_ui_is_in_player_menu(void) {
    return lumaui_is_in_player_menu();
}

bool sm64dx_ui_is_disabled(void) {
    return lumaui_is_disabled();
}

bool sm64dx_ui_is_chat_box_focused(void) {
    return lumaui_is_chat_box_focused();
}

bool sm64dx_ui_is_console_focused(void) {
    return lumaui_is_console_focused();
}

bool sm64dx_ui_uses_interactable_pad(void) {
    return lumaui_uses_interactable_pad();
}

OSContPad *sm64dx_ui_get_interactable_pad(void) {
    return lumaui_get_interactable_pad();
}

void sm64dx_ui_render(void) {
    lumaui_render();
}

void sm64dx_ui_reset_hud_params(void) {
    lumaui_reset_hud_params();
}

void sm64dx_ui_gfx_displaylist_begin(void) {
    lumaui_gfx_displaylist_begin();
}

void sm64dx_ui_gfx_displaylist_end(void) {
    lumaui_gfx_displaylist_end();
}

bool sm64dx_ui_pause_menu_is_created(void) {
    return lumaui_pause_menu_is_created();
}

void sm64dx_ui_pause_menu_create(void) {
    lumaui_pause_menu_create();
}

int sm64dx_ui_pause_menu_consume_result(void) {
    return lumaui_pause_menu_consume_result();
}

void sm64dx_ui_set_palette_toggle_visible(bool visible) {
    lumaui_set_palette_toggle_visible(visible);
}

const struct Sm64dxMainMenuSound *sm64dx_ui_get_main_menu_sounds(void) {
    return sSm64dxMainMenuSounds;
}
