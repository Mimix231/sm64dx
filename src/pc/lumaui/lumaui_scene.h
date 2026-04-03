#ifndef LUMAUI_SCENE_H
#define LUMAUI_SCENE_H

enum LumaUISceneId {
    LUMAUI_SCENE_NONE,
    LUMAUI_SCENE_TITLE,
    LUMAUI_SCENE_SAVE_SELECT,
    LUMAUI_SCENE_OPTIONS,
    LUMAUI_SCENE_PAUSE,
    LUMAUI_SCENE_COUNT,
};

struct LumaUIState;

void lumaui_scene_update(struct LumaUIState *state);
void lumaui_scene_render(struct LumaUIState *state);

#endif
