#include "lumaui.h"

#include "lumaui_core.h"

#include "sm64.h"
#include "game/game_init.h"

static OSContPad sLumaUIInteractablePad = { 0 };

void lumaui_init(void) {
    lumaui_core_init();
}

bool lumaui_is_in_main_menu(void) {
    lumaui_core_init();
    return lumaui_core_is_in_main_menu();
}

bool lumaui_is_in_player_menu(void) {
    return false;
}

bool lumaui_is_disabled(void) {
    return false;
}

bool lumaui_is_chat_box_focused(void) {
    return false;
}

bool lumaui_is_console_focused(void) {
    return false;
}

bool lumaui_uses_interactable_pad(void) {
    return false;
}

OSContPad *lumaui_get_interactable_pad(void) {
    bzero(&sLumaUIInteractablePad, sizeof(sLumaUIInteractablePad));
    return &sLumaUIInteractablePad;
}

void lumaui_render(void) {
    lumaui_core_update();
    lumaui_core_render();
}

void lumaui_reset_hud_params(void) {
}

void lumaui_gfx_displaylist_begin(void) {
}

void lumaui_gfx_displaylist_end(void) {
}

bool lumaui_pause_menu_is_created(void) {
    return lumaui_core_pause_menu_is_created();
}

void lumaui_pause_menu_create(void) {
    lumaui_core_pause_menu_create();
}

int lumaui_pause_menu_consume_result(void) {
    return lumaui_core_pause_menu_consume_result();
}

void lumaui_set_palette_toggle_visible(bool visible) {
    lumaui_core_set_palette_toggle_visible(visible);
}
