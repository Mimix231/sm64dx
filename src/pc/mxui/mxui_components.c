#include <math.h>
#include <string.h>

#include "sm64.h"
#include "mxui_components.h"

#include "pc/controller/controller_api.h"
#include "pc/lua/smlua_hooks.h"
#include "pc/mods/mod_bindings.h"

#include "audio/external.h"
#include "sounds.h"

#include "mxui_render.h"

static bool mxui_component_rect_matches(struct MxuiRect a, struct MxuiRect b) {
    return fabsf(a.x - b.x) < 1.0f
        && fabsf(a.y - b.y) < 1.0f
        && fabsf(a.w - b.w) < 1.0f
        && fabsf(a.h - b.h) < 1.0f;
}

static void mxui_component_note_button_press(struct MxuiRect rect) {
    sMxui.pressedRect = rect;
    sMxui.pressedRectValid = true;
    sMxui.pressedFrames = 6;
}

static void mxui_component_play_click_sound(bool danger) {
    play_sound(danger ? SOUND_MENU_CAMERA_BUZZ : SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
}

static bool mxui_component_consume_mouse_press(void) {
    if (!sMxui.input.mousePressed) {
        return false;
    }
    sMxui.input.mousePressed = false;
    return true;
}

static bool mxui_component_consume_accept_press(void) {
    if (!sMxui.input.accept) {
        return false;
    }
    sMxui.input.accept = false;
    return true;
}

static void mxui_component_consume_release_and_back(void) {
    sMxui.input.mouseReleased = false;
    sMxui.input.accept = false;
    sMxui.input.back = false;
    sMxui.mouseCaptureValid = false;
    sMxui.pressedRectValid = false;
}

struct MxuiComponentSlots {
    struct MxuiRect labelRect;
    struct MxuiRect controlRect;
    bool stacked;
};

static struct MxuiComponentSlots mxui_component_widget_slots(struct MxuiRect rect, const char* label, f32 minControlWidth, f32 maxControlWidth) {
    const f32 padX = 14.0f;
    const f32 gap = 12.0f;
    const f32 available = MAX(0.0f, rect.w - padX * 2.0f);
    const f32 labelIdeal = mxui_measure(label != NULL ? label : "", FONT_NORMAL, 0.78f) + 20.0f;
    f32 controlWidth = mxui_clampf(MAX(minControlWidth, available * 0.42f), minControlWidth, MIN(maxControlWidth, available - 96.0f));
    if (controlWidth > available - 72.0f) {
        controlWidth = MAX(minControlWidth * 0.78f, available * 0.46f);
    }

    f32 labelWidth = MAX(84.0f, available - controlWidth - gap);
    bool stacked = labelIdeal > labelWidth && rect.h >= 72.0f;

    if (stacked) {
        f32 labelHeight = mxui_row_height_for_text(label, 28.0f, 0.58f, 0.78f, FONT_NORMAL, available, true, 2);
        return (struct MxuiComponentSlots) {
            .labelRect = { rect.x + padX, rect.y + 8.0f, available, labelHeight },
            .controlRect = { rect.x + padX, rect.y + 8.0f + labelHeight + 8.0f, available, rect.h - (24.0f + labelHeight) },
            .stacked = true,
        };
    }

    return (struct MxuiComponentSlots) {
        .labelRect = { rect.x + padX, rect.y + 4.0f, labelWidth, rect.h - 8.0f },
        .controlRect = { rect.x + rect.w - padX - controlWidth, rect.y + 6.0f, controlWidth, rect.h - 12.0f },
        .stacked = false,
    };
}

static bool mxui_component_widget_click(struct MxuiRect rect, bool focused, bool hovered) {
    if (hovered && sMxui.input.mousePressed) {
        sMxui.mouseCaptureRect = rect;
        sMxui.mouseCaptureValid = true;
        mxui_component_consume_mouse_press();
        return false;
    }
    if (hovered && sMxui.input.mouseReleased && sMxui.mouseCaptureValid && mxui_component_rect_matches(rect, sMxui.mouseCaptureRect)) {
        sMxui.mouseCaptureValid = false;
        return true;
    }
    if (focused && mxui_component_consume_accept_press()) {
        return true;
    }
    return false;
}

bool mxui_component_button(struct MxuiRect rect, const char* label, bool danger) {
    struct MxuiTheme theme = mxui_theme();
    bool hovered = false;
    bool focused = mxui_focusable(rect, &hovered);
    bool pressed = sMxui.pressedRectValid && sMxui.pressedFrames > 0 && mxui_component_rect_matches(rect, sMxui.pressedRect);
    struct MxuiColor fill = danger ? theme.danger : theme.button;
    if (hovered) {
        fill = danger ? mxui_color(240, 92, 92, 255) : theme.buttonHover;
    } else if (focused) {
        fill = danger ? mxui_color(224, 88, 88, 255) : theme.buttonActive;
    }
    if (pressed) {
        fill = danger ? mxui_color(214, 72, 88, 255) : mxui_color(
            MIN(fill.r + 18, 255),
            MIN(fill.g + 18, 255),
            MIN(fill.b + 18, 255),
            fill.a
        );
    }

    struct MxuiColor glow = focused ? theme.glow : (hovered ? mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, theme.glow.a / 2) : mxui_color(0, 0, 0, 0));
    struct MxuiRect drawRect = rect;
    if (pressed) {
        drawRect.x += 2.0f;
        drawRect.y += 2.0f;
        drawRect.w -= 4.0f;
        drawRect.h -= 4.0f;
    }

    mxui_skin_draw_panel(drawRect, fill, theme.border, glow, 12.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(label, (struct MxuiRect){ drawRect.x + 14.0f, drawRect.y + 4.0f, drawRect.w - 28.0f, drawRect.h - 8.0f }, 0.62f, 0.38f, FONT_MENU, focused ? theme.buttonTextActive : theme.buttonText, MXUI_TEXT_CENTER, true, 2);

    if (mxui_component_widget_click(rect, focused, hovered)) {
        mxui_component_note_button_press(rect);
        mxui_component_play_click_sound(danger);
        return true;
    }
    return false;
}

bool mxui_component_toggle(struct MxuiRect rect, const char* label, bool* value) {
    struct MxuiTheme theme = mxui_theme();
    bool hovered = false;
    bool focused = mxui_focusable(rect, &hovered);
    struct MxuiComponentSlots slots = mxui_component_widget_slots(rect, label, 42.0f, 76.0f);
    mxui_skin_draw_panel(rect, theme.panelAlt, focused ? theme.buttonHover : theme.border, focused ? theme.glow : mxui_color(0, 0, 0, 0), 10.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(label, slots.labelRect, 0.78f, 0.58f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, slots.stacked ? 2 : 1);

    struct MxuiRect box = {
        slots.controlRect.x + slots.controlRect.w - 28.0f,
        slots.controlRect.y + (slots.controlRect.h - 24.0f) * 0.5f,
        24.0f,
        24.0f
    };
    mxui_draw_rect(box, *value ? theme.success : mxui_color(24, 24, 24, 220));
    mxui_draw_outline(box, theme.border, 1.0f);

    if (mxui_component_widget_click(rect, focused, hovered) || (focused && (sMxui.input.left || sMxui.input.right))) {
        *value = !*value;
        mxui_component_play_click_sound(false);
        return true;
    }
    return false;
}

bool mxui_component_select_u32(struct MxuiRect rect, const char* label, const char* const* choices, s32 count, unsigned int* value) {
    struct MxuiTheme theme = mxui_theme();
    bool hovered = false;
    bool focused = mxui_focusable(rect, &hovered);
    struct MxuiComponentSlots slots = mxui_component_widget_slots(rect, label, 212.0f, 300.0f);
    mxui_skin_draw_panel(rect, theme.panelAlt, focused ? theme.buttonHover : theme.border, focused ? theme.glow : mxui_color(0, 0, 0, 0), 10.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(label, slots.labelRect, 0.78f, 0.58f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, slots.stacked ? 2 : 1);

    if (*value >= (u32)count) {
        *value = 0;
    }

    f32 arrowWidth = mxui_clampf(slots.controlRect.h, 28.0f, 38.0f);
    struct MxuiRect left = { slots.controlRect.x, slots.controlRect.y, arrowWidth, slots.controlRect.h };
    struct MxuiRect right = { slots.controlRect.x + slots.controlRect.w - arrowWidth, slots.controlRect.y, arrowWidth, slots.controlRect.h };
    struct MxuiRect center = { left.x + left.w + 8.0f, slots.controlRect.y, MAX(60.0f, slots.controlRect.w - left.w - right.w - 16.0f), slots.controlRect.h };
    mxui_skin_draw_panel(left, theme.button, theme.border, mxui_color(0, 0, 0, 0), 8.0f, 1.0f);
    mxui_skin_draw_panel(right, theme.button, theme.border, mxui_color(0, 0, 0, 0), 8.0f, 1.0f);
    mxui_skin_draw_panel(center, mxui_color(26, 38, 58, 225), theme.border, mxui_color(0, 0, 0, 0), 8.0f, 1.0f);
    mxui_draw_text_box_fitted("<", left, 0.62f, 0.46f, FONT_MENU, theme.buttonText, MXUI_TEXT_CENTER, false, 1);
    mxui_draw_text_box_fitted(">", right, 0.62f, 0.46f, FONT_MENU, theme.buttonText, MXUI_TEXT_CENTER, false, 1);
    mxui_draw_text_box_fitted(choices[*value], center, 0.60f, 0.44f, FONT_MENU, theme.text, MXUI_TEXT_CENTER, false, 1);

    if (focused && sMxui.input.left) {
        *value = (*value == 0) ? (unsigned int)(count - 1) : (*value - 1);
        mxui_component_play_click_sound(false);
        return true;
    }
    if (focused && sMxui.input.right) {
        *value = (*value + 1) % count;
        mxui_component_play_click_sound(false);
        return true;
    }
    if (mxui_rect_contains(left, sMxui.input.mouseX, sMxui.input.mouseY) && mxui_component_consume_mouse_press()) {
        *value = (*value == 0) ? (unsigned int)(count - 1) : (*value - 1);
        mxui_component_play_click_sound(false);
        return true;
    }
    if (mxui_rect_contains(right, sMxui.input.mouseX, sMxui.input.mouseY) && mxui_component_consume_mouse_press()) {
        *value = (*value + 1) % count;
        mxui_component_play_click_sound(false);
        return true;
    }
    return false;
}

bool mxui_component_slider_u32(struct MxuiRect rect, const char* label, unsigned int* value, unsigned int minValue, unsigned int maxValue, unsigned int step) {
    struct MxuiTheme theme = mxui_theme();
    bool hovered = false;
    bool focused = mxui_focusable(rect, &hovered);
    struct MxuiComponentSlots slots = mxui_component_widget_slots(rect, label, 220.0f, 320.0f);
    mxui_skin_draw_panel(rect, theme.panelAlt, focused ? theme.buttonHover : theme.border, focused ? theme.glow : mxui_color(0, 0, 0, 0), 10.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(label, slots.labelRect, 0.78f, 0.58f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, slots.stacked ? 2 : 1);

    f32 valueBoxW = mxui_clampf(slots.controlRect.w * 0.24f, 52.0f, 76.0f);
    struct MxuiRect valueRect = { slots.controlRect.x, slots.controlRect.y, valueBoxW, slots.controlRect.h };
    struct MxuiRect bar = { valueRect.x + valueRect.w + 10.0f, slots.controlRect.y, MAX(60.0f, slots.controlRect.w - valueRect.w - 10.0f), slots.controlRect.h };
    mxui_skin_draw_panel(bar, mxui_color(18, 18, 18, 255), theme.border, mxui_color(0, 0, 0, 0), 8.0f, 1.0f);
    f32 ratio = (maxValue > minValue) ? ((f32)(*value - minValue) / (f32)(maxValue - minValue)) : 0.0f;
    ratio = mxui_clampf(ratio, 0.0f, 1.0f);
    struct MxuiRect fill = { bar.x + 2, bar.y + 2, (bar.w - 4) * ratio, bar.h - 4 };
    mxui_skin_draw_panel(fill, theme.button, theme.border, mxui_color(0, 0, 0, 0), 6.0f, 0.0f);

    char valueText[32] = { 0 };
    snprintf(valueText, sizeof(valueText), "%u", *value);
    mxui_skin_draw_panel(valueRect, mxui_color(26, 38, 58, 225), theme.border, mxui_color(0, 0, 0, 0), 8.0f, 1.0f);
    mxui_draw_text_box_fitted(valueText, valueRect, 0.60f, 0.46f, FONT_MENU, theme.textDim, MXUI_TEXT_CENTER, false, 1);

    if (focused && sMxui.input.left && *value > minValue) {
        *value = (*value > minValue + step) ? (*value - step) : minValue;
        mxui_component_play_click_sound(false);
        return true;
    }
    if (focused && sMxui.input.right && *value < maxValue) {
        *value = MIN(maxValue, *value + step);
        mxui_component_play_click_sound(false);
        return true;
    }
    if (mxui_rect_contains(bar, sMxui.input.mouseX, sMxui.input.mouseY) && mxui_component_consume_mouse_press()) {
        f32 pick = (sMxui.input.mouseX - bar.x) / MAX(1.0f, bar.w);
        *value = minValue + (unsigned int)roundf((maxValue - minValue) * mxui_clampf(pick, 0.0f, 1.0f));
        mxui_component_play_click_sound(false);
        return true;
    }
    return false;
}

bool mxui_component_bind_button(struct MxuiRect rect, const char* text, bool focused, bool hovered) {
    struct MxuiTheme theme = mxui_theme();
    struct MxuiColor fill = hovered ? theme.buttonHover : (focused ? theme.buttonActive : theme.button);
    struct MxuiColor glow = focused ? theme.glow : (hovered ? mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, theme.glow.a / 2) : mxui_color(0, 0, 0, 0));
    bool pressed = sMxui.pressedRectValid && sMxui.pressedFrames > 0 && mxui_component_rect_matches(rect, sMxui.pressedRect);
    struct MxuiRect drawRect = rect;
    if (pressed) {
        drawRect.x += 1.5f;
        drawRect.y += 1.5f;
        drawRect.w -= 3.0f;
        drawRect.h -= 3.0f;
    }
    mxui_skin_draw_panel(drawRect, fill, theme.border, glow, 8.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(text, (struct MxuiRect){ drawRect.x + 6.0f, drawRect.y + 3.0f, drawRect.w - 12.0f, drawRect.h - 6.0f }, 0.48f, 0.34f, FONT_MENU, focused ? theme.buttonTextActive : theme.buttonText, MXUI_TEXT_CENTER, true, 2);
    if (mxui_component_widget_click(rect, focused, hovered)) {
        mxui_component_note_button_press(rect);
        mxui_component_play_click_sound(false);
        return true;
    }
    return false;
}

bool mxui_component_bind_row(struct MxuiRect rect, const char* label, unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS], struct Mod* mod, const char* bindId, s32 hookIndex) {
    struct MxuiTheme theme = mxui_theme();
    struct MxuiComponentSlots slots = mxui_component_widget_slots(rect, label, 340.0f, 420.0f);
    mxui_skin_draw_panel(rect, theme.panelAlt, theme.border, mxui_color(0, 0, 0, 0), 10.0f, 1.0f);
    mxui_draw_text_box_fitted(label, slots.labelRect, 0.76f, 0.56f, FONT_NORMAL, theme.text, MXUI_TEXT_LEFT, true, slots.stacked ? 2 : 1);

    bool changed = false;
    f32 spacing = 8.0f;
    f32 resetWidth = mxui_clampf(slots.controlRect.w * 0.18f, 64.0f, 82.0f);
    f32 slotWidth = MAX(52.0f, (slots.controlRect.w - resetWidth - spacing * (MAX_BINDS)) / (f32)MAX_BINDS);
    f32 startX = slots.controlRect.x;
    for (s32 i = 0; i < MAX_BINDS; i++) {
        struct MxuiRect slotRect = { startX + i * (slotWidth + spacing), slots.controlRect.y, slotWidth, slots.controlRect.h };
        char text[32] = { 0 };
        if (sMxui.capturedBind == bindValue && sMxui.capturedBindSlot == i) {
            snprintf(text, sizeof(text), "...");
        } else if (bindValue[i] == VK_INVALID) {
            snprintf(text, sizeof(text), "---");
        } else {
            snprintf(text, sizeof(text), "%s", mxui_bind_name(bindValue[i]));
        }

        bool hovered = false;
        bool focused = mxui_focusable(slotRect, &hovered);
        if (mxui_component_bind_button(slotRect, text, focused, hovered)) {
            controller_get_raw_key();
            sMxui.capturedBind = bindValue;
            memcpy(sMxui.capturedDefaultBind, defaultValue, sizeof(sMxui.capturedDefaultBind));
            sMxui.capturedBindMod = mod;
            sMxui.capturedHookIndex = hookIndex;
            sMxui.capturedBindSlot = i;
            snprintf(sMxui.capturedBindId, sizeof(sMxui.capturedBindId), "%s", bindId != NULL ? bindId : "");
        }
    }

    struct MxuiRect resetRect = {
        slots.controlRect.x + slots.controlRect.w - resetWidth,
        slots.controlRect.y,
        resetWidth,
        slots.controlRect.h
    };
    bool resetHovered = false;
    bool resetFocused = mxui_focusable(resetRect, &resetHovered);
    if (mxui_component_bind_button(resetRect, "Reset", resetFocused, resetHovered)) {
        mxui_restore_bind_defaults(bindValue, defaultValue);
        controller_reconfigure();
        if (mod != NULL && bindId != NULL && bindId[0] != '\0') {
            mod_bindings_set(mod->relativePath, bindId, bindValue);
        }
        if (hookIndex >= 0 && hookIndex < gHookedModMenuElementsCount) {
            smlua_call_mod_menu_element_hook(&gHookedModMenuElements[hookIndex], hookIndex);
        }
        changed = true;
    }
    return changed;
}

void mxui_component_footer_button(struct MxuiRect footer, bool right, const char* label, bool* clicked) {
    struct MxuiRect rect = right
        ? (struct MxuiRect){ footer.x + footer.w - 232, footer.y + 2.0f, 232, footer.h - 4.0f }
        : (struct MxuiRect){ footer.x, footer.y + 2.0f, 232, footer.h - 4.0f };
    mxui_render_reset_texture_clipping();
    struct MxuiTheme theme = mxui_theme();
    bool hovered = false;
    bool focused = mxui_focusable(rect, &hovered);
    bool pressed = sMxui.pressedRectValid && sMxui.pressedFrames > 0 && mxui_component_rect_matches(rect, sMxui.pressedRect);
    struct MxuiColor fill = focused ? theme.buttonActive : theme.button;
    if (hovered) {
        fill = theme.buttonHover;
    }
    if (pressed) {
        fill = mxui_color(
            MIN(fill.r + 12, 255),
            MIN(fill.g + 12, 255),
            MIN(fill.b + 12, 255),
            fill.a
        );
    }

    struct MxuiRect drawRect = rect;
    if (pressed) {
        drawRect.x += 1.5f;
        drawRect.y += 1.5f;
        drawRect.w -= 3.0f;
        drawRect.h -= 3.0f;
    }
    mxui_skin_draw_panel(drawRect, fill, theme.border, focused ? theme.glow : mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, hovered ? theme.glow.a / 2 : 0), 11.0f, focused ? 2.0f : 1.0f);
    mxui_draw_text_box_fitted(label, (struct MxuiRect){ drawRect.x + 12.0f, drawRect.y + 4.0f, drawRect.w - 24.0f, drawRect.h - 8.0f }, 0.58f, 0.34f, FONT_MENU, focused ? theme.buttonTextActive : theme.buttonText, MXUI_TEXT_CENTER, true, 1);
    bool activated = mxui_component_widget_click(rect, focused, hovered);
    if (activated) {
        mxui_component_note_button_press(rect);
        mxui_component_play_click_sound(false);
    }
    if (clicked != NULL) {
        *clicked = activated;
    }
}

void mxui_component_footer_center_text(struct MxuiRect footer, const char* text) {
    struct MxuiTheme theme = mxui_theme();
    struct MxuiRect textRect = footer;
    if (footer.w > 540.0f) {
        textRect.x += 248.0f;
        textRect.w -= 496.0f;
    }
    mxui_render_reset_texture_clipping();
    mxui_draw_text_box_fitted(text, textRect, 0.58f, 0.36f, FONT_MENU, theme.textDim, MXUI_TEXT_CENTER, false, 1);
}

void mxui_component_render_confirm(void) {
    if (!sMxui.confirmOpen) {
        return;
    }

    struct MxuiTheme theme = mxui_theme();
    f32 anim = mxui_clampf(sMxui.confirmAnim, 0.0f, 1.0f);
    f32 scale = 0.94f + anim * 0.06f;
    struct MxuiRect modal = {
        (mxui_render_screen_width() - 520.0f) * 0.5f,
        (mxui_render_screen_height() - 240.0f) * 0.5f,
        520.0f,
        240.0f
    };
    modal = mxui_layout_centered(modal, modal.w * scale, modal.h * scale);
    mxui_draw_rect((struct MxuiRect){ 0, 0, (f32)mxui_render_screen_width(), (f32)mxui_render_screen_height() }, mxui_color(0, 0, 0, (u8)(150.0f * anim)));
    mxui_skin_draw_panel(
        (struct MxuiRect){ modal.x + 12, modal.y + 12, modal.w - 24, modal.h - 24 },
        mxui_color(theme.shell.r, theme.shell.g, theme.shell.b, (u8)(theme.shell.a * anim)),
        mxui_color(theme.border.r, theme.border.g, theme.border.b, (u8)(theme.border.a * anim)),
        mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, (u8)(theme.glow.a * anim)),
        16.0f,
        2.0f
    );
    mxui_draw_text_box_fitted(sMxui.confirmTitle, (struct MxuiRect){ modal.x + 40, modal.y + 28, modal.w - 80, 34 }, 0.76f, 0.46f, FONT_MENU, theme.title, MXUI_TEXT_CENTER, false, 1);
    mxui_draw_text_box_fitted(sMxui.confirmMessage, (struct MxuiRect){ modal.x + 42, modal.y + 90, modal.w - 84, 60 }, 0.54f, 0.38f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 3);
    sMxui.renderingModal = true;
    if (sMxui.input.mouseReleased && !mxui_rect_contains(modal, sMxui.input.mouseX, sMxui.input.mouseY)) {
        sMxui.confirmOpen = false;
        sMxui.confirmTarget = 0.0f;
        mxui_component_consume_release_and_back();
        sMxui.ignoreAcceptUntilRelease = true;
        sMxui.ignoreMenuToggleUntilRelease = true;
        sMxui.renderingModal = false;
        return;
    }
    if (mxui_component_button((struct MxuiRect){ modal.x + 40, modal.y + 170, 180, 42 }, "No", false) || sMxui.input.back) {
        sMxui.confirmOpen = false;
        sMxui.confirmTarget = 0.0f;
        mxui_component_consume_release_and_back();
        sMxui.ignoreAcceptUntilRelease = true;
        sMxui.ignoreMenuToggleUntilRelease = true;
    }
    if (mxui_component_button((struct MxuiRect){ modal.x + modal.w - 220, modal.y + 170, 180, 42 }, "Yes", false)) {
        MxuiActionCallback confirmYes = sMxui.confirmYes;
        sMxui.confirmOpen = false;
        sMxui.confirmTarget = 0.0f;
        mxui_component_consume_release_and_back();
        sMxui.ignoreAcceptUntilRelease = true;
        sMxui.ignoreMenuToggleUntilRelease = true;
        if (confirmYes != NULL) {
            confirmYes();
        }
    }
    sMxui.renderingModal = false;
}
