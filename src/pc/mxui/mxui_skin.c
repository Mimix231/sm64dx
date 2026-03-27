#include <math.h>

#include "mxui_internal.h"

static struct MxuiColor mxui_skin_mix(struct MxuiColor a, struct MxuiColor b, f32 t) {
    t = mxui_clampf(t, 0.0f, 1.0f);
    return mxui_color(
        (u8)(a.r + (b.r - a.r) * t),
        (u8)(a.g + (b.g - a.g) * t),
        (u8)(a.b + (b.b - a.b) * t),
        (u8)(a.a + (b.a - a.a) * t)
    );
}

static struct MxuiColor mxui_skin_alpha(struct MxuiColor color, f32 alphaScale) {
    color.a = (u8)mxui_clampf((f32)color.a * alphaScale, 0.0f, 255.0f);
    return color;
}

static void mxui_skin_draw_round_fill(struct MxuiRect rect, f32 radius, struct MxuiColor color) {
    if (rect.w <= 0.0f || rect.h <= 0.0f || color.a == 0) {
        return;
    }

    radius = MIN(radius, MIN(rect.w, rect.h) * 0.5f);
    if (radius <= 1.0f) {
        mxui_draw_rect(rect, color);
        return;
    }

    const f32 step = 2.0f;
    f32 bodyTop = rect.y + radius;
    f32 bodyHeight = rect.h - radius * 2.0f;
    if (bodyHeight > 0.0f) {
        mxui_draw_rect((struct MxuiRect){ rect.x, bodyTop, rect.w, bodyHeight }, color);
    }

    for (f32 offset = 0.0f; offset < radius; offset += step) {
        f32 sampleY = offset + step * 0.5f;
        if (sampleY > radius) {
            sampleY = radius;
        }
        f32 dy = radius - sampleY;
        f32 inset = radius - sqrtf(MAX(0.0f, radius * radius - dy * dy));
        f32 rowHeight = MIN(step, radius - offset);
        mxui_draw_rect((struct MxuiRect){ rect.x + inset, rect.y + offset, rect.w - inset * 2.0f, rowHeight }, color);
        mxui_draw_rect((struct MxuiRect){ rect.x + inset, rect.y + rect.h - offset - rowHeight, rect.w - inset * 2.0f, rowHeight }, color);
    }
}

void mxui_skin_draw_panel(struct MxuiRect rect, struct MxuiColor fill, struct MxuiColor border, struct MxuiColor glow, f32 radius, f32 borderWidth) {
    if (glow.a > 0) {
        for (s32 layer = 3; layer >= 1; layer--) {
            f32 spread = (f32)layer * 3.0f;
            struct MxuiRect glowRect = {
                rect.x - spread,
                rect.y - spread,
                rect.w + spread * 2.0f,
                rect.h + spread * 2.0f,
            };
            struct MxuiColor glowLayer = glow;
            glowLayer.a = (u8)(glow.a / (layer + 1));
            mxui_skin_draw_round_fill(glowRect, radius + spread, glowLayer);
        }
    }

    mxui_skin_draw_round_fill(rect, radius, border);
    struct MxuiRect inner = {
        rect.x + borderWidth,
        rect.y + borderWidth,
        rect.w - borderWidth * 2.0f,
        rect.h - borderWidth * 2.0f,
    };
    f32 innerRadius = MAX(0.0f, radius - borderWidth);
    struct MxuiColor white = mxui_color(255, 255, 255, fill.a);
    struct MxuiColor black = mxui_color(0, 0, 0, fill.a);
    struct MxuiColor base = fill;
    struct MxuiColor top = mxui_skin_mix(fill, white, 0.12f);
    struct MxuiColor bottom = mxui_skin_mix(fill, black, 0.12f);

    mxui_skin_draw_round_fill(inner, innerRadius, base);

    struct MxuiRect highlight = {
        inner.x + 3.0f,
        inner.y + 3.0f,
        MAX(0.0f, inner.w - 6.0f),
        MAX(0.0f, inner.h * 0.26f),
    };
    mxui_skin_draw_round_fill(highlight, MAX(0.0f, innerRadius - 3.0f), mxui_skin_alpha(top, 0.42f));

    struct MxuiRect shadowBand = {
        inner.x + 4.0f,
        inner.y + inner.h * 0.58f,
        MAX(0.0f, inner.w - 8.0f),
        MAX(0.0f, inner.h * 0.26f),
    };
    mxui_skin_draw_round_fill(shadowBand, MAX(0.0f, innerRadius - 4.0f), mxui_skin_alpha(bottom, 0.26f));

    struct MxuiRect innerOutline = {
        inner.x + 1.0f,
        inner.y + 1.0f,
        MAX(0.0f, inner.w - 2.0f),
        MAX(0.0f, inner.h - 2.0f),
    };
    mxui_skin_draw_round_fill((struct MxuiRect){ innerOutline.x, innerOutline.y, innerOutline.w, 1.0f }, MAX(0.0f, innerRadius - 1.0f), mxui_skin_alpha(mxui_skin_mix(border, white, 0.22f), 0.55f));
    mxui_skin_draw_round_fill((struct MxuiRect){ innerOutline.x, innerOutline.y + innerOutline.h - 1.0f, innerOutline.w, 1.0f }, MAX(0.0f, innerRadius - 1.0f), mxui_skin_alpha(mxui_skin_mix(border, black, 0.18f), 0.45f));
}
