#include "sm64.h"
#include "mxui_internal.h"
#include "mxui_render.h"

#include "engine/math_util.h"

static struct MxuiColor mxui_scale_alpha(struct MxuiColor color, f32 alphaScale) {
    color.a = (u8)mxui_clampf((f32)color.a * alphaScale, 0.0f, 255.0f);
    return color;
}

static f32 mxui_ease_out_cubic(f32 t) {
    f32 inv = 1.0f - mxui_clampf(t, 0.0f, 1.0f);
    return 1.0f - inv * inv * inv;
}

static f32 sMxuiSectionTopPad = 16.0f;
static f32 sMxuiSectionBottomPad = 16.0f;
static f32 sMxuiSectionRowGap = 10.0f;
static f32 sMxuiSectionGap = 18.0f;
static f32 sMxuiSectionInnerPad = 14.0f;
static f32 sMxuiDefaultRowHeight = 54.0f;

static bool mxui_template_has_footer(const struct MxuiScreenConfig* config) {
    if (config == NULL) {
        return false;
    }
    if (config->showBackFooter) {
        return true;
    }
    return config->id == MXUI_SCREEN_SAVE_SELECT
        || config->id == MXUI_SCREEN_MODS
        || config->id == MXUI_SCREEN_DYNOS;
}

static const struct TextureInfo* mxui_theme_sprite_primary(void) {
    return NULL;
}

static const struct TextureInfo* mxui_theme_sprite_secondary(void) {
    return NULL;
}

static void mxui_draw_theme_sprites(struct MxuiRect header, struct MxuiColor tint, f32 alphaScale) {
    const struct TextureInfo* primary = mxui_theme_sprite_primary();
    const struct TextureInfo* secondary = mxui_theme_sprite_secondary();
    struct MxuiColor spriteTint = tint;
    spriteTint.a = (u8)mxui_clampf((f32)spriteTint.a * alphaScale, 0.0f, 255.0f);
    mxui_set_color(spriteTint);
    if (primary != NULL) {
        mxui_render_texture(primary, header.x + 6.0f, header.y + 6.0f, 0.60f, 0.60f);
    }
    if (secondary != NULL) {
        mxui_render_texture(secondary, header.x + header.w - 54.0f, header.y + 8.0f, 0.52f, 0.52f);
    }
}

struct MxuiRect mxui_layout_inset(struct MxuiRect rect, f32 insetX, f32 insetY) {
    return (struct MxuiRect) {
        rect.x + insetX,
        rect.y + insetY,
        rect.w - insetX * 2.0f,
        rect.h - insetY * 2.0f,
    };
}

struct MxuiRect mxui_layout_split(struct MxuiRect rect, bool rightSide, f32 gap) {
    f32 half = (rect.w - gap) * 0.5f;
    if (rightSide) {
        return (struct MxuiRect) { rect.x + half + gap, rect.y, half, rect.h };
    }
    return (struct MxuiRect) { rect.x, rect.y, half, rect.h };
}

struct MxuiRect mxui_layout_centered(struct MxuiRect rect, f32 width, f32 height) {
    return (struct MxuiRect) {
        rect.x + (rect.w - width) * 0.5f,
        rect.y + (rect.h - height) * 0.5f,
        width,
        height,
    };
}

f32 mxui_row_height_for_text(const char* text, f32 preferredHeight, f32 minScale, f32 preferredScale, enum MxuiFontType font, f32 width, bool wrap, s32 maxLines) {
    struct MxuiRect measureRect = { 0.0f, 0.0f, width, preferredHeight };
    f32 scale = mxui_fit_text_scale(text, measureRect, preferredScale, minScale, font, wrap, maxLines);
    f32 textHeight = mxui_measure_text_box_height(text, width, scale, font, wrap, maxLines);
    return MAX(preferredHeight, ceilf(textHeight + 18.0f));
}

struct MxuiRect mxui_shell_rect(void) {
    f32 screenW = mxui_render_screen_width();
    f32 screenH = mxui_render_screen_height();
    f32 scale = MIN(screenW / 1280.0f, screenH / 720.0f) * mxui_ui_scale();
    f32 shellW = MIN(screenW - 36.0f, 1128.0f * scale);
    f32 shellH = MIN(screenH - 26.0f, 676.0f * scale);
    f32 anim = mxui_ease_out_cubic(sMxui.shellAnim);
    f32 shellScale = 0.92f + anim * 0.08f;
    shellW *= shellScale;
    shellH *= shellScale;
    return (struct MxuiRect) {
        (screenW - shellW) * 0.5f,
        (screenH - shellH) * 0.5f + (1.0f - anim) * 20.0f,
        shellW,
        shellH,
    };
}

static struct MxuiRect mxui_template_content_rect(struct MxuiRect shell, struct MxuiRect footer, enum MxuiScreenTemplateKind templateKind, bool hasFooter) {
    f32 contentBottom = hasFooter ? footer.y - 18.0f : (shell.y + shell.h - 28.0f);
    struct MxuiRect content = { shell.x + 24, shell.y + 112, shell.w - 48, contentBottom - (shell.y + 112) };

    switch (templateKind) {
        case MXUI_TEMPLATE_FRONT_PAGE: {
            f32 inset = MAX(24.0f, shell.w * 0.12f);
            content = mxui_layout_inset(content, inset, 0.0f);
            break;
        }
        case MXUI_TEMPLATE_SETTINGS_PAGE:
            content = mxui_layout_inset(content, 8.0f, 0.0f);
            break;
        case MXUI_TEMPLATE_DETAIL_PAGE:
            content = mxui_layout_inset(content, 18.0f, 0.0f);
            break;
        case MXUI_TEMPLATE_GRID_PAGE:
        default:
            break;
    }

    return content;
}

struct MxuiContext mxui_begin_screen_template(const struct MxuiScreenConfig* config) {
    struct MxuiTheme theme = mxui_theme();
    struct MxuiRect screenRect = { 0, 0, mxui_render_screen_width(), mxui_render_screen_height() };
    struct MxuiRect shell = mxui_shell_rect();
    f32 anim = mxui_ease_out_cubic(sMxui.shellAnim);
    f32 screenAnim = mxui_ease_out_cubic(sMxui.screenAnim);
    const char* title = (config != NULL && config->title != NULL) ? config->title : "";
    const char* subtitle = (config != NULL && config->subtitle != NULL) ? config->subtitle : "";
    enum MxuiScreenTemplateKind templateKind = (config != NULL) ? config->templateKind : MXUI_TEMPLATE_SETTINGS_PAGE;

    mxui_draw_rect(screenRect, mxui_scale_alpha(theme.overlay, anim));
    mxui_skin_draw_panel(
        shell,
        mxui_scale_alpha(theme.shell, anim),
        mxui_scale_alpha(theme.border, anim),
        mxui_scale_alpha(theme.glow, anim),
        18.0f,
        2.0f
    );

    struct MxuiRect header = { shell.x + 34, shell.y + 20, shell.w - 68, 86 };
    struct MxuiRect footer = { shell.x + 24, shell.y + shell.h - 56, shell.w - 48, 40 };
    bool hasFooter = mxui_template_has_footer(config);
    struct MxuiRect content = mxui_template_content_rect(shell, footer, templateKind, hasFooter);
    f32 contentOffset = (1.0f - screenAnim) * 18.0f;
    header.y += contentOffset * 0.35f;
    content.y += contentOffset;
    footer.y += contentOffset * 0.8f;

    mxui_draw_theme_sprites(header, mxui_scale_alpha(theme.title, anim * 0.9f), anim * screenAnim);
    mxui_draw_text_box_fitted(title, (struct MxuiRect){ header.x, header.y + 2.0f, header.w, 42.0f }, 0.92f, 0.56f, FONT_MENU, mxui_scale_alpha(theme.title, anim * screenAnim), MXUI_TEXT_CENTER, false, 1);
    if (subtitle != NULL && subtitle[0] != '\0') {
        mxui_draw_text_box_fitted(subtitle, (struct MxuiRect){ header.x + 26.0f, header.y + 42.0f, header.w - 52.0f, 24.0f }, 0.60f, 0.42f, FONT_NORMAL, mxui_scale_alpha(theme.textDim, anim * screenAnim), MXUI_TEXT_CENTER, false, 1);
    }

    if (hasFooter) {
        mxui_skin_draw_panel(
            (struct MxuiRect){ footer.x - 2.0f, footer.y - 4.0f, footer.w + 4.0f, footer.h + 8.0f },
            mxui_scale_alpha(theme.panelAlt, anim),
            mxui_scale_alpha(theme.border, anim * 0.9f),
            mxui_scale_alpha(mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 36), anim),
            14.0f,
            1.0f
        );
    }

    {
        f32 screenMaxW = mxui_render_screen_width();
        f32 screenMaxH = mxui_render_screen_height();
        const f32 shellPad = 10.0f;
        const f32 contentPad = 6.0f;
        f32 clipTop = MAX(shell.y + 92.0f, content.y - 10.0f);
        f32 clipBottom = hasFooter ? (footer.y - 12.0f) : (shell.y + shell.h - 20.0f);
        f32 hitX = MAX(0.0f, content.x - contentPad);
        f32 hitY = MAX(0.0f, clipTop);
        f32 hitW = MIN(screenMaxW - hitX, content.w + contentPad * 2.0f);
        f32 hitH = MAX(0.0f, MIN(screenMaxH - hitY, clipBottom - clipTop));
        f32 scissorX = MAX(0.0f, shell.x + shellPad);
        f32 scissorY = MAX(0.0f, shell.y + 12.0f);
        f32 scissorW = MIN(screenMaxW - scissorX, shell.w - shellPad * 2.0f);
        f32 scissorH = MIN(screenMaxH - scissorY, shell.h - 24.0f);
        sMxui.contentClipRect = (struct MxuiRect){ hitX, hitY, hitW, hitH };
        sMxui.contentClipValid = hitH > 0.0f && hitW > 0.0f;
        mxui_render_set_scissor(scissorX, scissorY, scissorW, scissorH);
    }

    return (struct MxuiContext) {
        .config = config,
        .shell = shell,
        .header = header,
        .content = content,
        .footer = footer,
        .cursorY = content.y,
        .contentHeight = 0,
    };
}

struct MxuiContext mxui_begin_screen(const char* title, const char* subtitle) {
    struct MxuiScreenConfig config = {
        .id = MXUI_SCREEN_NONE,
        .title = title,
        .subtitle = subtitle,
        .templateKind = MXUI_TEMPLATE_SETTINGS_PAGE,
        .showBackFooter = false,
        .backLabel = NULL,
    };
    return mxui_begin_screen_template(&config);
}

void mxui_end_screen(struct MxuiContext* ctx) {
    (void)ctx;
    mxui_render_reset_scissor();
    mxui_render_reset_texture_clipping();
    sMxui.contentClipValid = false;
}

void mxui_content_reset(struct MxuiContext* ctx) {
    if (ctx == NULL) {
        return;
    }
    ctx->cursorY = ctx->content.y;
    ctx->contentHeight = 0.0f;
}

struct MxuiRect mxui_stack_next_row(struct MxuiContext* ctx, f32 height) {
    struct MxuiScreenState* screen = mxui_current();
    height = MAX(height, sMxuiDefaultRowHeight);
    struct MxuiRect rect = {
        ctx->content.x,
        ctx->cursorY - (screen != NULL ? screen->scroll : 0.0f),
        ctx->content.w,
        height,
    };
    ctx->cursorY += height + sMxuiSectionRowGap;
    ctx->contentHeight = MAX(ctx->contentHeight, ctx->cursorY - ctx->content.y);
    return rect;
}

struct MxuiRowPair mxui_stack_next_split_row(struct MxuiContext* ctx, f32 height, f32 gap) {
    struct MxuiRect row = mxui_stack_next_row(ctx, height);
    return (struct MxuiRowPair) {
        .left = mxui_layout_split(row, false, gap),
        .right = mxui_layout_split(row, true, gap),
    };
}

struct MxuiSectionLayout mxui_section_begin(struct MxuiContext* ctx, const char* title, const char* description, s32 rowCount) {
    struct MxuiTheme theme = mxui_theme();
    struct MxuiScreenState* screen = mxui_current();
    f32 titleHeight = (title != NULL && title[0] != '\0') ? mxui_font_line_height(FONT_MENU, 0.60f) : 0.0f;
    f32 descWidth = ctx->content.w - sMxuiSectionInnerPad * 2.0f;
    f32 descHeight = (description != NULL && description[0] != '\0')
        ? mxui_measure_text_box_height(description, descWidth, 0.58f, FONT_NORMAL, true, 3)
        : 0.0f;
    f32 headerHeight = sMxuiSectionTopPad + titleHeight;
    if (descHeight > 0.0f) {
        headerHeight += 8.0f + descHeight;
    }
    headerHeight += 12.0f;

    rowCount = MAX(0, rowCount);
    f32 bodyHeight = rowCount > 0
        ? rowCount * sMxuiDefaultRowHeight + (rowCount - 1) * sMxuiSectionRowGap
        : 0.0f;
    f32 totalHeight = MAX(96.0f, headerHeight + bodyHeight + sMxuiSectionBottomPad);
    struct MxuiRect rect = {
        ctx->content.x,
        ctx->cursorY - (screen != NULL ? screen->scroll : 0.0f),
        ctx->content.w,
        totalHeight,
    };

    mxui_skin_draw_panel(rect, theme.panel, theme.border, mxui_color(theme.glow.r, theme.glow.g, theme.glow.b, 28), 14.0f, 1.5f);

    f32 textY = rect.y + sMxuiSectionTopPad;
    if (title != NULL && title[0] != '\0') {
        mxui_draw_text_box_fitted(title,
            (struct MxuiRect){ rect.x + sMxuiSectionInnerPad, textY, rect.w - sMxuiSectionInnerPad * 2.0f, titleHeight + 4.0f },
            0.60f, 0.42f, FONT_MENU, theme.title, MXUI_TEXT_LEFT, true, 2);
        textY += titleHeight + 8.0f;
    }
    if (description != NULL && description[0] != '\0') {
        mxui_draw_text_box_fitted(description,
            (struct MxuiRect){ rect.x + sMxuiSectionInnerPad, textY, rect.w - sMxuiSectionInnerPad * 2.0f, descHeight + 4.0f },
            0.58f, 0.42f, FONT_NORMAL, theme.textDim, MXUI_TEXT_LEFT, true, 3);
    }

    ctx->cursorY += totalHeight + sMxuiSectionGap;
    ctx->contentHeight = MAX(ctx->contentHeight, ctx->cursorY - ctx->content.y);

    return (struct MxuiSectionLayout) {
        .rect = rect,
        .body = {
            rect.x + sMxuiSectionInnerPad,
            rect.y + headerHeight,
            rect.w - sMxuiSectionInnerPad * 2.0f,
            MAX(0.0f, totalHeight - headerHeight - sMxuiSectionBottomPad),
        },
        .cursorY = rect.y + headerHeight,
        .rowGap = sMxuiSectionRowGap,
    };
}

struct MxuiRect mxui_section_next_row(struct MxuiSectionLayout* section, f32 height) {
    height = MAX(height, sMxuiDefaultRowHeight);
    struct MxuiRect rect = {
        section->body.x,
        section->cursorY,
        section->body.w,
        height,
    };
    section->cursorY += height + section->rowGap;
    return rect;
}

struct MxuiRowPair mxui_section_next_split_row(struct MxuiSectionLayout* section, f32 height, f32 gap) {
    struct MxuiRect row = mxui_section_next_row(section, height);
    return (struct MxuiRowPair) {
        .left = mxui_layout_split(row, false, gap),
        .right = mxui_layout_split(row, true, gap),
    };
}

struct MxuiRect mxui_section(struct MxuiContext* ctx, const char* title, const char* description, f32 height) {
    s32 rowCount = 0;
    if (height > 0.0f) {
        rowCount = MAX(1, (s32)roundf((height - 136.0f) / 48.0f) + 1);
    }
    return mxui_section_begin(ctx, title, description, rowCount).rect;
}

f32 mxui_section_rows(s32 rowCount) {
    rowCount = MAX(1, rowCount);
    return sMxuiSectionTopPad
        + mxui_font_line_height(FONT_MENU, 0.60f)
        + 8.0f
        + mxui_text_line_advance(FONT_NORMAL, 0.58f)
        + 12.0f
        + rowCount * sMxuiDefaultRowHeight
        + (rowCount - 1) * sMxuiSectionRowGap
        + sMxuiSectionBottomPad;
}
