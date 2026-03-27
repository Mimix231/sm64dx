#include "loading.h"

#ifdef LOADING_SCREEN_SUPPORTED

#include <assert.h>

#include "game/game_init.h"
#include "game/memory.h"
#include "pc_main.h"
#include "pc/utils/misc.h"
#include "pc/cliopts.h"
#include "pc/mxui/mxui_font.h"
#include "pc/mxui/mxui_hud.h"
#include "pc/mxui/mxui_language.h"
#include "pc/mxui/mxui_render.h"
#include "pc/mxui/mxui_runtime.h"
#include "rom_checker.h"

struct LoadingSegment gCurrLoadingSegment = { "", 0 };
static bool sLoadingVisible = false;

struct ThreadHandle gLoadingThread = { 0 };

void loading_screen_set_segment_text(const char* text) {
    snprintf(gCurrLoadingSegment.str, sizeof(gCurrLoadingSegment.str), "%s", text != NULL ? text : "");
}

void loading_screen_reset_progress_bar(void) {
    gCurrLoadingSegment.percentage = 0.0f;
}

static void loading_screen_draw(void) {
    const f32 screenW = mxui_hud_get_screen_width();
    const f32 screenH = mxui_hud_get_screen_height();
    const f32 shellW = MIN(screenW - 120.0f, 620.0f);
    const f32 shellH = 240.0f;
    const f32 shellX = (screenW - shellW) * 0.5f;
    const f32 shellY = (screenH - shellH) * 0.5f;
    const f32 barW = shellW - 120.0f;
    const f32 barH = 18.0f;
    const f32 barX = shellX + 60.0f;
    const f32 barY = shellY + shellH - 62.0f;
    const f32 clampedProgress = gCurrLoadingSegment.percentage < 0.0f ? 0.0f : (gCurrLoadingSegment.percentage > 1.0f ? 1.0f : gCurrLoadingSegment.percentage);

    mxui_runtime_reset_hud_params();

    mxui_hud_set_color(8, 10, 18, 180);
    mxui_hud_render_rect(0, 0, screenW, screenH);

    mxui_hud_set_color(30, 42, 66, 238);
    mxui_hud_render_rect(shellX, shellY, shellW, shellH);
    mxui_hud_set_color(255, 210, 96, 255);
    mxui_hud_render_rect(shellX, shellY, shellW, 2.0f);
    mxui_hud_render_rect(shellX, shellY + shellH - 2.0f, shellW, 2.0f);
    mxui_hud_render_rect(shellX, shellY, 2.0f, shellH);
    mxui_hud_render_rect(shellX + shellW - 2.0f, shellY, 2.0f, shellH);

    mxui_hud_set_font(FONT_MENU);
    mxui_hud_set_color(255, 242, 210, 255);
    mxui_hud_print_text("Super Mario 64 DX", shellX + shellW * 0.5f - mxui_hud_measure_text("Super Mario 64 DX") * 0.5f, shellY + 34.0f, 0.85f);

    mxui_hud_set_font(FONT_NORMAL);
    mxui_hud_set_color(224, 232, 244, 255);
    if (gCurrLoadingSegment.str[0] != '\0') {
        char status[320] = { 0 };
        if (clampedProgress > 0.0f) {
            snprintf(status, sizeof(status), "%s... %d%%", gCurrLoadingSegment.str, (s32)floorf(clampedProgress * 100.0f));
        } else {
            snprintf(status, sizeof(status), "%s...", gCurrLoadingSegment.str);
        }
        const f32 scale = 0.5f;
        const f32 statusWidth = mxui_hud_measure_text(status) * scale;
        mxui_hud_print_text(status, shellX + shellW * 0.5f - statusWidth * 0.5f, shellY + 108.0f, scale);
    }

    mxui_hud_set_color(18, 24, 38, 255);
    mxui_hud_render_rect(barX, barY, barW, barH);
    mxui_hud_set_color(255, 210, 96, 255);
    mxui_hud_render_rect(barX, barY, barW * clampedProgress, barH);

    mxui_hud_set_color(170, 190, 214, 255);
    mxui_hud_print_text("Preparing local assets and mods", shellX + 60.0f, shellY + 148.0f, 0.38f);
}

static void loading_screen_produce_frame_callback(void) {
    if (sLoadingVisible) {
        mxui_render_displaylist_begin();
        loading_screen_draw();
        mxui_render_displaylist_end();
    }
}

static void loading_screen_produce_one_frame(void) {
    produce_one_dummy_frame(loading_screen_produce_frame_callback, 0x00, 0x00, 0x00);
}

static void init_loading_screen(void) {
    sLoadingVisible = true;
}

void loading_screen_reset(void) {
    sLoadingVisible = false;
    alloc_display_list_reset();
    gDisplayListHead = NULL;
    rendering_init();
    configWindow.settings_changed = true;
}

void render_loading_screen(void) {
    if (!sLoadingVisible) { init_loading_screen(); }

    // loading screen loop
    while (!gGameInited) {
        WAPI.main_loop(loading_screen_produce_one_frame);
    }

    int err = join_thread(&gLoadingThread);
    assert(err == 0);
}

void render_rom_setup_screen(void) {
    if (!sLoadingVisible) { init_loading_screen(); }

    loading_screen_set_segment_text("No rom detected, drag & drop Super Mario 64 (U) [!].z64 on to this screen");

    while (!gRomIsValid) {
        WAPI.main_loop(loading_screen_produce_one_frame);
    }
}

#endif
