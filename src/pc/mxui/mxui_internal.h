#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <PR/ultratypes.h>

#include "mxui.h"
#include "mxui_font.h"

#include "pc/configfile.h"
#include "pc/mods/mod.h"

#define MXUI_STACK_MAX 16
#define MXUI_MAX_LANGUAGES 128
#define MXUI_PROFILE_COUNT 3

struct MxuiColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct MxuiRect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

struct MxuiInput {
    f32 mouseX;
    f32 mouseY;
    f32 mouseScroll;
    bool mouseDown;
    bool mousePressed;
    bool mouseReleased;
    bool menuToggle;
    bool accept;
    bool back;
    bool up;
    bool down;
    bool left;
    bool right;
    bool prevPage;
    bool nextPage;
};

struct MxuiScreenState {
    enum MxuiScreenId id;
    s32 tag;
    s32 subTag;
    f32 scroll;
    f32 auxScroll;
    s32 focusIndex;
    s32 focusCount;
    s32 lastFocusIndex;
    s32 page;
    s32 auxValue;
};

struct MxuiTheme {
    struct MxuiColor overlay;
    struct MxuiColor shell;
    struct MxuiColor panel;
    struct MxuiColor panelAlt;
    struct MxuiColor border;
    struct MxuiColor text;
    struct MxuiColor textDim;
    struct MxuiColor title;
    struct MxuiColor button;
    struct MxuiColor buttonHover;
    struct MxuiColor buttonActive;
    struct MxuiColor buttonText;
    struct MxuiColor buttonTextActive;
    struct MxuiColor danger;
    struct MxuiColor success;
    struct MxuiColor glow;
    struct MxuiColor shadow;
};

enum MxuiTextAlign {
    MXUI_TEXT_LEFT,
    MXUI_TEXT_CENTER,
    MXUI_TEXT_RIGHT,
};

enum MxuiScreenTemplateKind {
    MXUI_TEMPLATE_FRONT_PAGE,
    MXUI_TEMPLATE_SETTINGS_PAGE,
    MXUI_TEMPLATE_GRID_PAGE,
    MXUI_TEMPLATE_DETAIL_PAGE,
};

struct MxuiScreenConfig {
    enum MxuiScreenId id;
    const char* title;
    const char* subtitle;
    enum MxuiScreenTemplateKind templateKind;
    bool showBackFooter;
    const char* backLabel;
};

struct MxuiContext {
    const struct MxuiScreenConfig* config;
    struct MxuiRect shell;
    struct MxuiRect header;
    struct MxuiRect content;
    struct MxuiRect footer;
    f32 cursorY;
    f32 contentHeight;
};

struct MxuiSectionLayout {
    struct MxuiRect rect;
    struct MxuiRect body;
    f32 cursorY;
    f32 rowGap;
};

struct MxuiRowPair {
    struct MxuiRect left;
    struct MxuiRect right;
};

struct MxuiModList {
    s32 indices[512];
    s32 count;
    s32 pageCount;
};

enum MxuiDeferredActionType {
    MXUI_DEFERRED_NONE,
    MXUI_DEFERRED_OPEN_SCREEN,
    MXUI_DEFERRED_PUSH_SCREEN,
    MXUI_DEFERRED_POP_SCREEN,
    MXUI_DEFERRED_CLEAR,
    MXUI_DEFERRED_OPEN_CONFIRM,
    MXUI_DEFERRED_CLOSE_PAUSE,
    MXUI_DEFERRED_OPEN_CHAR_SELECT,
    MXUI_DEFERRED_OPEN_CHAR_SELECT_TAB,
};

struct MxuiState {
    bool initialized;
    bool active;
    bool mainMenu;
    bool pauseMenu;
    bool rendering;
    bool pauseClosing;
    bool prevMouseDown;
    bool focusMovedByNav;
    bool ignoreAcceptUntilRelease;
    bool ignoreMenuToggleUntilRelease;
    bool renderingModal;
    bool renderingFooter;
    f32 shellAnim;
    f32 shellTarget;
    f32 screenAnim;
    f32 screenTarget;
    f32 confirmAnim;
    f32 confirmTarget;
    struct MxuiInput input;
    struct MxuiScreenState stack[MXUI_STACK_MAX];
    s32 depth;
    s32 nextFocusIndex;
    struct MxuiRect focusedRect;
    bool focusedRectValid;
    struct MxuiRect contentClipRect;
    bool contentClipValid;
    struct MxuiRect mouseCaptureRect;
    bool mouseCaptureValid;
    struct MxuiRect pressedRect;
    bool pressedRectValid;
    s32 pressedFrames;
    bool wantsConfigSave;
    char toast[256];
    s32 toastTimer;
    bool confirmOpen;
    char confirmTitle[128];
    char confirmMessage[512];
    MxuiActionCallback confirmYes;
    unsigned int* capturedBind;
    unsigned int capturedDefaultBind[MAX_BINDS];
    struct Mod* capturedBindMod;
    char capturedBindId[64];
    s32 capturedHookIndex;
    s32 capturedBindSlot;
    bool pendingThemeRefresh;
    unsigned int modCategory;
    unsigned int modProfile;
    char languageChoices[MXUI_MAX_LANGUAGES][64];
    s32 languageChoiceCount;
    enum MxuiDeferredActionType deferredAction;
    enum MxuiScreenId deferredScreenId;
    s32 deferredTag;
    s16 deferredPauseMode;
    char deferredConfirmTitle[128];
    char deferredConfirmMessage[512];
    MxuiActionCallback deferredConfirmYes;
};

extern struct MxuiState sMxui;

f32 mxui_ui_scale(void);
f32 mxui_clampf(f32 value, f32 minValue, f32 maxValue);
struct MxuiColor mxui_color(u8 r, u8 g, u8 b, u8 a);
struct MxuiTheme mxui_theme(void);
const struct MxuiScreenConfig* mxui_screen_config(enum MxuiScreenId screenId);
struct MxuiScreenState* mxui_current(void);
void mxui_finish_bind_capture(void);
void mxui_reset_screen(struct MxuiScreenState* screen, enum MxuiScreenId screenId, s32 tag);
void mxui_push_if_possible(enum MxuiScreenId screenId, s32 tag);

void mxui_set_color(struct MxuiColor color);
f32 mxui_font_line_height(enum MxuiFontType font, f32 scale);
f32 mxui_text_line_advance(enum MxuiFontType font, f32 scale);
f32 mxui_text_y_in_rect(struct MxuiRect rect, enum MxuiFontType font, f32 scale);
f32 mxui_measure(const char* text, enum MxuiFontType font, f32 scale);
f32 mxui_measure_text_box_height(const char* text, f32 width, f32 scale, enum MxuiFontType font, bool wrap, s32 maxLines);
f32 mxui_fit_text_scale(const char* text, struct MxuiRect rect, f32 preferredScale, f32 minScale, enum MxuiFontType font, bool wrap, s32 maxLines);
void mxui_draw_text_box_fitted(const char* text, struct MxuiRect rect, f32 preferredScale, f32 minScale, enum MxuiFontType font, struct MxuiColor color, enum MxuiTextAlign align, bool wrap, s32 maxLines);
void mxui_draw_rect(struct MxuiRect rect, struct MxuiColor color);
void mxui_draw_outline(struct MxuiRect rect, struct MxuiColor color, f32 thickness);
void mxui_draw_text(const char* text, f32 x, f32 y, f32 scale, enum MxuiFontType font, struct MxuiColor color, bool center);
void mxui_draw_text_right(const char* text, f32 x, f32 y, f32 scale, enum MxuiFontType font, struct MxuiColor color);
void mxui_draw_text_box(const char* text, struct MxuiRect rect, f32 scale, enum MxuiFontType font, struct MxuiColor color, enum MxuiTextAlign align, bool wrap, s32 maxLines);
void mxui_toast(const char* message, s32 frames);
bool mxui_rect_contains(struct MxuiRect rect, f32 x, f32 y);
void mxui_clamp_focus(struct MxuiScreenState* screen);
void mxui_scroll_into_view(struct MxuiScreenState* screen);
void mxui_apply_input(void);
bool mxui_input_accept_pressed(void);
bool mxui_input_accept_down(void);
bool mxui_input_back_pressed(void);
bool mxui_input_menu_toggle_pressed(void);
bool mxui_input_menu_toggle_down(void);
bool mxui_input_prev_page_pressed(void);
bool mxui_input_next_page_pressed(void);
struct MxuiRect mxui_shell_rect(void);
struct MxuiContext mxui_begin_screen(const char* title, const char* subtitle);
struct MxuiContext mxui_begin_screen_template(const struct MxuiScreenConfig* config);
void mxui_end_screen(struct MxuiContext* ctx);
struct MxuiRect mxui_section(struct MxuiContext* ctx, const char* title, const char* description, f32 height);
f32 mxui_section_rows(s32 rowCount);
struct MxuiRect mxui_layout_inset(struct MxuiRect rect, f32 insetX, f32 insetY);
struct MxuiRect mxui_layout_split(struct MxuiRect rect, bool rightSide, f32 gap);
struct MxuiRect mxui_layout_centered(struct MxuiRect rect, f32 width, f32 height);
f32 mxui_row_height_for_text(const char* text, f32 preferredHeight, f32 minScale, f32 preferredScale, enum MxuiFontType font, f32 width, bool wrap, s32 maxLines);
bool mxui_focusable(struct MxuiRect rect, bool* hoveredOut);
bool mxui_widget_click(struct MxuiRect rect, bool focused, bool hovered);
void mxui_sync_focus_feedback(void);
bool mxui_bind_overlap(const unsigned int a[MAX_BINDS], const unsigned int b[MAX_BINDS]);
const char* mxui_bind_name(unsigned int bind);
void mxui_restore_bind_defaults(unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS]);
bool mxui_button(struct MxuiRect rect, const char* label, bool danger);
bool mxui_toggle(struct MxuiRect rect, const char* label, bool* value);
bool mxui_select_u32(struct MxuiRect rect, const char* label, const char* const* choices, s32 count, unsigned int* value);
bool mxui_slider_u32(struct MxuiRect rect, const char* label, unsigned int* value, unsigned int minValue, unsigned int maxValue, unsigned int step);
bool mxui_bind_button(struct MxuiRect rect, const char* text, bool focused, bool hovered);
void mxui_capture_bind_if_needed(void);
bool mxui_bind_row(struct MxuiRect rect, const char* label, unsigned int bindValue[MAX_BINDS], const unsigned int defaultValue[MAX_BINDS], struct Mod* mod, const char* bindId, s32 hookIndex);
void mxui_handle_navigation(void);
void mxui_scroll_apply(struct MxuiContext* ctx);
void mxui_footer_button(struct MxuiRect footer, bool right, const char* label, bool* clicked);
void mxui_footer_center_text(struct MxuiRect footer, const char* text);
void mxui_render_confirm(void);
void mxui_quit_game(void);
void mxui_apply_deferred_action(void);
void mxui_skin_draw_panel(struct MxuiRect rect, struct MxuiColor fill, struct MxuiColor border, struct MxuiColor glow, f32 radius, f32 borderWidth);
void mxui_content_reset(struct MxuiContext* ctx);
struct MxuiRect mxui_stack_next_row(struct MxuiContext* ctx, f32 height);
struct MxuiRowPair mxui_stack_next_split_row(struct MxuiContext* ctx, f32 height, f32 gap);
struct MxuiSectionLayout mxui_section_begin(struct MxuiContext* ctx, const char* title, const char* description, s32 rowCount);
struct MxuiRect mxui_section_next_row(struct MxuiSectionLayout* section, f32 height);
struct MxuiRowPair mxui_section_next_split_row(struct MxuiSectionLayout* section, f32 height, f32 gap);

void mxui_render_boot(struct MxuiContext* ctx);
void mxui_render_save_select(struct MxuiContext* ctx);
void mxui_render_manage_saves(struct MxuiContext* ctx);
void mxui_render_manage_slot(struct MxuiContext* ctx, s32 slot);
void mxui_render_home(struct MxuiContext* ctx);
void mxui_render_pause(struct MxuiContext* ctx);
