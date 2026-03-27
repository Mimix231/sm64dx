#pragma once

#include <stdbool.h>

#include "types.h"

#define STAGE_MUSIC 0

struct MainMenuSounds {
    char* name;
    int sound;
};

struct GlobalTextures {
    struct TextureInfo apostrophe;
    struct TextureInfo arrow_down;
    struct TextureInfo arrow_up;
    struct TextureInfo camera;
    struct TextureInfo coin;
    struct TextureInfo double_quote;
    struct TextureInfo lakitu;
    struct TextureInfo luigi_head;
    struct TextureInfo mario_head;
    struct TextureInfo no_camera;
    struct TextureInfo star;
    struct TextureInfo toad_head;
    struct TextureInfo waluigi_head;
    struct TextureInfo wario_head;
};

extern struct MainMenuSounds gMainMenuSounds[];
extern struct GlobalTextures gGlobalTextures;
extern bool gPanelLanguageOnStartup;

void mxui_assets_init(void);
