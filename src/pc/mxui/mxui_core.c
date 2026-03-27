#include <math.h>
#include <string.h>

#include "sm64.h"
#include "mxui_internal.h"
#include "mxui_components.h"
#include "mxui_render.h"

#include "pc/configfile.h"
#include "pc/controller/controller_api.h"
#include "pc/controller/controller_bind_mapping.h"
#include "pc/controller/controller_keyboard.h"
#include "pc/lua/smlua_hooks.h"
#include "pc/mods/mod_bindings.h"

#include "audio/external.h"
#include "engine/math_util.h"
#include "game/game_init.h"
#include "sounds.h"

f32 mxui_ui_scale(void) {
    static const f32 sScaleChoices[] = { 1.0f, 0.5f, 0.85f, 1.0f, 1.5f };
    u32 choice = configDjuiScale;
    if (choice >= (sizeof(sScaleChoices) / sizeof(sScaleChoices[0]))) {
        choice = 0;
    }
    return sScaleChoices[choice];
}

f32 mxui_clampf(f32 value, f32 minValue, f32 maxValue) {
    if (value < minValue) { return minValue; }
    if (value > maxValue) { return maxValue; }
    return value;
}

struct MxuiColor mxui_color(u8 r, u8 g, u8 b, u8 a) {
    struct MxuiColor color = { r, g, b, a };
    return color;
}

static bool mxui_rect_matches(struct MxuiRect a, struct MxuiRect b) {
    return fabsf(a.x - b.x) < 1.0f
        && fabsf(a.y - b.y) < 1.0f
        && fabsf(a.w - b.w) < 1.0f
        && fabsf(a.h - b.h) < 1.0f;
}

static void mxui_note_button_press(struct MxuiRect rect) {
    sMxui.pressedRect = rect;
    sMxui.pressedRectValid = true;
    sMxui.pressedFrames = 6;
}

static struct MxuiColor mxui_theme_color(u8 r, u8 g, u8 b, u8 a) {
    return mxui_color(r, g, b, a);
}

struct MxuiTheme mxui_theme(void) {
    struct MxuiTheme theme = {
        .overlay = mxui_theme_color(4, 6, 14, 152),
        .shell = mxui_theme_color(28, 58, 92, 236),
        .panel = mxui_theme_color(35, 86, 124, 226),
        .panelAlt = mxui_theme_color(56, 108, 146, 232),
        .border = mxui_theme_color(255, 221, 118, 255),
        .text = mxui_theme_color(252, 248, 236, 255),
        .textDim = mxui_theme_color(222, 233, 238, 255),
        .title = mxui_theme_color(255, 233, 134, 255),
        .button = mxui_theme_color(101, 176, 106, 255),
        .buttonHover = mxui_theme_color(126, 204, 130, 255),
        .buttonActive = mxui_theme_color(255, 237, 138, 255),
        .buttonText = mxui_theme_color(251, 249, 242, 255),
        .buttonTextActive = mxui_theme_color(250, 247, 236, 255),
        .danger = mxui_theme_color(198, 74, 72, 255),
        .success = mxui_theme_color(130, 218, 148, 255),
        .glow = mxui_theme_color(124, 210, 255, 96),
        .shadow = mxui_theme_color(0, 0, 0, 156),
    };

    if (configDjuiTheme == 1) {
        theme.overlay = mxui_theme_color(6, 10, 18, 156);
        theme.shell = mxui_theme_color(44, 70, 118, 236);
        theme.panel = mxui_theme_color(62, 98, 156, 228);
        theme.panelAlt = mxui_theme_color(84, 124, 180, 232);
        theme.border = mxui_theme_color(244, 250, 255, 255);
        theme.title = mxui_theme_color(255, 255, 255, 255);
        theme.button = mxui_theme_color(152, 214, 255, 255);
        theme.buttonHover = mxui_theme_color(194, 232, 255, 255);
        theme.buttonActive = mxui_theme_color(255, 255, 255, 255);
        theme.buttonText = mxui_theme_color(242, 248, 255, 255);
        theme.buttonTextActive = mxui_theme_color(242, 248, 255, 255);
        theme.glow = mxui_theme_color(214, 238, 255, 110);
        theme.shadow = mxui_theme_color(6, 14, 28, 150);
    } else if (configDjuiTheme == 2) {
        theme.overlay = mxui_theme_color(0, 12, 16, 156);
        theme.shell = mxui_theme_color(16, 56, 80, 236);
        theme.panel = mxui_theme_color(26, 84, 112, 228);
        theme.panelAlt = mxui_theme_color(36, 106, 138, 232);
        theme.border = mxui_theme_color(162, 246, 236, 255);
        theme.title = mxui_theme_color(212, 255, 246, 255);
        theme.button = mxui_theme_color(76, 178, 188, 255);
        theme.buttonHover = mxui_theme_color(112, 214, 220, 255);
        theme.buttonActive = mxui_theme_color(202, 252, 246, 255);
        theme.buttonText = mxui_theme_color(242, 252, 248, 255);
        theme.buttonTextActive = mxui_theme_color(242, 252, 248, 255);
        theme.glow = mxui_theme_color(104, 236, 226, 96);
        theme.shadow = mxui_theme_color(0, 12, 18, 150);
    } else if (configDjuiTheme == 3) {
        theme.overlay = mxui_theme_color(8, 4, 18, 160);
        theme.shell = mxui_theme_color(42, 32, 74, 236);
        theme.panel = mxui_theme_color(66, 52, 108, 228);
        theme.panelAlt = mxui_theme_color(88, 72, 130, 232);
        theme.border = mxui_theme_color(232, 220, 255, 255);
        theme.title = mxui_theme_color(252, 242, 255, 255);
        theme.button = mxui_theme_color(146, 124, 210, 255);
        theme.buttonHover = mxui_theme_color(178, 152, 234, 255);
        theme.buttonActive = mxui_theme_color(235, 222, 255, 255);
        theme.buttonText = mxui_theme_color(248, 244, 255, 255);
        theme.buttonTextActive = mxui_theme_color(248, 244, 255, 255);
        theme.danger = mxui_theme_color(192, 78, 104, 255);
        theme.glow = mxui_theme_color(174, 156, 255, 104);
        theme.shadow = mxui_theme_color(2, 0, 12, 164);
    } else if (configDjuiTheme == 4) {
        theme.overlay = mxui_theme_color(18, 4, 0, 162);
        theme.shell = mxui_theme_color(70, 28, 20, 238);
        theme.panel = mxui_theme_color(106, 38, 24, 230);
        theme.panelAlt = mxui_theme_color(136, 52, 28, 234);
        theme.border = mxui_theme_color(255, 202, 98, 255);
        theme.title = mxui_theme_color(255, 228, 126, 255);
        theme.button = mxui_theme_color(232, 114, 48, 255);
        theme.buttonHover = mxui_theme_color(252, 142, 64, 255);
        theme.buttonActive = mxui_theme_color(255, 220, 116, 255);
        theme.buttonText = mxui_theme_color(255, 246, 232, 255);
        theme.buttonTextActive = mxui_theme_color(255, 246, 232, 255);
        theme.danger = mxui_theme_color(202, 52, 48, 255);
        theme.glow = mxui_theme_color(255, 128, 48, 110);
        theme.shadow = mxui_theme_color(18, 4, 2, 160);
    } else if (configDjuiTheme == 5) {
        theme.overlay = mxui_theme_color(8, 12, 20, 158);
        theme.shell = mxui_theme_color(44, 58, 80, 238);
        theme.panel = mxui_theme_color(68, 92, 122, 230);
        theme.panelAlt = mxui_theme_color(92, 116, 148, 234);
        theme.border = mxui_theme_color(230, 240, 250, 255);
        theme.title = mxui_theme_color(255, 244, 188, 255);
        theme.text = mxui_theme_color(250, 248, 242, 255);
        theme.textDim = mxui_theme_color(224, 232, 238, 255);
        theme.button = mxui_theme_color(124, 184, 128, 255);
        theme.buttonHover = mxui_theme_color(148, 208, 150, 255);
        theme.buttonActive = mxui_theme_color(248, 236, 170, 255);
        theme.buttonText = mxui_theme_color(250, 248, 242, 255);
        theme.buttonTextActive = mxui_theme_color(255, 252, 236, 255);
        theme.glow = mxui_theme_color(178, 214, 255, 100);
        theme.shadow = mxui_theme_color(8, 12, 20, 160);
    } else if (configDjuiTheme == 6) {
        theme.overlay = mxui_theme_color(20, 10, 2, 156);
        theme.shell = mxui_theme_color(92, 58, 22, 238);
        theme.panel = mxui_theme_color(124, 76, 28, 230);
        theme.panelAlt = mxui_theme_color(154, 98, 34, 234);
        theme.border = mxui_theme_color(255, 222, 132, 255);
        theme.title = mxui_theme_color(255, 240, 176, 255);
        theme.text = mxui_theme_color(255, 247, 230, 255);
        theme.textDim = mxui_theme_color(248, 224, 182, 255);
        theme.button = mxui_theme_color(72, 170, 152, 255);
        theme.buttonHover = mxui_theme_color(94, 194, 176, 255);
        theme.buttonActive = mxui_theme_color(255, 236, 156, 255);
        theme.buttonText = mxui_theme_color(252, 248, 236, 255);
        theme.buttonTextActive = mxui_theme_color(255, 252, 242, 255);
        theme.danger = mxui_theme_color(210, 82, 58, 255);
        theme.glow = mxui_theme_color(255, 210, 112, 108);
        theme.shadow = mxui_theme_color(18, 8, 0, 162);
    }

    return theme;
}

struct MxuiScreenState* mxui_current(void) {
    if (sMxui.depth <= 0) { return NULL; }
    return &sMxui.stack[sMxui.depth - 1];
}

void mxui_finish_bind_capture(void) {
    sMxui.capturedBind = NULL;
    sMxui.capturedBindMod = NULL;
    sMxui.capturedHookIndex = -1;
    sMxui.capturedBindSlot = -1;
    sMxui.capturedBindId[0] = '\0';
}

void mxui_reset_screen(struct MxuiScreenState* screen, enum MxuiScreenId screenId, s32 tag) {
    memset(screen, 0, sizeof(*screen));
    screen->id = screenId;
    screen->tag = tag;
    screen->lastFocusIndex = -1;
}

void mxui_push_if_possible(enum MxuiScreenId screenId, s32 tag) {
    if (sMxui.depth >= MXUI_STACK_MAX) { return; }
    mxui_reset_screen(&sMxui.stack[sMxui.depth], screenId, tag);
    sMxui.depth++;
    sMxui.active = true;
}

void mxui_set_color(struct MxuiColor color) {
    mxui_render_set_color(color.r, color.g, color.b, color.a);
}

void mxui_draw_rect(struct MxuiRect rect, struct MxuiColor color) {
    mxui_set_color(color);
    mxui_render_rect(rect.x, rect.y, rect.w, rect.h);
}

void mxui_draw_outline(struct MxuiRect rect, struct MxuiColor color, f32 thickness) {
    mxui_draw_rect((struct MxuiRect){ rect.x, rect.y, rect.w, thickness }, color);
    mxui_draw_rect((struct MxuiRect){ rect.x, rect.y + rect.h - thickness, rect.w, thickness }, color);
    mxui_draw_rect((struct MxuiRect){ rect.x, rect.y, thickness, rect.h }, color);
    mxui_draw_rect((struct MxuiRect){ rect.x + rect.w - thickness, rect.y, thickness, rect.h }, color);
}

void mxui_toast(const char* message, s32 frames) {
    snprintf(sMxui.toast, sizeof(sMxui.toast), "%s", message);
    sMxui.toastTimer = frames;
}

bool mxui_rect_contains(struct MxuiRect rect, f32 x, f32 y) {
    return x >= rect.x && y >= rect.y && x <= rect.x + rect.w && y <= rect.y + rect.h;
}

void mxui_clamp_focus(struct MxuiScreenState* screen) {
    if (screen == NULL) { return; }
    if (screen->focusCount <= 0) {
        screen->focusIndex = 0;
        return;
    }
    if (screen->focusIndex < 0) {
        screen->focusIndex = screen->focusCount - 1;
    } else if (screen->focusIndex >= screen->focusCount) {
        screen->focusIndex = 0;
    }
}

static void mxui_play_focus_sound(void) {
    play_sound(SOUND_MENU_CHANGE_SELECT, gGlobalSoundSource);
}

static void mxui_play_click_sound(bool danger) {
    play_sound(danger ? SOUND_MENU_CAMERA_BUZZ : SOUND_MENU_CLICK_FILE_SELECT, gGlobalSoundSource);
}

static bool mxui_consume_mouse_press(void) {
    if (!sMxui.input.mousePressed) {
        return false;
    }
    sMxui.input.mousePressed = false;
    return true;
}

static bool mxui_consume_accept_press(void) {
    if (!sMxui.input.accept) {
        return false;
    }
    sMxui.input.accept = false;
    return true;
}

static void mxui_consume_release_and_back(void) {
    sMxui.input.mouseReleased = false;
    sMxui.input.accept = false;
    sMxui.input.back = false;
    sMxui.mouseCaptureValid = false;
    sMxui.pressedRectValid = false;
}

struct MxuiWidgetSlots {
    struct MxuiRect labelRect;
    struct MxuiRect controlRect;
    bool stacked;
};

static struct MxuiWidgetSlots mxui_widget_slots(struct MxuiRect rect, const char* label, f32 minControlWidth, f32 maxControlWidth) {
    const f32 padX = 14.0f;
    const f32 gap = 12.0f;
    const f32 available = MAX(0.0f, rect.w - padX * 2.0f);
    const f32 labelIdeal = mxui_measure(label != NULL ? label : "", FONT_NORMAL, 0.56f) + 16.0f;
    f32 controlWidth = mxui_clampf(MAX(minControlWidth, available * 0.42f), minControlWidth, MIN(maxControlWidth, available - 96.0f));
    if (controlWidth > available - 72.0f) {
        controlWidth = MAX(minControlWidth * 0.78f, available * 0.46f);
    }

    f32 labelWidth = MAX(84.0f, available - controlWidth - gap);
    bool stacked = labelIdeal > labelWidth && rect.h >= 72.0f;

    if (stacked) {
        f32 labelHeight = mxui_row_height_for_text(label, 28.0f, 0.42f, 0.56f, FONT_NORMAL, available, true, 2);
        return (struct MxuiWidgetSlots) {
            .labelRect = { rect.x + padX, rect.y + 8.0f, available, labelHeight },
            .controlRect = { rect.x + padX, rect.y + 8.0f + labelHeight + 8.0f, available, rect.h - (24.0f + labelHeight) },
            .stacked = true,
        };
    }

    return (struct MxuiWidgetSlots) {
        .labelRect = { rect.x + padX, rect.y + 4.0f, labelWidth, rect.h - 8.0f },
        .controlRect = { rect.x + rect.w - padX - controlWidth, rect.y + 6.0f, controlWidth, rect.h - 12.0f },
        .stacked = false,
    };
}

void mxui_apply_input(void) {
    struct MxuiInput* input = &sMxui.input;
    memset(input, 0, sizeof(*input));

    if (sMxui.pressedFrames > 0) {
        sMxui.pressedFrames--;
        if (sMxui.pressedFrames == 0) {
            sMxui.pressedRectValid = false;
        }
    }

    input->mouseX = mxui_render_mouse_x();
    input->mouseY = mxui_render_mouse_y();
    input->mouseScroll = mxui_render_mouse_scroll_y();
    input->mouseDown = mxui_render_mouse_buttons_down() != 0;
    input->mousePressed = input->mouseDown && !sMxui.prevMouseDown;
    input->mouseReleased = !input->mouseDown && sMxui.prevMouseDown;
    sMxui.prevMouseDown = input->mouseDown;
    bool mouseUiInteraction = input->mouseDown || input->mousePressed || input->mouseReleased;

    bool acceptPressed = mxui_input_accept_pressed();
    bool acceptDown = mxui_input_accept_down();
    bool menuTogglePressed = mxui_input_menu_toggle_pressed();
    bool menuToggleDown = mxui_input_menu_toggle_down();

    if (sMxui.ignoreAcceptUntilRelease) {
        if (!acceptDown) {
            sMxui.ignoreAcceptUntilRelease = false;
        }
        acceptPressed = false;
    }
    if (sMxui.ignoreMenuToggleUntilRelease) {
        if (!menuToggleDown) {
            sMxui.ignoreMenuToggleUntilRelease = false;
        }
        menuTogglePressed = false;
    }

    if (mouseUiInteraction) {
        acceptPressed = false;
        acceptDown = false;
        menuTogglePressed = false;
        menuToggleDown = false;
    }

    input->menuToggle = menuTogglePressed;
    input->accept = acceptPressed;
    input->back = mouseUiInteraction ? false : mxui_input_back_pressed();
    input->up = controller_key_pressed(SCANCODE_HOME) || (gPlayer1Controller->buttonPressed & U_JPAD);
    input->down = controller_key_pressed(SCANCODE_END) || (gPlayer1Controller->buttonPressed & D_JPAD);
    input->left = controller_key_pressed(SCANCODE_LEFT) || (gPlayer1Controller->buttonPressed & (L_JPAD | L_CBUTTONS));
    input->right = controller_key_pressed(SCANCODE_RIGHT) || (gPlayer1Controller->buttonPressed & (R_JPAD | R_CBUTTONS));
    input->prevPage = mxui_input_prev_page_pressed();
    input->nextPage = mxui_input_next_page_pressed();
    sMxui.focusMovedByNav = false;
}

bool mxui_focusable(struct MxuiRect rect, bool* hoveredOut) {
    if (sMxui.confirmOpen && !sMxui.renderingModal) {
        if (hoveredOut != NULL) {
            *hoveredOut = false;
        }
        return false;
    }

    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL) {
        if (hoveredOut != NULL) {
            *hoveredOut = false;
        }
        return false;
    }
    if (!sMxui.renderingModal && !sMxui.renderingFooter && sMxui.contentClipValid) {
        struct MxuiRect clip = sMxui.contentClipRect;
        if (rect.x + rect.w < clip.x || rect.x > clip.x + clip.w
            || rect.y + rect.h < clip.y || rect.y > clip.y + clip.h) {
            if (hoveredOut != NULL) {
                *hoveredOut = false;
            }
            return false;
        }
    }

    s32 index = sMxui.nextFocusIndex++;
    bool hovered = mxui_rect_contains(rect, sMxui.input.mouseX, sMxui.input.mouseY);
    if (hoveredOut != NULL) {
        *hoveredOut = hovered;
    }

    if (hovered) {
        screen->focusIndex = index;
        sMxui.focusedRect = rect;
        sMxui.focusedRectValid = true;
    } else if (screen->focusIndex == index) {
        sMxui.focusedRect = rect;
        sMxui.focusedRectValid = true;
    }

    return screen->focusIndex == index;
}

bool mxui_widget_click(struct MxuiRect rect, bool focused, bool hovered) {
    if (hovered && sMxui.input.mousePressed) {
        sMxui.mouseCaptureRect = rect;
        sMxui.mouseCaptureValid = true;
        mxui_consume_mouse_press();
        return false;
    }
    if (hovered && sMxui.input.mouseReleased && sMxui.mouseCaptureValid && mxui_rect_matches(rect, sMxui.mouseCaptureRect)) {
        sMxui.mouseCaptureValid = false;
        return true;
    }
    if (focused && mxui_consume_accept_press()) {
        return true;
    }
    (void)rect;
    return false;
}

void mxui_sync_focus_feedback(void) {
    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL || screen->focusCount <= 0) {
        return;
    }
    if (screen->focusIndex != screen->lastFocusIndex) {
        if (screen->lastFocusIndex >= 0) {
            mxui_play_focus_sound();
        }
        screen->lastFocusIndex = screen->focusIndex;
    }
}

bool mxui_bind_overlap(const unsigned int a[MAX_BINDS], const unsigned int b[MAX_BINDS]) {
    for (s32 i = 0; i < MAX_BINDS; i++) {
        if (a[i] == VK_INVALID) { continue; }
        for (s32 j = 0; j < MAX_BINDS; j++) {
            if (a[i] == b[j]) {
                return true;
            }
        }
    }
    return false;
}

const char* mxui_bind_name(unsigned int bind) {
#if defined(CAPI_SDL1) || defined(CAPI_SDL2)
    return translate_bind_to_name(bind);
#else
    UNUSED(bind);
    return "---";
#endif
}

void mxui_restore_bind_defaults(unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS]) {
    for (s32 i = 0; i < MAX_BINDS; i++) {
        bindValue[i] = defaultValue[i];
    }
}

bool mxui_button(struct MxuiRect rect, const char* label, bool danger) {
    return mxui_component_button(rect, label, danger);
}

bool mxui_toggle(struct MxuiRect rect, const char* label, bool* value) {
    return mxui_component_toggle(rect, label, value);
}

bool mxui_select_u32(struct MxuiRect rect, const char* label, const char* const* choices, s32 count, unsigned int* value) {
    return mxui_component_select_u32(rect, label, choices, count, value);
}

bool mxui_slider_u32(struct MxuiRect rect, const char* label, unsigned int* value, unsigned int minValue, unsigned int maxValue, unsigned int step) {
    return mxui_component_slider_u32(rect, label, value, minValue, maxValue, step);
}

bool mxui_bind_button(struct MxuiRect rect, const char* text, bool focused, bool hovered) {
    return mxui_component_bind_button(rect, text, focused, hovered);
}

void mxui_capture_bind_if_needed(void) {
    if (sMxui.capturedBind == NULL) { return; }

    u32 key = controller_get_raw_key();
    if (key == VK_INVALID) {
        if (sMxui.input.back) {
            mxui_finish_bind_capture();
        }
        return;
    }

    for (s32 i = 0; i < MAX_BINDS; i++) {
        if (i == sMxui.capturedBindSlot) { continue; }
        if (sMxui.capturedBind[i] == key) {
            sMxui.capturedBind[i] = VK_INVALID;
        }
    }

    sMxui.capturedBind[sMxui.capturedBindSlot] = key;
    controller_reconfigure();

    if (sMxui.capturedBindMod != NULL && sMxui.capturedBindId[0] != '\0') {
        mod_bindings_set(sMxui.capturedBindMod->relativePath, sMxui.capturedBindId, sMxui.capturedBind);
    }
    if (sMxui.capturedHookIndex >= 0 && sMxui.capturedHookIndex < gHookedModMenuElementsCount) {
        smlua_call_mod_menu_element_hook(&gHookedModMenuElements[sMxui.capturedHookIndex], sMxui.capturedHookIndex);
    }

    mxui_finish_bind_capture();
}

bool mxui_bind_row(struct MxuiRect rect, const char* label, unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS], struct Mod* mod, const char* bindId, s32 hookIndex) {
    return mxui_component_bind_row(rect, label, bindValue, defaultValue, mod, bindId, hookIndex);
}

void mxui_handle_navigation(void) {
    struct MxuiScreenState* screen = mxui_current();
    if (screen == NULL || sMxui.confirmOpen) { return; }

    if (sMxui.capturedBind != NULL) {
        return;
    }

    if (sMxui.input.up) {
        screen->focusIndex--;
        sMxui.focusMovedByNav = true;
    } else if (sMxui.input.down) {
        screen->focusIndex++;
        sMxui.focusMovedByNav = true;
    }
    mxui_clamp_focus(screen);
}

void mxui_footer_button(struct MxuiRect footer, bool right, const char* label, bool* clicked) {
    mxui_component_footer_button(footer, right, label, clicked);
}

void mxui_footer_center_text(struct MxuiRect footer, const char* text) {
    mxui_component_footer_center_text(footer, text);
}

void mxui_render_confirm(void) {
    mxui_component_render_confirm();
}
