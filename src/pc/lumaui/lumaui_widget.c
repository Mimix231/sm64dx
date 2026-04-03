#include "lumaui_widget.h"

#include "lumaui_space.h"
#include "lumaui_text.h"
#include "lumaui_theme.h"

static s16 lumaui_widget_centered_label_top(const struct LumaUIRect *rect) {
    if (rect->h <= LUMAUI_TEXT_GLYPH_H) {
        return rect->y;
    }
    return (s16) (rect->y + ((rect->h - LUMAUI_TEXT_GLYPH_H) / 2));
}

void lumaui_widget_render_button(const struct LumaUIButtonSpec *button) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    const struct LumaUIColor *fill = button->primary ? &theme->primary : &theme->card;
    const struct LumaUIColor *border = button->primary ? &theme->primaryBorder : &theme->cardBorder;
    const struct LumaUIColor *textColor = button->primary ? &theme->card : &theme->text;
    struct LumaUIRect face = lumaui_space_inset_rect(&button->rect, 2, 2);
    struct LumaUIRect highlight = { face.x + 2, face.y + 2, face.w - 4, 2 };
    struct LumaUIRect footer = { face.x + 2, face.y + face.h - 3, face.w - 4, 1 };
    struct LumaUIRect clipRect = lumaui_space_inset_rect(&button->rect, 8, 3);

    if (button->selected || button->hovered) {
        border = &theme->accent;
    }

    lumaui_render_card(&button->rect, button->selected || button->hovered);
    if (face.w > 0 && face.h > 0) {
        lumaui_render_panel(&face, fill, border);
    }
    if (highlight.w > 0 && highlight.h > 0) {
        lumaui_render_panel(&highlight, border, border);
    }
    if (footer.w > 0 && footer.h > 0) {
        lumaui_render_panel(&footer, &theme->panelBorder, &theme->panelBorder);
    }

    lumaui_space_push_clip(&clipRect);
    lumaui_text_draw_centered(button->rect.x + (button->rect.w / 2),
                              lumaui_widget_centered_label_top(&button->rect),
                              button->label, textColor);
    lumaui_space_pop_clip();
}

void lumaui_widget_render_tab_strip(const struct LumaUIRect *rect, const char *leftLabel, const char *rightLabel) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect left = *rect;
    struct LumaUIRect right = *rect;

    left.w /= 2;
    right.x += left.w;
    right.w -= left.w;

    lumaui_render_panel(&left, &theme->panel, &theme->badgeBorder);
    lumaui_render_panel(&right, &theme->card, &theme->cardBorder);

    lumaui_space_push_clip(&left);
    lumaui_text_draw_centered(left.x + (left.w / 2), lumaui_widget_centered_label_top(&left), leftLabel, &theme->text);
    lumaui_space_pop_clip();

    lumaui_space_push_clip(&right);
    lumaui_text_draw_centered(right.x + (right.w / 2), lumaui_widget_centered_label_top(&right), rightLabel, &theme->mutedText);
    lumaui_space_pop_clip();
}

void lumaui_widget_render_scroll_region(const struct LumaUIRect *rect) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect thumbTrack = { rect->x + rect->w - 10, rect->y + 6, 5, rect->h - 12 };
    struct LumaUIRect thumb = { thumbTrack.x, thumbTrack.y + 4, thumbTrack.w, 18 };

    lumaui_render_card(rect, false);
    lumaui_render_panel(&thumbTrack, &theme->panel, &theme->panelBorder);
    lumaui_render_panel(&thumb, &theme->accent, &theme->accent);
}

void lumaui_widget_render_badge(const struct LumaUIRect *rect, const char *label) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect clipRect = lumaui_space_inset_rect(rect, 4, 1);
    struct LumaUIRect inner = lumaui_space_inset_rect(rect, 1, 1);
    struct LumaUIRect highlight = { inner.x + 1, inner.y + 1, inner.w - 2, 1 };

    lumaui_render_panel(rect, &theme->panel, &theme->badgeBorder);
    lumaui_render_panel(&inner, &theme->badge, &theme->badgeBorder);
    if (highlight.w > 0 && highlight.h > 0) {
        lumaui_render_panel(&highlight, &theme->panelBorder, &theme->panelBorder);
    }
    lumaui_space_push_clip(&clipRect);
    lumaui_text_draw_centered(rect->x + (rect->w / 2), lumaui_widget_centered_label_top(rect), label, &theme->text);
    lumaui_space_pop_clip();
}

void lumaui_widget_render_icon_slot(const struct LumaUIRect *rect, const char *label, bool selected) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect icon = { rect->x + 4, rect->y + 4, 18, rect->h - 8 };
    struct LumaUIRect labelRect = { rect->x + 26, rect->y + 4, rect->w - 30, rect->h - 8 };
    const struct LumaUIColor *iconColor = selected ? &theme->primary : &theme->accent;

    lumaui_render_card(rect, selected);
    lumaui_render_panel(&icon, iconColor, &theme->panelBorder);

    lumaui_space_push_clip(&labelRect);
    lumaui_text_draw(labelRect.x, lumaui_widget_centered_label_top(&labelRect), label, &theme->text);
    lumaui_space_pop_clip();
}

void lumaui_widget_render_action_bar(const char *leftText, const char *rightText) {
    const struct LumaUITheme *theme = lumaui_theme_get();
    struct LumaUIRect bar = { 16, 216, 288, 20 };
    struct LumaUIRect clipRect = lumaui_space_inset_rect(&bar, 6, 2);
    struct LumaUIRect seam = { bar.x + 8, bar.y + 5, bar.w - 16, 1 };

    lumaui_render_card(&bar, false);
    lumaui_render_panel(&clipRect, &theme->panel, &theme->panelBorder);
    if (seam.w > 0 && seam.h > 0) {
        lumaui_render_panel(&seam, &theme->panelBorder, &theme->panelBorder);
    }

    lumaui_space_push_clip(&clipRect);
    lumaui_text_draw(bar.x + 8, lumaui_widget_centered_label_top(&bar), leftText, &theme->text);
    if (rightText != NULL && rightText[0] != '\0') {
        s16 width = lumaui_text_measure_width(rightText);
        lumaui_text_draw(bar.x + bar.w - width - 10, lumaui_widget_centered_label_top(&bar), rightText, &theme->mutedText);
    }
    lumaui_space_pop_clip();
}
