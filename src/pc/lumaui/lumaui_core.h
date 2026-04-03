#ifndef LUMAUI_CORE_H
#define LUMAUI_CORE_H

#include <stdbool.h>

#include "lumaui_input.h"
#include "lumaui_scene.h"

#define LUMAUI_SCENE_STACK_CAPACITY 8

struct LumaUIModalState {
    bool active;
    char title[64];
    char body[256];
};

struct LumaUIState {
    bool initialized;
    bool paletteToggleVisible;
    enum LumaUISceneId sceneStack[LUMAUI_SCENE_STACK_CAPACITY];
    int sceneCount;
    int selectedIndex[LUMAUI_SCENE_COUNT];
    int pauseResult;
    int pendingStartSlot;
    bool pendingStartSound;
    struct LumaUIInputState input;
    struct LumaUIModalState modal;
};

struct LumaUIState *lumaui_core_get_state(void);

void lumaui_core_init(void);
void lumaui_core_update(void);
void lumaui_core_render(void);

bool lumaui_core_has_active_scene(void);
bool lumaui_core_is_in_main_menu(void);
bool lumaui_core_pause_menu_is_created(void);
void lumaui_core_pause_menu_create(void);
int lumaui_core_pause_menu_consume_result(void);

void lumaui_core_push_scene(enum LumaUISceneId scene);
void lumaui_core_pop_scene(void);
void lumaui_core_replace_scene(enum LumaUISceneId scene);
void lumaui_core_clear_scenes(void);
enum LumaUISceneId lumaui_core_get_active_scene(void);
bool lumaui_core_scene_is_active(enum LumaUISceneId scene);

void lumaui_core_open_modal(const char *title, const char *body);
void lumaui_core_close_modal(void);
void lumaui_core_request_pause_resume(void);
void lumaui_core_queue_start_save_slot(int slot, bool playSound);
void lumaui_core_set_palette_toggle_visible(bool visible);

#endif
