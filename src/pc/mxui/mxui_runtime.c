#include "mxui_runtime.h"

#include "sm64.h"

#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "pc/lua/smlua_hooks.h"
#include "pc/pc_main.h"

#include "mxui.h"
#include "mxui_cursor.h"
#include "mxui_assets.h"
#include "mxui_font.h"
#include "mxui_hud.h"
#include "mxui_render.h"

static bool sMxuiRendered60fps = false;
static Gfx* sSavedDisplayListHead = NULL;

bool gMxuiDisabled = false;
extern f32 gFramePercentage;
extern void mxui_exports_init(void);

void mxui_runtime_init(void) {
    gMxuiDisabled = false;
    mxui_exports_init();
    mxui_assets_init();
    mxui_init();
}

void mxui_runtime_shutdown(void) {
    sSavedDisplayListHead = NULL;
    sMxuiRendered60fps = false;
    mxui_shutdown();
    gMxuiDisabled = false;
}

void mxui_runtime_open_main_flow(void) {
    gMxuiDisabled = false;
    mxui_open_main_flow();
}

void mxui_runtime_reset_hud_params(void) {
    mxui_hud_set_resolution(0);
    mxui_hud_set_font(FONT_NORMAL);
    mxui_hud_set_rotation(0, 0, 0);
    mxui_hud_reset_color();
    mxui_hud_reset_scissor();
    mxui_render_reset_texture_clipping();
}

void mxui_runtime_render(void) {
    if (gMxuiDisabled || !mxui_is_active()) {
        return;
    }

    sSavedDisplayListHead = gDisplayListHead;
    mxui_runtime_reset_hud_params();
    create_dl_ortho_matrix();
    mxui_render_displaylist_begin();
    smlua_call_event_hooks(HOOK_ON_HUD_RENDER, mxui_runtime_reset_hud_params);
    mxui_render();
    mxui_cursor_update();
    mxui_render_displaylist_end();
}

void patch_mxui_before(void) {
    sMxuiRendered60fps = false;
    sSavedDisplayListHead = NULL;
}

void patch_mxui_interpolated(UNUSED f32 delta) {
    bool rerenderUi = mxui_is_main_menu_active() || mxui_is_pause_active();

    if (gMxuiDisabled || !mxui_is_active()) {
        return;
    }

    if (rerenderUi) {
        if (gFramePercentage >= 0.5f && !sMxuiRendered60fps) {
            if (sSavedDisplayListHead == NULL) {
                return;
            }
            sMxuiRendered60fps = true;
            gDisplayListHead = sSavedDisplayListHead;
            mxui_runtime_render();
            gDPFullSync(gDisplayListHead++);
            gSPEndDisplayList(gDisplayListHead++);
        } else {
            mxui_cursor_interp();
        }
        return;
    }

    mxui_cursor_interp();
}
