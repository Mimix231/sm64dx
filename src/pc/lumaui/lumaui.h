#ifndef LUMAUI_H
#define LUMAUI_H

#include <stdbool.h>
#include <PR/os_cont.h>

void lumaui_init(void);

bool lumaui_is_in_main_menu(void);
bool lumaui_is_in_player_menu(void);
bool lumaui_is_disabled(void);
bool lumaui_is_chat_box_focused(void);
bool lumaui_is_console_focused(void);
bool lumaui_uses_interactable_pad(void);
OSContPad *lumaui_get_interactable_pad(void);

void lumaui_render(void);
void lumaui_reset_hud_params(void);
void lumaui_gfx_displaylist_begin(void);
void lumaui_gfx_displaylist_end(void);

bool lumaui_pause_menu_is_created(void);
void lumaui_pause_menu_create(void);
int lumaui_pause_menu_consume_result(void);

void lumaui_set_palette_toggle_visible(bool visible);

#endif
