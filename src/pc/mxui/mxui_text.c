#include <math.h>
#include <string.h>

#include "sm64.h"
#include "mxui_internal.h"
#include "mxui_font.h"
#include "mxui_render.h"

static f32 mxui_measure_span(const char* text, size_t length, enum MxuiFontType font, f32 scale) {
    if (text == NULL || length == 0) {
        return 0.0f;
    }

    char line[1024] = { 0 };
    length = MIN(length, sizeof(line) - 1);
    memcpy(line, text, length);
    line[length] = '\0';

    enum MxuiFontType lastFont = mxui_font_get_current();
    mxui_font_set_current(font);
    f32 width = mxui_font_measure_text_raw(line) * scale;
    mxui_font_set_current(lastFont);
    return width;
}

static size_t mxui_trim_trailing_spaces(const char* text, size_t length) {
    while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t')) {
        length--;
    }
    return length;
}

static void mxui_copy_span(char* out, size_t outSize, const char* text, size_t length) {
    if (outSize == 0) {
        return;
    }
    length = MIN(length, outSize - 1);
    memcpy(out, text, length);
    out[length] = '\0';
}

static const char* mxui_skip_wrap_spaces(const char* text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

static const char* mxui_next_line_span(const char* text, f32 maxWidth, enum MxuiFontType font, f32 scale, bool wrap, char* out, size_t outSize, bool* truncatedOut) {
    const char* cursor = text;
    const char* lastSpace = NULL;
    size_t currentLen = 0;

    if (truncatedOut != NULL) {
        *truncatedOut = false;
    }

    if (text == NULL || text[0] == '\0') {
        if (outSize > 0) {
            out[0] = '\0';
        }
        return NULL;
    }

    if (!wrap || maxWidth <= 1.0f) {
        const char* end = strchr(text, '\n');
        size_t len = (end != NULL) ? (size_t)(end - text) : strlen(text);
        len = mxui_trim_trailing_spaces(text, len);
        mxui_copy_span(out, outSize, text, len);
        return (end != NULL) ? (end + 1) : NULL;
    }

    while (*cursor != '\0' && *cursor != '\n') {
        size_t candidateLen = (size_t)(cursor - text) + 1;
        if (*cursor == ' ' || *cursor == '\t') {
            lastSpace = cursor;
        }

        if (mxui_measure_span(text, candidateLen, font, scale) > maxWidth) {
            size_t breakLen = 0;
            const char* next = NULL;

            if (lastSpace != NULL && lastSpace >= text) {
                breakLen = (size_t)(lastSpace - text);
                next = mxui_skip_wrap_spaces(lastSpace + 1);
            } else if (currentLen > 0) {
                breakLen = currentLen;
                next = cursor;
            } else {
                breakLen = 1;
                next = cursor + 1;
            }

            breakLen = mxui_trim_trailing_spaces(text, breakLen);
            mxui_copy_span(out, outSize, text, breakLen);
            if (truncatedOut != NULL) {
                *truncatedOut = (*next != '\0');
            }
            return next;
        }

        currentLen = candidateLen;
        cursor++;
    }

    currentLen = mxui_trim_trailing_spaces(text, currentLen);
    mxui_copy_span(out, outSize, text, currentLen);
    if (*cursor == '\n') {
        return cursor + 1;
    }
    return NULL;
}

static void mxui_apply_ellipsis(char* line, size_t lineSize, f32 maxWidth, enum MxuiFontType font, f32 scale) {
    size_t len = strlen(line);
    if (lineSize == 0 || len == 0) {
        return;
    }

    while (len > 0 && mxui_measure_span(line, len, font, scale) + mxui_measure("...", font, scale) > maxWidth) {
        len--;
    }

    if (len == 0) {
        mxui_copy_span(line, lineSize, "...", 3);
        return;
    }

    line[len] = '\0';
    if (len + 3 >= lineSize) {
        len = lineSize - 4;
        line[len] = '\0';
    }
    strcat(line, "...");
}

f32 mxui_font_line_height(enum MxuiFontType font, f32 scale) {
    return mxui_font_line_height_raw(font) * mxui_font_default_scale_raw(font) * scale;
}

f32 mxui_text_line_advance(enum MxuiFontType font, f32 scale) {
    return mxui_font_line_height(font, scale) * 1.15f;
}

f32 mxui_text_y_in_rect(struct MxuiRect rect, enum MxuiFontType font, f32 scale) {
    return floorf(rect.y + (rect.h - mxui_font_line_height(font, scale)) * 0.5f + 1.5f);
}

f32 mxui_measure(const char* text, enum MxuiFontType font, f32 scale) {
    f32 width = 0.0f;
    const char* lineStart = text;

    if (text == NULL) {
        return 0.0f;
    }

    while (lineStart != NULL) {
        const char* lineEnd = strchr(lineStart, '\n');
        size_t lineLength = (lineEnd != NULL) ? (size_t)(lineEnd - lineStart) : strlen(lineStart);
        width = MAX(width, mxui_measure_span(lineStart, lineLength, font, scale));
        lineStart = (lineEnd != NULL) ? (lineEnd + 1) : NULL;
    }

    return width;
}

f32 mxui_measure_text_box_height(const char* text, f32 width, f32 scale, enum MxuiFontType font, bool wrap, s32 maxLines) {
    if (text == NULL || text[0] == '\0') {
        return 0.0f;
    }

    f32 lineAdvance = mxui_text_line_advance(font, scale);
    const char* cursor = text;
    s32 lines = 0;
    char buffer[1024] = { 0 };

    while (cursor != NULL && cursor[0] != '\0') {
        bool truncated = false;
        cursor = mxui_next_line_span(cursor, width, font, scale, wrap, buffer, sizeof(buffer), &truncated);
        lines++;
        if (maxLines > 0 && lines >= maxLines) {
            break;
        }
    }

    return lineAdvance * lines;
}

f32 mxui_fit_text_scale(const char* text, struct MxuiRect rect, f32 preferredScale, f32 minScale, enum MxuiFontType font, bool wrap, s32 maxLines) {
    f32 scale = preferredScale;
    if (text == NULL || text[0] == '\0') {
        return preferredScale;
    }

    while (scale > minScale) {
        f32 height = mxui_measure_text_box_height(text, rect.w, scale, font, wrap, maxLines);
        f32 width = wrap ? rect.w : mxui_measure(text, font, scale);
        if (height <= rect.h + 0.5f && width <= rect.w + 0.5f) {
            break;
        }
        scale -= 0.04f;
    }

    return MAX(minScale, scale);
}

void mxui_draw_text(const char* text, f32 x, f32 y, f32 scale, enum MxuiFontType font, struct MxuiColor color, bool center) {
    struct MxuiRect rect = {
        center ? x - mxui_measure(text, font, scale) * 0.5f : x,
        y,
        center ? mxui_measure(text, font, scale) : 4096.0f,
        mxui_measure_text_box_height(text, 4096.0f, scale, font, false, 0),
    };
    mxui_draw_text_box(text, rect, scale, font, color, center ? MXUI_TEXT_CENTER : MXUI_TEXT_LEFT, false, 0);
}

void mxui_draw_text_right(const char* text, f32 x, f32 y, f32 scale, enum MxuiFontType font, struct MxuiColor color) {
    struct MxuiRect rect = {
        x - mxui_measure(text, font, scale),
        y,
        mxui_measure(text, font, scale),
        mxui_measure_text_box_height(text, 4096.0f, scale, font, false, 0),
    };
    mxui_draw_text_box(text, rect, scale, font, color, MXUI_TEXT_RIGHT, false, 0);
}

void mxui_draw_text_box(const char* text, struct MxuiRect rect, f32 scale, enum MxuiFontType font, struct MxuiColor color, enum MxuiTextAlign align, bool wrap, s32 maxLines) {
    if (text == NULL || text[0] == '\0' || rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    enum MxuiFontType lastFont = mxui_font_get_current();
    const f32 lineAdvance = mxui_text_line_advance(font, scale);
    const f32 maxWidth = MAX(1.0f, rect.w);
    const char* cursor = text;
    s32 lineCount = 0;
    char lines[8][1024] = { 0 };
    f32 widths[8] = { 0 };

    mxui_font_set_current(font);
    mxui_render_reset_texture_clipping();

    while (cursor != NULL && cursor[0] != '\0' && lineCount < (s32)(sizeof(lines) / sizeof(lines[0]))) {
        bool truncated = false;
        const char* next = mxui_next_line_span(cursor, maxWidth, font, scale, wrap, lines[lineCount], sizeof(lines[lineCount]), &truncated);

        if (maxLines > 0 && lineCount == maxLines - 1 && next != NULL && next[0] != '\0') {
            mxui_apply_ellipsis(lines[lineCount], sizeof(lines[lineCount]), maxWidth, font, scale);
            next = NULL;
        }

        widths[lineCount] = mxui_measure_span(lines[lineCount], strlen(lines[lineCount]), font, scale);
        lineCount++;
        if ((maxLines > 0 && lineCount >= maxLines) || next == NULL) {
            break;
        }
        cursor = next;
    }

    if (lineCount <= 0) {
        mxui_font_set_current(lastFont);
        return;
    }

    f32 totalHeight = lineAdvance * lineCount;
    f32 lineY = rect.y;
    if (totalHeight < rect.h) {
        lineY += floorf((rect.h - totalHeight) * 0.5f);
    }

    for (s32 lineIndex = 0; lineIndex < lineCount; lineIndex++) {
        f32 width = widths[lineIndex];
        f32 drawX = rect.x;
        if (align == MXUI_TEXT_CENTER) {
            drawX = rect.x + (rect.w - width) * 0.5f;
        } else if (align == MXUI_TEXT_RIGHT) {
            drawX = rect.x + rect.w - width;
        }

        mxui_set_color(mxui_color(0, 0, 0, MIN(color.a, 40)));
        mxui_font_print_text_raw(lines[lineIndex], drawX + 1.0f, lineY + 1.0f, scale);
        mxui_set_color(color);
        mxui_font_print_text_raw(lines[lineIndex], drawX, lineY, scale);
        lineY += lineAdvance;
        if (lineY > rect.y + rect.h + 1.0f) {
            break;
        }
    }

    mxui_render_reset_texture_clipping();
    mxui_font_set_current(lastFont);
}

void mxui_draw_text_box_fitted(const char* text, struct MxuiRect rect, f32 preferredScale, f32 minScale, enum MxuiFontType font, struct MxuiColor color, enum MxuiTextAlign align, bool wrap, s32 maxLines) {
    if (font == FONT_NORMAL && minScale < 0.42f) {
        minScale = 0.42f;
    }
    if (font == FONT_NORMAL && preferredScale < minScale) {
        preferredScale = minScale;
    }
    f32 scale = mxui_fit_text_scale(text, rect, preferredScale, minScale, font, wrap, maxLines);
    mxui_draw_text_box(text, rect, scale, font, color, align, wrap, maxLines);
}
