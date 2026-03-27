#include "sm64.h"
#include "game/game_init.h"
#include "mxui_internal.h"

#include "pc/configfile.h"
#include "pc/controller/controller_api.h"
#include "pc/controller/controller_keyboard.h"

bool mxui_input_accept_pressed(void) {
    return controller_key_pressed(SCANCODE_ENTER)
        || ((gPlayer1Controller->buttonPressed & (A_BUTTON | START_BUTTON)) != 0);
}

bool mxui_input_accept_down(void) {
    return controller_key_down(SCANCODE_ENTER)
        || ((gPlayer1Controller->buttonDown & (A_BUTTON | START_BUTTON)) != 0);
}

bool mxui_input_back_pressed(void) {
    return (gPlayer1Controller->buttonPressed & B_BUTTON) != 0;
}

bool mxui_input_menu_toggle_pressed(void) {
    return controller_bind_pressed(configKeyGameMenu);
}

bool mxui_input_menu_toggle_down(void) {
    return controller_bind_down(configKeyGameMenu);
}

bool mxui_input_prev_page_pressed(void) {
    return controller_bind_pressed(configKeyPrevPage)
        || ((gPlayer1Controller->buttonPressed & L_TRIG) != 0);
}

bool mxui_input_next_page_pressed(void) {
    return controller_bind_pressed(configKeyNextPage)
        || ((gPlayer1Controller->buttonPressed & R_TRIG) != 0);
}
