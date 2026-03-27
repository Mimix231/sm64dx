#include "mxui_internal.h"

#include "engine/math_util.h"

void mxui_scroll_into_view(struct MxuiScreenState* screen) {
    if (!sMxui.focusMovedByNav || !sMxui.focusedRectValid) { return; }
    struct MxuiRect shell = mxui_shell_rect();
    f32 top = shell.y + 120.0f;
    f32 bottom = shell.y + shell.h - 74.0f;
    if (sMxui.focusedRect.y < top) {
        screen->scroll = MAX(0.0f, screen->scroll - (top - sMxui.focusedRect.y));
    } else if (sMxui.focusedRect.y + sMxui.focusedRect.h > bottom) {
        screen->scroll += (sMxui.focusedRect.y + sMxui.focusedRect.h) - bottom;
    }
}

void mxui_scroll_apply(struct MxuiContext* ctx) {
    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL) { return; }

    f32 maxScroll = MAX(0.0f, ctx->contentHeight - ctx->content.h);
    bool mouseInsideContent = mxui_rect_contains(ctx->content, sMxui.input.mouseX, sMxui.input.mouseY);
    if (mouseInsideContent && sMxui.input.mouseScroll != 0.0f) {
        screen->scroll = mxui_clampf(screen->scroll - sMxui.input.mouseScroll * 30.0f, 0.0f, maxScroll);
    }
    screen->scroll = mxui_clampf(screen->scroll, 0.0f, maxScroll);
    sMxui.focusMovedByNav = false;
}
