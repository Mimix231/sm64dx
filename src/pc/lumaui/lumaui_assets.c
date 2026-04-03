#include "lumaui_assets.h"

#include "lumaui_scene.h"

const char *lumaui_assets_brand_name(void) {
    return "SM64DX";
}

const char *lumaui_assets_scene_name(int sceneId) {
    switch ((enum LumaUISceneId) sceneId) {
        case LUMAUI_SCENE_TITLE:       return "Title";
        case LUMAUI_SCENE_SAVE_SELECT: return "Select File";
        case LUMAUI_SCENE_OPTIONS:     return "Options";
        case LUMAUI_SCENE_PAUSE:       return "Pause";
        default:                       return "Scene";
    }
}

const char *lumaui_assets_scene_subtitle(int sceneId) {
    switch ((enum LumaUISceneId) sceneId) {
        case LUMAUI_SCENE_TITLE:
            return "Offline-first frontend for sm64dx.";
        case LUMAUI_SCENE_SAVE_SELECT:
            return "Choose a file and launch straight into the adventure.";
        case LUMAUI_SCENE_OPTIONS:
            return "Startup setup and title preferences.";
        case LUMAUI_SCENE_PAUSE:
            return "Gameplay paused. Resume routes fully through LumaUI.";
        default:
            return "Fullscreen LumaUI scene.";
    }
}
