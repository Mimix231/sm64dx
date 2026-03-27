#include "mxui_render.h"

#include "sm64.h"

#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/memory.h"
#include "pc/controller/controller_mouse.h"
#include "pc/gfx/gfx.h"
#include "gfx_dimensions.h"

#include "engine/math_util.h"

void gfx_get_dimensions(uint32_t *width, uint32_t *height);

static const Vtx sMxuiSimpleRectVertices[] = {
    {{{ 0, -1, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 1, -1, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 1,  0, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
    {{{ 0,  0, 0 }, 0, { 0, 0 }, { 0xff, 0xff, 0xff, 0xff }}},
};

static const Gfx dl_mxui_simple_rect[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING),
    gsDPSetCombineMode(G_CC_FADE, G_CC_FADE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPVertexNonGlobal(sMxuiSimpleRectVertices, 4, 0),
    gsSP2Triangles(0, 1, 2, 0x0, 0, 2, 3, 0x0),
    gsSPEndDisplayList(),
};

static const Gfx dl_mxui_display_list_begin[] = {
    gsSPTextureAddrMxui(1),
    gsSPEndDisplayList(),
};

static const Gfx dl_mxui_display_list_end[] = {
    gsSPTextureAddrMxui(0),
    gsSPEndDisplayList(),
};

static f32 mxui_render_scale_internal(void) {
    if (configDjuiScale == 0) {
        u32 windowWidth = 0;
        u32 windowHeight = 0;
        gfx_get_dimensions(&windowWidth, &windowHeight);
        return clamp(roundf((((f32)windowHeight / (f32)SCREEN_HEIGHT) / 4.0f) / 0.5f) * 0.5f, 0.5f, 1.5f);
    }

    switch (configDjuiScale) {
        case 1:  return 0.5f;
        case 2:  return 0.85f;
        case 3:  return 1.0f;
        case 4:  return 1.5f;
        default: return 1.0f;
    }
}

static u8 mxui_render_power_of_two(u32 value) {
    return (u8)log2f(value);
}

f32 mxui_render_screen_width(void) {
    u32 windowWidth = 0;
    u32 windowHeight = 0;
    gfx_get_dimensions(&windowWidth, &windowHeight);
    (void)windowHeight;
    return (f32)windowWidth / mxui_render_scale_internal();
}

f32 mxui_render_screen_height(void) {
    u32 windowWidth = 0;
    u32 windowHeight = 0;
    gfx_get_dimensions(&windowWidth, &windowHeight);
    (void)windowWidth;
    return (f32)windowHeight / mxui_render_scale_internal();
}

f32 mxui_render_scale(void) {
    return mxui_render_scale_internal();
}

f32 mxui_render_mouse_x(void) {
    controller_mouse_read_window();
    return (mouse_window_x + gfx_current_dimensions.x_adjust_4by3) / mxui_render_scale_internal();
}

f32 mxui_render_mouse_y(void) {
    controller_mouse_read_window();
    return mouse_window_y / mxui_render_scale_internal();
}

f32 mxui_render_mouse_scroll_y(void) {
    return mouse_scroll_y;
}

u32 mxui_render_mouse_buttons_down(void) {
    return mouse_window_buttons;
}

void mxui_render_position_translate(f32* x, f32* y) {
    u32 windowWidth = 0;
    u32 windowHeight = 0;
    gfx_get_dimensions(&windowWidth, &windowHeight);
    *x = GFX_DIMENSIONS_FROM_LEFT_EDGE(0) + *x * ((f32)SCREEN_HEIGHT / (f32)windowHeight) * mxui_render_scale_internal();
    *y = SCREEN_HEIGHT - *y * ((f32)SCREEN_HEIGHT / (f32)windowHeight) * mxui_render_scale_internal();
}

void mxui_render_size_translate(f32* size) {
    u32 windowWidth = 0;
    u32 windowHeight = 0;
    gfx_get_dimensions(&windowWidth, &windowHeight);
    (void)windowWidth;
    *size = *size * ((f32)SCREEN_HEIGHT / (f32)windowHeight) * mxui_render_scale_internal();
}

void mxui_render_set_color(u8 r, u8 g, u8 b, u8 a) {
    gDPSetEnvColor(gDisplayListHead++, r, g, b, a);
}

void mxui_render_displaylist_begin(void) {
    gSPDisplayList(gDisplayListHead++, dl_mxui_display_list_begin);
}

void mxui_render_displaylist_end(void) {
    gSPDisplayList(gDisplayListHead++, dl_mxui_display_list_end);
}

void mxui_render_rect(f32 x, f32 y, f32 w, f32 h) {
    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedW = w;
    f32 translatedH = h;
    mxui_render_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, 0.0f);
    mxui_render_size_translate(&translatedW);
    mxui_render_size_translate(&translatedH);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, translatedW, translatedH, 1.0f);
    gSPDisplayList(gDisplayListHead++, dl_mxui_simple_rect);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

void mxui_render_texture_raw(const Texture* texture, u32 w, u32 h, u8 fmt, u8 siz, bool filter) {
    if (texture == NULL) {
        return;
    }

    gDPSetTextureFilter(gDisplayListHead++, filter ? G_TF_BILERP : G_TF_POINT);
    gDPSetTextureOverrideMxui(gDisplayListHead++, texture, mxui_render_power_of_two(w), mxui_render_power_of_two(h), fmt, siz);
    gDPPipeSync(gDisplayListHead++);
    gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING | G_CULL_BOTH);
    gDPSetCombineMode(gDisplayListHead++, G_CC_FADEA, G_CC_FADEA);
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlockWithoutTexture(gDisplayListHead++, NULL, G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, 64, 0, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0);
    *(gDisplayListHead++) = (Gfx)gsSPExecuteMxui(G_TEXOVERRIDE_DJUI);
    gSPVertexNonGlobal(gDisplayListHead++, sMxuiSimpleRectVertices, 4, 0);
    gSP2TrianglesMxui(gDisplayListHead++, 0, 1, 2, 0x0, 0, 2, 3, 0x0);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gDisplayListHead++, G_CC_SHADE, G_CC_SHADE);
    gSPSetGeometryMode(gDisplayListHead++, G_LIGHTING | G_CULL_BACK);
}

void mxui_render_texture_tile_raw(const Texture* texture, u32 w, u32 h, u8 fmt, u8 siz, u32 tileX, u32 tileY, u32 tileW, u32 tileH, bool filter, bool font) {
    if (!gDisplayListHead || texture == NULL) {
        return;
    }

    Vtx* vtx = alloc_display_list(sizeof(Vtx) * 4);
    if (vtx == NULL) {
        return;
    }

    f32 aspect = tileH ? ((f32)tileW / (f32)tileH) : 1.0f;
    f32 offsetX = (font ? -1024.0f / (f32)w : 0.0f) + 1.0f;
    f32 offsetY = (font ? -1024.0f / (f32)h : 0.0f) + 1.0f;

    vtx[0] = (Vtx){{{ 0,          -1, 0 }, 0, { ( tileX          * 2048.0f) / (f32)w + offsetX, ((tileY + tileH) * 2048.0f) / (f32)h + offsetY }, { 0xff, 0xff, 0xff, 0xff }}};
    vtx[1] = (Vtx){{{ 1 * aspect, -1, 0 }, 0, { ((tileX + tileW) * 2048.0f) / (f32)w + offsetX, ((tileY + tileH) * 2048.0f) / (f32)h + offsetY }, { 0xff, 0xff, 0xff, 0xff }}};
    vtx[2] = (Vtx){{{ 1 * aspect,  0, 0 }, 0, { ((tileX + tileW) * 2048.0f) / (f32)w + offsetX, ( tileY          * 2048.0f) / (f32)h + offsetY }, { 0xff, 0xff, 0xff, 0xff }}};
    vtx[3] = (Vtx){{{ 0,           0, 0 }, 0, { ( tileX          * 2048.0f) / (f32)w + offsetX, ( tileY          * 2048.0f) / (f32)h + offsetY }, { 0xff, 0xff, 0xff, 0xff }}};

    gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING | G_CULL_BOTH);
    gDPSetCombineMode(gDisplayListHead++, G_CC_FADEA, G_CC_FADEA);
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(gDisplayListHead++, filter ? G_TF_BILERP : G_TF_POINT);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureOverrideMxui(gDisplayListHead++, texture, mxui_render_power_of_two(w), mxui_render_power_of_two(h), fmt, siz);
    gDPLoadTextureBlockWithoutTexture(gDisplayListHead++, NULL, G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, 64, 0, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0);
    *(gDisplayListHead++) = (Gfx)gsSPExecuteMxui(G_TEXOVERRIDE_DJUI);
    gSPVertexNonGlobal(gDisplayListHead++, vtx, 4, 0);
    *(gDisplayListHead++) = (Gfx)gsSPExecuteMxui(G_TEXCLIP_DJUI);
    gSP2TrianglesMxui(gDisplayListHead++, 0, 1, 2, 0x0, 0, 2, 3, 0x0);
    gSPTexture(gDisplayListHead++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gDisplayListHead++, G_CC_SHADE, G_CC_SHADE);
    gSPSetGeometryMode(gDisplayListHead++, G_LIGHTING | G_CULL_BACK);
}

void mxui_render_texture(const struct TextureInfo* texture, f32 x, f32 y, f32 scaleX, f32 scaleY) {
    if (texture == NULL || texture->texture == NULL) {
        return;
    }

    f32 translatedX = x;
    f32 translatedY = y;
    f32 translatedScaleX = scaleX;
    f32 translatedScaleY = scaleY;
    mxui_render_position_translate(&translatedX, &translatedY);
    create_dl_translation_matrix(MXUI_MTX_PUSH, translatedX, translatedY, 0.0f);
    mxui_render_size_translate(&translatedScaleX);
    mxui_render_size_translate(&translatedScaleY);
    create_dl_scale_matrix(MXUI_MTX_NOPUSH, texture->width * translatedScaleX, texture->height * translatedScaleY, 1.0f);
    mxui_render_texture_raw(texture->texture, texture->width, texture->height, texture->format, texture->size, false);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

void mxui_render_set_scissor(f32 x, f32 y, f32 w, f32 h) {
    u32 windowWidth = 0;
    u32 windowHeight = 0;
    gfx_get_dimensions(&windowWidth, &windowHeight);

    f32 ulx = x;
    f32 uly = y;
    f32 lrx = x + w;
    f32 lry = y + h;
    f32 factor = ((f32)SCREEN_HEIGHT / (f32)windowHeight) * mxui_render_scale_internal();
    f32 leftEdge = GFX_DIMENSIONS_FROM_LEFT_EDGE(0);
    ulx = leftEdge + ulx * factor;
    uly *= factor;
    lrx = leftEdge + lrx * factor;
    lry *= factor;

    ulx = clamp(ulx, 0.0f, SCREEN_WIDTH);
    uly = clamp(uly, 0.0f, SCREEN_HEIGHT);
    lrx = clamp(lrx, ulx, SCREEN_WIDTH);
    lry = clamp(lry, uly, SCREEN_HEIGHT);

    gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);
}

void mxui_render_reset_scissor(void) {
    gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, 0, BORDER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - BORDER_HEIGHT);
}

void mxui_render_reset_texture_clipping(void) {
    gDPSetTextureClippingMxui(gDisplayListHead++, 0, 0, 0, 0);
}
