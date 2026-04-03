#ifndef LUMAUI_SPACE_H
#define LUMAUI_SPACE_H

#include <stdbool.h>
#include <PR/ultratypes.h>

#define LUMAUI_LOGICAL_WIDTH 320
#define LUMAUI_LOGICAL_HEIGHT 240
#define LUMAUI_CLIP_STACK_CAPACITY 16

struct LumaUIRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
};

void lumaui_space_begin_frame(void);
struct LumaUIRect lumaui_space_full_frame_rect(void);

s32 lumaui_space_to_screen_x(s32 x);
bool lumaui_space_point_in_rect(s32 x, s32 y, const struct LumaUIRect *rect);
bool lumaui_space_intersect_rect(const struct LumaUIRect *a, const struct LumaUIRect *b,
                                 struct LumaUIRect *out);
struct LumaUIRect lumaui_space_inset_rect(const struct LumaUIRect *rect, s16 insetX, s16 insetY);

void lumaui_space_push_clip(const struct LumaUIRect *rect);
void lumaui_space_pop_clip(void);
const struct LumaUIRect *lumaui_space_current_clip(void);

#endif
