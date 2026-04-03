#include "lumaui_core.h"

#include <stdio.h>
#include <string.h>

#include "lumaui_render.h"
#include "lumaui_scene.h"

#include "game/sm64dx_ui.h"
#include "pc/djui/djui_sm64dx.h"
#include "pc/pc_main.h"

static struct LumaUIState sLumaUIState = { 0 };

static void lumaui_core_commit_pending_actions(void) {
    if (sLumaUIState.pendingStartSlot > 0) {
        int slot = sLumaUIState.pendingStartSlot;
        bool playSound = sLumaUIState.pendingStartSound;

        sLumaUIState.pendingStartSlot = 0;
        sLumaUIState.pendingStartSound = false;
        lumaui_core_clear_scenes();
        sm64dx_start_save_slot(slot, playSound);
    }
}

struct LumaUIState *lumaui_core_get_state(void) {
    return &sLumaUIState;
}

void lumaui_core_init(void) {
    if (sLumaUIState.initialized) {
        return;
    }

    memset(&sLumaUIState, 0, sizeof(sLumaUIState));
    sLumaUIState.initialized = true;
    sLumaUIState.sceneStack[0] = LUMAUI_SCENE_TITLE;
    sLumaUIState.sceneCount = 1;
}

void lumaui_core_update(void) {
    lumaui_core_init();
    lumaui_core_commit_pending_actions();

    if (!lumaui_core_has_active_scene()) {
        return;
    }

    lumaui_input_update(&sLumaUIState);
    lumaui_scene_update(&sLumaUIState);
}

void lumaui_core_render(void) {
    lumaui_core_init();
    if (!lumaui_core_has_active_scene()) {
        return;
    }

    lumaui_render_begin();
    lumaui_scene_render(&sLumaUIState);
    lumaui_render_modal(&sLumaUIState);
    if (sLumaUIState.input.cursorVisible) {
        lumaui_render_cursor((s16) sLumaUIState.input.cursorX, (s16) sLumaUIState.input.cursorY);
    }
}

bool lumaui_core_has_active_scene(void) {
    return sLumaUIState.sceneCount > 0;
}

bool lumaui_core_is_in_main_menu(void) {
    enum LumaUISceneId activeScene = lumaui_core_get_active_scene();
    return activeScene != LUMAUI_SCENE_NONE && activeScene != LUMAUI_SCENE_PAUSE;
}

bool lumaui_core_pause_menu_is_created(void) {
    return lumaui_core_scene_is_active(LUMAUI_SCENE_PAUSE);
}

void lumaui_core_pause_menu_create(void) {
    lumaui_core_init();
    if (lumaui_core_is_in_main_menu() || lumaui_core_scene_is_active(LUMAUI_SCENE_PAUSE)) {
        return;
    }
    lumaui_core_push_scene(LUMAUI_SCENE_PAUSE);
}

int lumaui_core_pause_menu_consume_result(void) {
    int result = sLumaUIState.pauseResult;
    sLumaUIState.pauseResult = 0;
    return result;
}

void lumaui_core_push_scene(enum LumaUISceneId scene) {
    if (scene == LUMAUI_SCENE_NONE) {
        return;
    }

    if (sLumaUIState.sceneCount >= LUMAUI_SCENE_STACK_CAPACITY) {
        return;
    }

    sLumaUIState.sceneStack[sLumaUIState.sceneCount++] = scene;
}

void lumaui_core_pop_scene(void) {
    if (sLumaUIState.sceneCount <= 0) {
        return;
    }

    sLumaUIState.sceneCount--;
    sLumaUIState.sceneStack[sLumaUIState.sceneCount] = LUMAUI_SCENE_NONE;
}

void lumaui_core_replace_scene(enum LumaUISceneId scene) {
    lumaui_core_clear_scenes();
    if (scene != LUMAUI_SCENE_NONE) {
        lumaui_core_push_scene(scene);
    }
}

void lumaui_core_clear_scenes(void) {
    memset(sLumaUIState.sceneStack, 0, sizeof(sLumaUIState.sceneStack));
    sLumaUIState.sceneCount = 0;
    sLumaUIState.modal.active = false;
    if (wm_api != NULL && wm_api->set_cursor_visible != NULL) {
        wm_api->set_cursor_visible(true);
    }
}

enum LumaUISceneId lumaui_core_get_active_scene(void) {
    if (sLumaUIState.sceneCount <= 0) {
        return LUMAUI_SCENE_NONE;
    }
    return sLumaUIState.sceneStack[sLumaUIState.sceneCount - 1];
}

bool lumaui_core_scene_is_active(enum LumaUISceneId scene) {
    for (int i = 0; i < sLumaUIState.sceneCount; i++) {
        if (sLumaUIState.sceneStack[i] == scene) {
            return true;
        }
    }
    return false;
}

void lumaui_core_open_modal(const char *title, const char *body) {
    snprintf(sLumaUIState.modal.title, sizeof(sLumaUIState.modal.title), "%s", title != NULL ? title : "");
    snprintf(sLumaUIState.modal.body, sizeof(sLumaUIState.modal.body), "%s", body != NULL ? body : "");
    sLumaUIState.modal.active = true;
}

void lumaui_core_close_modal(void) {
    sLumaUIState.modal.active = false;
    sLumaUIState.modal.title[0] = '\0';
    sLumaUIState.modal.body[0] = '\0';
}

void lumaui_core_request_pause_resume(void) {
    sLumaUIState.pauseResult = 1;
    if (lumaui_core_get_active_scene() == LUMAUI_SCENE_PAUSE) {
        lumaui_core_pop_scene();
    }
    lumaui_core_close_modal();
}

void lumaui_core_queue_start_save_slot(int slot, bool playSound) {
    sLumaUIState.pendingStartSlot = slot;
    sLumaUIState.pendingStartSound = playSound;
}

void lumaui_core_set_palette_toggle_visible(bool visible) {
    sLumaUIState.paletteToggleVisible = visible;
}
