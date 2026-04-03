#include "lumaui_space.h"

#include "gfx_dimensions.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"

static struct LumaUIRect sClipStack[LUMAUI_CLIP_STACK_CAPACITY];
static s32 sClipDepth = 0;

struct LumaUIRect lumaui_space_full_frame_rect(void) {
    struct LumaUIRect rect = {
        (s16) -gfx_current_dimensions.x_adjust_4by3,
        0,
        (s16) (LUMAUI_LOGICAL_WIDTH + (gfx_current_dimensions.x_adjust_4by3 * 2)),
        LUMAUI_LOGICAL_HEIGHT,
    };
    return rect;
}

static void lumaui_space_apply_clip(const struct LumaUIRect *rect) {
    s32 left = lumaui_space_to_screen_x(rect->x);
    s32 right = lumaui_space_to_screen_x(rect->x + rect->w);
    s32 top = rect->y;
    s32 bottom = rect->y + rect->h;

    if (left < 0) {
        left = 0;
    }
    if (right > SCREEN_WIDTH) {
        right = SCREEN_WIDTH;
    }
    if (top < 0) {
        top = 0;
    }
    if (bottom > SCREEN_HEIGHT) {
        bottom = SCREEN_HEIGHT;
    }
    if (right <= left) {
        right = left + 1;
    }
    if (bottom <= top) {
        bottom = top + 1;
    }

    gDPSetScissor(gDisplayListHead++, G_SC_NON_INTERLACE, left, top, right, bottom);
}

void lumaui_space_begin_frame(void) {
    sClipDepth = 1;
    sClipStack[0] = lumaui_space_full_frame_rect();
    lumaui_space_apply_clip(&sClipStack[0]);
}

s32 lumaui_space_to_screen_x(s32 x) {
    return GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(x);
}

bool lumaui_space_point_in_rect(s32 x, s32 y, const struct LumaUIRect *rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

bool lumaui_space_intersect_rect(const struct LumaUIRect *a, const struct LumaUIRect *b,
                                 struct LumaUIRect *out) {
    s16 left = a->x > b->x ? a->x : b->x;
    s16 top = a->y > b->y ? a->y : b->y;
    s16 right = (a->x + a->w) < (b->x + b->w) ? (a->x + a->w) : (b->x + b->w);
    s16 bottom = (a->y + a->h) < (b->y + b->h) ? (a->y + a->h) : (b->y + b->h);

    if (right <= left || bottom <= top) {
        if (out != NULL) {
            out->x = left;
            out->y = top;
            out->w = 0;
            out->h = 0;
        }
        return false;
    }

    if (out != NULL) {
        out->x = left;
        out->y = top;
        out->w = right - left;
        out->h = bottom - top;
    }
    return true;
}

struct LumaUIRect lumaui_space_inset_rect(const struct LumaUIRect *rect, s16 insetX, s16 insetY) {
    struct LumaUIRect result = *rect;

    result.x += insetX;
    result.y += insetY;
    result.w -= insetX * 2;
    result.h -= insetY * 2;

    if (result.w < 0) {
        result.w = 0;
    }
    if (result.h < 0) {
        result.h = 0;
    }

    return result;
}

void lumaui_space_push_clip(const struct LumaUIRect *rect) {
    struct LumaUIRect clipped = { 0 };

    if (rect == NULL || sClipDepth <= 0) {
        return;
    }
    if (sClipDepth >= LUMAUI_CLIP_STACK_CAPACITY) {
        return;
    }

    if (!lumaui_space_intersect_rect(&sClipStack[sClipDepth - 1], rect, &clipped)) {
        clipped = *rect;
        clipped.w = 1;
        clipped.h = 1;
    }

    sClipStack[sClipDepth++] = clipped;
    lumaui_space_apply_clip(&clipped);
}

void lumaui_space_pop_clip(void) {
    if (sClipDepth <= 1) {
        return;
    }

    sClipDepth--;
    lumaui_space_apply_clip(&sClipStack[sClipDepth - 1]);
}

const struct LumaUIRect *lumaui_space_current_clip(void) {
    if (sClipDepth <= 0) {
        return NULL;
    }
    return &sClipStack[sClipDepth - 1];
}
