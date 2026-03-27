#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "sm64.h"

#include "engine/math_util.h"
#include "game/camera.h"
#include "game/first_person_cam.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/rendering_graph_node.h"
#include "gfx_dimensions.h"
#include "pc/controller/controller_mouse.h"
#include "pc/gfx/gfx.h"
#include "pc/utils/misc.h"

#include "mxui.h"
#include "mxui_font.h"
#include "mxui_hud.h"
#include "mxui_render.h"
#include "mxui_unicode.h"

void gfx_get_dimensions(uint32_t *width, uint32_t *height);

#define MAX_INTERP_HUD 512

static const Vtx sMxuiHudSimpleRectVertices[] = {
    {{{ 0, -1, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 1, -1, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 1,  0, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 0,  0, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
};

static const Gfx dl_mxui_hud_simple_rect[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING),
    gsDPSetCombineMode(G_CC_FADE, G_CC_FADE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPVertexNonGlobal(sMxuiHudSimpleRectVertices, 4, 0),
    gsSP2Triangles(0, 1, 2, 0x0, 0, 2, 3, 0x0),
    gsSPEndDisplayList(),
};

struct InterpMxuiHud {
    Gfx* headPos;
    f32 z;
    f32 prevX;
    f32 prevY;
    f32 x;
    f32 y;
    f32 prevScaleW;
    f32 prevScaleH;
    f32 scaleW;
    f32 scaleH;
    f32 width;
    f32 height;
    u8 resolution;
    struct MxuiHudRotation rotation;
};

static u8 sResolution = MXUI_HUD_RESOLUTION_DJUI;
static u8 sFilter = MXUI_HUD_FILTER_NEAREST;
static s8 sFont = FONT_NORMAL;
static struct MxuiHudRotation sRotation = { 0, 0, 0, 0, 0, 0 };
static struct MxuiHudColor sColor = { 255, 255, 255, 255 };
static struct MxuiHudColor sRefColor = { 255, 255, 255, 255 };
static bool sColorAltered = false;
static f32 sMxuiHudZ = 0.0f;
static struct InterpMxuiHud sInterpMxuiHuds[MAX_INTERP_HUD] = { 0 };
static u16 sInterpMxuiHudCount = 0;

static void mxui_hud_position_translate(f32* x, f32* y) {
    if (sResolution == MXUI_HUD_RESOLUTION_DJUI) {
        mxui_render_position_translate(x, y);
    } else {
        *x = GFX_DIMENSIONS_FROM_LEFT_EDGE(0) + *x;
        *y = SCREEN_HEIGHT - *y;
    }
}

static void mxui_hud_size_translate(f32* size) {
    if (sResolution == MXUI_HUD_RESOLUTION_DJUI) {
        mxui_render_size_translate(size);
    }
}

static void mxui_hud_apply_rotation(f32 width, f32 height) {
    if (sRotation.rotation == 0.0f) {
        return;
    }

    f32 pivotTranslationX = width * sRotation.pivotX;
    f32 pivotTranslationY = height * sRotation.pivotY;
    create_dl_translation_matrix(MXUI_MTX_NOPUSH, +pivotTranslationX, -pivotTranslationY, 0);
    create_dl_rotation_matrix(MXUI_MTX_NOPUSH, sRotation.rotation, 0, 0, 1);
    create_dl_translation_matrix(MXUI_MTX_NOPUSH, -pivotTranslationX, +pivotTranslationY, 0);
}

static void mxui_hud_apply_rotation_aspect(f32 width, f32 height, f32 aspect) {
    if (sRotation.rotation == 0.0f) {
        return;
    }

    f32 pivotTranslationX = width * aspect * sRotation.pivotX;
    f32 pivotTranslationY = height * sRotation.pivotY;
    create_dl_translation_matrix(MXUI_MTX_NOPUSH, +pivotTranslationX, -pivotTranslationY, 0);
    create_dl_rotation_matrix(MXUI_MTX_NOPUSH, sRotation.rotation, 0, 0, 1);
    create_dl_translation_matrix(MXUI_MTX_NOPUSH, -pivotTranslationX, +pivotTranslationY, 0);
}

static void mxui_hud_set_draw_color(void) {
    mxui_render_set_color(sColor.r, sColor.g, sColor.b, sColor.a);
}

static void mxui_hud_draw_text_raw(const char* message, f32 x, f32 y, f32 scale) {
    if (message == NULL) {
        return;
    }

    const struct MxuiFont* font = gMxuiFonts[sFont];
    enum MxuiFontType lastFont = mxui_font_get_current();

    f32 translatedX = x + (font->xOffset * scale);
    f32 translatedY = y + (font->yOffset * scale);
    f32 translatedFontSize = font->defaultFontScale * scale;

    mxui_font_set_current((enum MxuiFontType)sFont);
    mxui_hud_set_draw_color();

    if (font->textBeginDisplayList != NULL) {
        gSPDisplayList(gDisplayListHead++, font->textBeginDisplayList);
    }

    mxui_hud_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, sMxuiHudZ);

    mxui_hud_size_translate(&translatedFontSize);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, translatedFontSize, translatedFontSize, 1.0f);

    char* cursor = (char*)message;
    mxui_render_reset_texture_clipping();
    while (*cursor != '\0') {
        if (*cursor != '\n') {
            f32 charWidth = font->char_width(cursor);
            font->render_char(cursor);
            create_dl_translation_matrix(MXUI_MTX_NOPUSH, charWidth, 0.0f, 0.0f);
        }
        cursor = mxui_unicode_next_char(cursor);
    }
    mxui_render_reset_texture_clipping();

    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    mxui_font_set_current(lastFont);
}

static void mxui_hud_draw_texture_raw(const Texture* texture, u32 width, u32 height, u8 fmt, u8 siz, f32 x, f32 y, f32 scaleW, f32 scaleH) {
    if (texture == NULL) {
        return;
    }

    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedW = scaleW;
    f32 translatedH = scaleH;

    mxui_hud_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, sMxuiHudZ);

    mxui_hud_size_translate(&translatedW);
    mxui_hud_size_translate(&translatedH);
    mxui_hud_apply_rotation(width * translatedW, height * translatedH);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, width * translatedW, height * translatedH, 1.0f);

    mxui_hud_set_draw_color();
    mxui_render_texture_raw(texture, width, height, fmt, siz, sFilter == MXUI_HUD_FILTER_LINEAR);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

static void mxui_hud_draw_texture_tile_raw(const Texture* texture, u32 width, u32 height, u8 fmt, u8 siz, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH) {
    if (texture == NULL) {
        return;
    }

    if (width != 0) {
        scaleW *= (f32)tileW / (f32)width;
    }
    if (height != 0) {
        scaleH *= (f32)tileH / (f32)height;
    }

    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedW = scaleW;
    f32 translatedH = scaleH;

    mxui_hud_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, sMxuiHudZ);

    mxui_hud_size_translate(&translatedW);
    mxui_hud_size_translate(&translatedH);
    f32 aspect = tileH ? ((f32)tileW / (f32)tileH) : 1.0f;
    mxui_hud_apply_rotation_aspect(width * translatedW, height * translatedH, aspect);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, width * translatedW, height * translatedH, 1.0f);

    mxui_hud_set_draw_color();
    mxui_render_texture_tile_raw(texture, width, height, fmt, siz, tileX, tileY, tileW, tileH, sFilter == MXUI_HUD_FILTER_LINEAR, false);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

static void mxui_hud_draw_rect_raw(f32 x, f32 y, f32 width, f32 height) {
    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedW = width;
    f32 translatedH = height;

    mxui_hud_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, sMxuiHudZ);

    mxui_hud_size_translate(&translatedW);
    mxui_hud_size_translate(&translatedH);
    mxui_hud_apply_rotation(translatedW, translatedH);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, translatedW, translatedH, 1.0f);

    mxui_hud_set_draw_color();
    gSPDisplayList(gDisplayListHead++, dl_mxui_hud_simple_rect);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

static void mxui_hud_rotate_and_translate_vec3f(Vec3f vec, Mat4* mtx, Vec3f out) {
    out[0] = (*mtx)[0][0] * vec[0] + (*mtx)[1][0] * vec[1] + (*mtx)[2][0] * vec[2];
    out[1] = (*mtx)[0][1] * vec[0] + (*mtx)[1][1] * vec[1] + (*mtx)[2][1] * vec[2];
    out[2] = (*mtx)[0][2] * vec[0] + (*mtx)[1][2] * vec[1] + (*mtx)[2][2] * vec[2];
    out[0] += (*mtx)[3][0];
    out[1] += (*mtx)[3][1];
    out[2] += (*mtx)[3][2];
}

void patch_mxui_hud_before(void) {
    sInterpMxuiHudCount = 0;
}

void patch_mxui_hud(f32 delta) {
    f32 savedZ = sMxuiHudZ;
    Gfx* savedHeadPos = gDisplayListHead;
    u8 savedResolution = sResolution;
    struct MxuiHudRotation savedRotation = sRotation;

    for (u16 i = 0; i < sInterpMxuiHudCount; i++) {
        struct InterpMxuiHud* interp = &sInterpMxuiHuds[i];
        f32 x = delta_interpolate_f32(interp->prevX, interp->x, delta);
        f32 y = delta_interpolate_f32(interp->prevY, interp->y, delta);
        f32 scaleW = delta_interpolate_f32(interp->prevScaleW, interp->scaleW, delta);
        f32 scaleH = delta_interpolate_f32(interp->prevScaleH, interp->scaleH, delta);
        sResolution = interp->resolution;
        sRotation = interp->rotation;
        sMxuiHudZ = interp->z;
        gDisplayListHead = interp->headPos;

        f32 translatedX = x;
        f32 translatedY = y;
        mxui_hud_position_translate(&translatedX, &translatedY);
        create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, sMxuiHudZ);

        f32 translatedW = scaleW;
        f32 translatedH = scaleH;
        mxui_hud_size_translate(&translatedW);
        mxui_hud_size_translate(&translatedH);
        if (sRotation.rotationDiff != 0.0f || sRotation.rotation != 0.0f) {
            s32 rotation = delta_interpolate_s32(sRotation.rotation - sRotation.rotationDiff, sRotation.rotation, delta);
            f32 pivotX = delta_interpolate_f32(sRotation.prevPivotX, sRotation.pivotX, delta);
            f32 pivotY = delta_interpolate_f32(sRotation.prevPivotY, sRotation.pivotY, delta);
            f32 pivotTranslationX = interp->width * translatedW * pivotX;
            f32 pivotTranslationY = interp->height * translatedH * pivotY;
            create_dl_translation_matrix(MXUI_MTX_NOPUSH, +pivotTranslationX, -pivotTranslationY, 0);
            create_dl_rotation_matrix(MXUI_MTX_NOPUSH, rotation, 0, 0, 1);
            create_dl_translation_matrix(MXUI_MTX_NOPUSH, -pivotTranslationX, +pivotTranslationY, 0);
        }

        create_dl_scale_matrix(MXUI_MTX_NOPUSH, interp->width * translatedW, interp->height * translatedH, 1.0f);
    }

    sResolution = savedResolution;
    sRotation = savedRotation;
    sMxuiHudZ = savedZ;
    gDisplayListHead = savedHeadPos;
}

u8 mxui_hud_get_resolution(void) {
    return sResolution;
}

void mxui_hud_set_resolution(u8 resolutionType) {
    if (resolutionType >= MXUI_HUD_RESOLUTION_COUNT) {
        return;
    }
    sResolution = resolutionType;
}

u8 mxui_hud_get_filter(void) {
    return sFilter;
}

void mxui_hud_set_filter(u8 filterType) {
    if (filterType >= MXUI_HUD_FILTER_COUNT) {
        return;
    }
    sFilter = filterType;
}

u8 mxui_hud_get_font(void) {
    return sFont;
}

void mxui_hud_set_font(s8 fontType) {
    if (fontType < 0 || fontType >= FONT_COUNT) {
        return;
    }
    sFont = fontType;
}

struct MxuiHudColor* mxui_hud_get_color(void) {
    sRefColor = sColor;
    return &sRefColor;
}

void mxui_hud_set_color(u8 r, u8 g, u8 b, u8 a) {
    sColor.r = r;
    sColor.g = g;
    sColor.b = b;
    sColor.a = a;
    sColorAltered = true;
    mxui_render_set_color(r, g, b, a);
}

void mxui_hud_reset_color(void) {
    if (!sColorAltered) {
        return;
    }
    sColor.r = 255;
    sColor.g = 255;
    sColor.b = 255;
    sColor.a = 255;
    sColorAltered = false;
    mxui_render_set_color(255, 255, 255, 255);
}

struct MxuiHudRotation* mxui_hud_get_rotation(void) {
    return &sRotation;
}

void mxui_hud_set_rotation(s16 rotation, f32 pivotX, f32 pivotY) {
    sRotation.rotationDiff = 0.0f;
    sRotation.prevPivotX = pivotX;
    sRotation.prevPivotY = pivotY;
    sRotation.rotation = (rotation * 180.0f) / 0x8000;
    sRotation.pivotX = pivotX;
    sRotation.pivotY = pivotY;
}

void mxui_hud_set_rotation_interpolated(s32 prevRotation, f32 prevPivotX, f32 prevPivotY, s32 rotation, f32 pivotX, f32 pivotY) {
    f32 normalizedDiff = ((rotation - prevRotation + 0x8000) & 0xFFFF) - 0x8000;
    sRotation.rotationDiff = (normalizedDiff * 180.0f) / 0x8000;
    sRotation.prevPivotX = prevPivotX;
    sRotation.prevPivotY = prevPivotY;
    sRotation.rotation = (rotation * 180.0f) / 0x8000;
    sRotation.pivotX = pivotX;
    sRotation.pivotY = pivotY;
}

u32 mxui_hud_get_screen_width(void) {
    if (sResolution == MXUI_HUD_RESOLUTION_N64) {
        return GFX_DIMENSIONS_ASPECT_RATIO * SCREEN_HEIGHT;
    }
    return (u32)mxui_render_screen_width();
}

u32 mxui_hud_get_screen_height(void) {
    if (sResolution == MXUI_HUD_RESOLUTION_N64) {
        return SCREEN_HEIGHT;
    }
    return (u32)mxui_render_screen_height();
}

f32 mxui_hud_get_mouse_x(void) {
    return mxui_render_mouse_x();
}

f32 mxui_hud_get_mouse_y(void) {
    return mxui_render_mouse_y();
}

f32 mxui_hud_get_raw_mouse_x(void) {
    return mouse_x;
}

f32 mxui_hud_get_raw_mouse_y(void) {
    return mouse_y;
}

bool mxui_hud_is_mouse_locked(void) {
    return mouse_relative_enabled;
}

void mxui_hud_set_mouse_locked(bool locked) {
    if (locked) {
        controller_mouse_enter_relative();
    } else {
        controller_mouse_leave_relative();
    }
}

u8 mxui_hud_get_mouse_buttons_down(void) {
    return mouse_window_buttons;
}

u8 mxui_hud_get_mouse_buttons_pressed(void) {
    return mouse_window_buttons_pressed;
}

u8 mxui_hud_get_mouse_buttons_released(void) {
    return mouse_window_buttons_released;
}

f32 mxui_hud_get_mouse_scroll_x(void) {
    return mouse_scroll_x;
}

f32 mxui_hud_get_mouse_scroll_y(void) {
    return mxui_render_mouse_scroll_y();
}

void mxui_hud_set_viewport(f32 x, f32 y, f32 width, f32 height) {
    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedW = width;
    f32 translatedH = height;
    mxui_hud_position_translate(&translatedX, &translatedY);
    mxui_hud_size_translate(&translatedW);
    mxui_hud_size_translate(&translatedH);

    static Vp vp = {{
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    }};
    vp.vp.vscale[0] = (s16)(translatedW * 2.0f);
    vp.vp.vscale[1] = (s16)(translatedH * 2.0f);
    vp.vp.vscale[2] = G_MAXZ / 2;
    vp.vp.vscale[3] = 0;
    vp.vp.vtrans[0] = (s16)((translatedX + translatedW) * 2.0f);
    vp.vp.vtrans[1] = (s16)((translatedY - translatedH) * 2.0f);
    vp.vp.vtrans[2] = G_MAXZ / 2;
    vp.vp.vtrans[3] = 0;
    gSPViewport(gDisplayListHead++, &vp);
}

void mxui_hud_reset_viewport(void) {
    extern Vp gViewportFullscreen;
    gSPViewport(gDisplayListHead++, &gViewportFullscreen);
}

void mxui_hud_set_scissor(f32 x, f32 y, f32 width, f32 height) {
    if (sResolution == MXUI_HUD_RESOLUTION_DJUI) {
        mxui_render_set_scissor(x, y, width, height);
        return;
    }

    f32 ulx = clamp(x, 0.0f, SCREEN_WIDTH);
    f32 uly = clamp(y, 0.0f, SCREEN_HEIGHT);
    f32 lrx = clamp(x + width, ulx, SCREEN_WIDTH);
    f32 lry = clamp(y + height, uly, SCREEN_HEIGHT);
    gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);
}

void mxui_hud_reset_scissor(void) {
    mxui_render_reset_scissor();
}

f32 mxui_hud_measure_text(const char* message) {
    if (message == NULL) {
        return 0.0f;
    }
    enum MxuiFontType lastFont = mxui_font_get_current();
    mxui_font_set_current((enum MxuiFontType)sFont);
    f32 width = mxui_font_measure_text_raw(message);
    mxui_font_set_current(lastFont);
    return width;
}

void mxui_hud_print_text(const char* message, f32 x, f32 y, f32 scale) {
    if (message == NULL) {
        return;
    }
    sMxuiHudZ += 0.01f;
    mxui_hud_draw_text_raw(message, x, y, scale);
}

void mxui_hud_print_text_interpolated(const char* message, f32 prevX, f32 prevY, f32 prevScale, f32 x, f32 y, f32 scale) {
    if (message == NULL) {
        return;
    }

    Gfx* savedHeadPos = gDisplayListHead;
    f32 savedZ = sMxuiHudZ;

    sMxuiHudZ += 0.01f;
    mxui_hud_draw_text_raw(message, prevX, prevY, prevScale);

    if (sInterpMxuiHudCount >= MAX_INTERP_HUD) {
        return;
    }

    const struct MxuiFont* font = gMxuiFonts[sFont];
    struct InterpMxuiHud* interp = &sInterpMxuiHuds[sInterpMxuiHudCount++];
    interp->headPos = savedHeadPos;
    interp->prevX = prevX;
    interp->prevY = prevY;
    interp->prevScaleW = prevScale;
    interp->prevScaleH = prevScale;
    interp->x = x;
    interp->y = y;
    interp->scaleW = scale;
    interp->scaleH = scale;
    interp->width = font->defaultFontScale;
    interp->height = font->defaultFontScale;
    interp->z = savedZ;
    interp->resolution = sResolution;
    interp->rotation = sRotation;
}

void mxui_hud_render_texture(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH) {
    if (texInfo == NULL) {
        return;
    }
    sMxuiHudZ += 0.01f;
    mxui_hud_draw_texture_raw(texInfo->texture, texInfo->width, texInfo->height, texInfo->format, texInfo->size, x, y, scaleW, scaleH);
}

void mxui_hud_render_texture_tile(struct TextureInfo* texInfo, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH) {
    if (texInfo == NULL) {
        return;
    }
    sMxuiHudZ += 0.01f;
    mxui_hud_draw_texture_tile_raw(texInfo->texture, texInfo->width, texInfo->height, texInfo->format, texInfo->size, x, y, scaleW, scaleH, tileX, tileY, tileW, tileH);
}

void mxui_hud_render_texture_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH) {
    if (texInfo == NULL) {
        return;
    }

    Gfx* savedHeadPos = gDisplayListHead;
    f32 savedZ = sMxuiHudZ;

    sMxuiHudZ += 0.01f;
    mxui_hud_draw_texture_raw(texInfo->texture, texInfo->width, texInfo->height, texInfo->format, texInfo->size, prevX, prevY, prevScaleW, prevScaleH);

    if (sInterpMxuiHudCount >= MAX_INTERP_HUD) {
        return;
    }

    struct InterpMxuiHud* interp = &sInterpMxuiHuds[sInterpMxuiHudCount++];
    interp->headPos = savedHeadPos;
    interp->prevX = prevX;
    interp->prevY = prevY;
    interp->prevScaleW = prevScaleW;
    interp->prevScaleH = prevScaleH;
    interp->x = x;
    interp->y = y;
    interp->scaleW = scaleW;
    interp->scaleH = scaleH;
    interp->width = texInfo->width;
    interp->height = texInfo->height;
    interp->z = savedZ;
    interp->resolution = sResolution;
    interp->rotation = sRotation;
}

void mxui_hud_render_texture_tile_interpolated(struct TextureInfo* texInfo, f32 prevX, f32 prevY, f32 prevScaleW, f32 prevScaleH, f32 x, f32 y, f32 scaleW, f32 scaleH, u32 tileX, u32 tileY, u32 tileW, u32 tileH) {
    if (texInfo == NULL) {
        return;
    }

    Gfx* savedHeadPos = gDisplayListHead;
    f32 savedZ = sMxuiHudZ;

    sMxuiHudZ += 0.01f;
    mxui_hud_draw_texture_tile_raw(texInfo->texture, texInfo->width, texInfo->height, texInfo->format, texInfo->size, prevX, prevY, prevScaleW, prevScaleH, tileX, tileY, tileW, tileH);

    if (sInterpMxuiHudCount >= MAX_INTERP_HUD) {
        return;
    }

    if (texInfo->width != 0) {
        scaleW *= ((f32)tileW / (f32)texInfo->width);
        prevScaleW *= ((f32)tileW / (f32)texInfo->width);
    }
    if (texInfo->height != 0) {
        scaleH *= ((f32)tileH / (f32)texInfo->height);
        prevScaleH *= ((f32)tileH / (f32)texInfo->height);
    }

    struct InterpMxuiHud* interp = &sInterpMxuiHuds[sInterpMxuiHudCount++];
    interp->headPos = savedHeadPos;
    interp->prevX = prevX;
    interp->prevY = prevY;
    interp->prevScaleW = prevScaleW;
    interp->prevScaleH = prevScaleH;
    interp->x = x;
    interp->y = y;
    interp->scaleW = scaleW;
    interp->scaleH = scaleH;
    interp->width = texInfo->width;
    interp->height = texInfo->height;
    interp->z = savedZ;
    interp->resolution = sResolution;
    interp->rotation = sRotation;
}

void mxui_hud_render_rect(f32 x, f32 y, f32 width, f32 height) {
    sMxuiHudZ += 0.01f;
    mxui_hud_draw_rect_raw(x, y, width, height);
}

void mxui_hud_render_rect_interpolated(f32 prevX, f32 prevY, f32 prevWidth, f32 prevHeight, f32 x, f32 y, f32 width, f32 height) {
    Gfx* savedHeadPos = gDisplayListHead;
    f32 savedZ = sMxuiHudZ;

    sMxuiHudZ += 0.01f;
    mxui_hud_draw_rect_raw(prevX, prevY, prevWidth, prevHeight);

    if (sInterpMxuiHudCount >= MAX_INTERP_HUD) {
        return;
    }

    struct InterpMxuiHud* interp = &sInterpMxuiHuds[sInterpMxuiHudCount++];
    interp->headPos = savedHeadPos;
    interp->prevX = prevX;
    interp->prevY = prevY;
    interp->prevScaleW = prevWidth;
    interp->prevScaleH = prevHeight;
    interp->x = x;
    interp->y = y;
    interp->scaleW = width;
    interp->scaleH = height;
    interp->width = 1.0f;
    interp->height = 1.0f;
    interp->z = savedZ;
    interp->resolution = sResolution;
    interp->rotation = sRotation;
}

void mxui_hud_render_line(f32 p1X, f32 p1Y, f32 p2X, f32 p2Y, f32 size) {
    f32 dx = p2X - p1X;
    f32 dy = p2Y - p1Y;
    f32 angle = atan2s(dy, dx) - 0x4000;
    f32 hDist = sqrtf((dx * dx) + (dy * dy));
    mxui_hud_set_rotation(angle, 0.0f, 0.5f);
    mxui_hud_render_rect(p1X, p1Y, hDist, size);
    mxui_hud_set_rotation(0, 0.0f, 0.0f);
}

f32 mxui_hud_get_current_fov(void) {
    return get_first_person_enabled() ? gFirstPersonCamera.fov : replace_value_if_not_zero(gFOVState.fov, gOverrideFOV) + gFOVState.fovOffset;
}

f32 mxui_hud_get_fov_coeff(void) {
    f32 fov = mxui_hud_get_current_fov();
    f32 fovDefault = tanf(45.0f * ((f32)M_PI / 360.0f));
    f32 fovCurrent = tanf(fov * ((f32)M_PI / 360.0f));
    return (fovDefault / fovCurrent) * 1.13f;
}

bool mxui_hud_world_pos_to_screen_pos(Vec3f pos, VEC_OUT Vec3f out) {
    if (!gCamera) {
        return false;
    }

    mxui_hud_rotate_and_translate_vec3f(pos, &gCamera->mtx, out);
    if (out[2] >= 0.0f) {
        return false;
    }

    out[0] *= 256.0f / -out[2];
    out[1] *= 256.0f / out[2];

    f32 fovCoeff = mxui_hud_get_fov_coeff();
    out[0] *= fovCoeff;
    out[1] *= fovCoeff;

    f32 screenWidth = 0.0f;
    f32 screenHeight = 0.0f;
    if (sResolution == MXUI_HUD_RESOLUTION_N64) {
        screenWidth = GFX_DIMENSIONS_ASPECT_RATIO * SCREEN_HEIGHT;
        screenHeight = SCREEN_HEIGHT;
    } else {
        u32 windowWidth = 0;
        u32 windowHeight = 0;
        gfx_get_dimensions(&windowWidth, &windowHeight);
        screenWidth = (f32)windowWidth;
        screenHeight = (f32)windowHeight;
    }

    out[0] += screenWidth * 0.5f;
    out[1] += screenHeight * 0.5f;

    extern Vp* gViewportOverride;
    if (gViewportOverride) {
        Vp_t* viewport = &gViewportOverride->vp;
        f32 width = viewport->vscale[0] / 2.0f;
        f32 height = viewport->vscale[1] / 2.0f;
        f32 x = (viewport->vtrans[0] / 4.0f) - width / 2.0f;
        f32 y = SCREEN_HEIGHT - ((viewport->vtrans[1] / 4.0f) + height / 2.0f);

        f32 xDiff = screenWidth / SCREEN_WIDTH;
        f32 yDiff = screenHeight / SCREEN_HEIGHT;
        width *= xDiff;
        height *= yDiff;
        x = x * xDiff - 1.0f;
        y = (screenHeight - y * yDiff) - height;

        out[0] = x + (out[0] * (width / screenWidth));
        out[1] = y + (out[1] * (height / screenHeight));
    }

    return true;
}

bool mxui_hud_is_pause_menu_created(void) {
    return mxui_is_pause_active();
}
