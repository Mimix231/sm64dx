#ifndef SM64DX_UI_H
#define SM64DX_UI_H

#include <stdbool.h>
#include <PR/os_cont.h>

#define SM64DX_UI_STAGE_MUSIC 0

struct Sm64dxMainMenuSound {
    const char *name;
    int sound;
};

bool sm64dx_ui_is_in_main_menu(void);
bool sm64dx_ui_is_in_player_menu(void);
bool sm64dx_ui_is_disabled(void);
bool sm64dx_ui_is_chat_box_focused(void);
bool sm64dx_ui_is_console_focused(void);
bool sm64dx_ui_uses_interactable_pad(void);
OSContPad *sm64dx_ui_get_interactable_pad(void);

void sm64dx_ui_render(void);
void sm64dx_ui_reset_hud_params(void);
void sm64dx_ui_gfx_displaylist_begin(void);
void sm64dx_ui_gfx_displaylist_end(void);

bool sm64dx_ui_pause_menu_is_created(void);
void sm64dx_ui_pause_menu_create(void);
int sm64dx_ui_pause_menu_consume_result(void);

void sm64dx_ui_set_palette_toggle_visible(bool visible);

const struct Sm64dxMainMenuSound *sm64dx_ui_get_main_menu_sounds(void);

#endif
