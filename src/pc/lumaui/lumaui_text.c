#include "lumaui_text.h"

#include <stdio.h>
#include <string.h>

#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/segment2.h"
#include "lumaui_space.h"
#include "lumaui_theme.h"

static void lumaui_text_draw_line(s16 x, s16 y, const char *line, const struct LumaUIColor *color) {
    if (line == NULL || line[0] == '\0') {
        return;
    }

    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, color->r, color->g, color->b, color->a);
    print_generic_ascii_string((s16) lumaui_space_to_screen_x(x), y, line);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

s16 lumaui_text_line_height(void) {
    return LUMAUI_TEXT_LINE_HEIGHT;
}

s16 lumaui_text_measure_width(const char *text) {
    if (text == NULL || text[0] == '\0') {
        return 0;
    }
    return (s16) get_generic_ascii_string_width(text);
}

s16 lumaui_text_measure_block_height(const char *text) {
    s16 lines = 1;

    if (text == NULL || text[0] == '\0') {
        return LUMAUI_TEXT_GLYPH_H;
    }

    for (const char *cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor == '\n') {
            lines++;
        }
    }

    return (lines - 1) * LUMAUI_TEXT_LINE_HEIGHT + LUMAUI_TEXT_GLYPH_H;
}

void lumaui_text_draw(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    char line[256] = { 0 };
    int lineLen = 0;

    if (text == NULL || text[0] == '\0') {
        return;
    }

    for (const char *cursor = text; ; cursor++) {
        if (*cursor == '\n' || *cursor == '\0') {
            line[lineLen] = '\0';
            lumaui_text_draw_line(x, y, line, color);
            lineLen = 0;
            if (*cursor == '\0') {
                break;
            }
            y += LUMAUI_TEXT_LINE_HEIGHT;
            continue;
        }

        if (lineLen < (int) sizeof(line) - 1) {
            line[lineLen++] = *cursor;
        }
    }
}

void lumaui_text_draw_centered(s16 centerX, s16 y, const char *text, const struct LumaUIColor *color) {
    s16 width = lumaui_text_measure_width(text);
    lumaui_text_draw((s16) (centerX - (width / 2)), y, text, color);
}

void lumaui_text_draw_block(s16 x, s16 y, const char *text, const struct LumaUIColor *color) {
    lumaui_text_draw(x, y, text, color);
}

void lumaui_text_draw_block_wrapped(s16 x, s16 y, s16 maxWidth, const char *text,
                                    const struct LumaUIColor *color) {
    char currentLine[128] = { 0 };
    char candidate[128] = { 0 };
    const char *cursor = text;
    int lineLen = 0;

    if (text == NULL || text[0] == '\0') {
        return;
    }

    while (*cursor != '\0') {
        char word[64] = { 0 };
        int wordLen = 0;

        while (*cursor == ' ') {
            cursor++;
        }

        if (*cursor == '\n') {
            currentLine[lineLen] = '\0';
            if (lineLen > 0) {
                lumaui_text_draw_line(x, y, currentLine, color);
                y += LUMAUI_TEXT_LINE_HEIGHT;
            }
            lineLen = 0;
            currentLine[0] = '\0';
            cursor++;
            continue;
        }

        if (*cursor == '\0') {
            break;
        }

        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\n' && wordLen < (int) sizeof(word) - 1) {
            word[wordLen++] = *cursor++;
        }
        word[wordLen] = '\0';

        if (lineLen == 0) {
            snprintf(candidate, sizeof(candidate), "%s", word);
        } else {
            snprintf(candidate, sizeof(candidate), "%s %s", currentLine, word);
        }

        if (lineLen > 0 && lumaui_text_measure_width(candidate) > maxWidth) {
            lumaui_text_draw_line(x, y, currentLine, color);
            y += LUMAUI_TEXT_LINE_HEIGHT;
            snprintf(currentLine, sizeof(currentLine), "%s", word);
            lineLen = (int) strlen(currentLine);
        } else {
            snprintf(currentLine, sizeof(currentLine), "%s", candidate);
            lineLen = (int) strlen(currentLine);
        }
    }

    if (lineLen > 0) {
        lumaui_text_draw_line(x, y, currentLine, color);
    }
}
