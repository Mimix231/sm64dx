#include <PR/ultratypes.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "data/dynos_cmap.cpp.h"

#define SPRITE_INDEX_START_CHAR '!'

struct SmCodeGlyph {
    char unicode[16];
    char base;
    f32 width;
    u32 spriteIndex;
};

struct SmCodeGlyph sMxuiGlyphs[] = {
    { "Ã", 'A', 0, 0 }, { "Ã…", 'A', 0, 0 }, { "Ã‚", 'A', 0, 0 }, { "Ã€", 'A', 0, 0 }, { "Ãƒ", 'A', 0, 0 }, { "Ã„", 'A', 0, 0 },
    { "Ã‡", 'C', 0, 0 }, { "Ã‰", 'E', 0, 0 }, { "ÃŠ", 'E', 0, 0 }, { "Ãˆ", 'E', 0, 0 }, { "Ã‹", 'E', 0, 0 }, { "Ã", 'I', 0, 0 },
    { "ÃŽ", 'I', 0, 0 }, { "ÃŒ", 'I', 0, 0 }, { "Ã", 'I', 0, 0 }, { "Ã‘", 'N', 0, 0 }, { "Ã“", 'O', 0, 0 }, { "Ã”", 'O', 0, 0 },
    { "Ã’", 'O', 0, 0 }, { "Ã•", 'O', 0, 0 }, { "Ã–", 'O', 0, 0 }, { "Ãš", 'U', 0, 0 }, { "Ã›", 'U', 0, 0 }, { "Ã™", 'U', 0, 0 },
    { "Ãœ", 'U', 0, 0 }, { "Ã", 'Y', 0, 0 }, { "Å¸", 'Y', 0, 0 },

    { "Ã¡", 'a', 0, 0 }, { "Ã¥", 'a', 0, 0 }, { "Ã¢", 'a', 0, 0 }, { "Ã ", 'a', 0, 0 }, { "Ã£", 'a', 0, 0 }, { "Ã¤", 'a', 0, 0 },
    { "Ã§", 'c', 0, 0 }, { "Ã©", 'e', 0, 0 }, { "Ãª", 'e', 0, 0 }, { "Ã¨", 'e', 0, 0 }, { "Ã«", 'e', 0, 0 }, { "Ã­", 'i', 0, 0 },
    { "Ã®", 'i', 0, 0 }, { "Ã¬", 'i', 0, 0 }, { "Ã¯", 'i', 0, 0 }, { "Ã±", 'n', 0, 0 }, { "Ã³", 'o', 0, 0 }, { "Ã´", 'o', 0, 0 },
    { "Ã²", 'o', 0, 0 }, { "Ãµ", 'o', 0, 0 }, { "Ã¶", 'o', 0, 0 }, { "Ãº", 'u', 0, 0 }, { "Ã»", 'u', 0, 0 }, { "Ã¹", 'u', 0, 0 },
    { "Ã¼", 'u', 0, 0 }, { "Ã½", 'y', 0, 0 }, { "Ã¿", 'y', 0, 0 },

    { "Ã¦", 'a', 15, 0 }, { "Ã†", 'a', 16, 0 }, { "Å“", 'o', 15, 0 }, { "Å’", 'o', 16, 0 }, { "Ã°", 'd', 0, 0 }, { "Ã", 'D', 14, 0 },
    { "Ã¸", 'o', 0, 0 }, { "Ã˜", 'O', 0, 0 }, { "ÃŸ", 'S', 0, 0 },

    { "Â¡", '!', 0, 0 }, { "Â¿", '?', 0, 0 },

    { "Ð‘", 15, 0, 0 }, { "Ð“", 14, 0, 0 }, { "Ð”", 17, 0, 0 }, { "Ð–", 17, 0, 0 }, { "Ð—", 13, 0, 0 }, { "Ð˜", 15, 0, 0 },
    { "Ð™", 15, 0, 0 }, { "Ð›", 13, 0, 0 }, { "ÐŸ", 14, 0, 0 }, { "Ð£", 12, 0, 0 }, { "Ð¤", 17, 0, 0 }, { "Ð¦", 14, 0, 0 },
    { "Ð§", 11, 0, 0 }, { "Ð¨", 17, 0, 0 }, { "Ð©", 17, 0, 0 }, { "Ðª", 13, 0, 0 }, { "Ð«", 17, 0, 0 }, { "Ð¬", 12, 0, 0 },
    { "Ñ¢", 14, 0, 0 }, { "Ð­", 13, 0, 0 }, { "Ð®", 17, 0, 0 }, { "Ð¯", 13, 0, 0 }, { "Ð„", 12, 0, 0 },

    { "Ð°", 13, 0, 0 }, { "Ð±", 11, 0, 0 }, { "Ð²", 11, 0, 0 }, { "Ð³", 10, 0, 0 }, { "Ð´", 12, 0, 0 }, { "Ð¶", 15, 0, 0 },
    { "Ð·", 13, 0, 0 }, { "Ð¸", 12, 0, 0 }, { "Ð¹", 12, 0, 0 }, { "Ðº", 9, 0, 0 }, { "Ð»", 10, 0, 0 }, { "Ð¼", 11, 0, 0 },
    { "Ð½", 11, 0, 0 }, { "Ð¿", 11, 0, 0 }, { "Ñ‚", 11, 0, 0 }, { "Ñ„", 14, 0, 0 }, { "Ñ†", 11, 0, 0 }, { "Ñ‡", 9, 0, 0 },
    { "Ñˆ", 17, 0, 0 }, { "Ñ‰", 17, 0, 0 }, { "ÑŠ", 14, 0, 0 }, { "Ñ‹", 17, 0, 0 }, { "ÑŒ", 12, 0, 0 }, { "Ñ£", 13, 0, 0 },
    { "Ñ", 12, 0, 0 }, { "ÑŽ", 16, 0, 0 }, { "Ñ", 12, 0, 0 }, { "Ñ”", 12, 0, 0 },

    { "ÄŒ", 'C', 0, 0 }, { "Ä", 'c', 0, 0 }, { "Äš", 'E', 0, 0 }, { "Ä›", 'e', 0, 0 }, { "Å ", 'S', 0, 0 }, { "Å¡", 's', 0, 0 },
    { "Å˜", 'R', 0, 0 }, { "Å™", 'r', 0, 0 }, { "Å½", 'Z', 0, 0 }, { "Å¾", 'z', 0, 0 }, { "Å®", 'U', 0, 0 }, { "Å¯", 'u', 0, 0 },
    { "ÄŽ", 'D', 0, 0 }, { "Ä", 'd', 0, 0 }, { "Å‡", 'N', 0, 0 }, { "Åˆ", 'n', 0, 0 }, { "Å¤", 'T', 0, 0 }, { "Å¥", 13, 0, 0 },

    { "Ä™", 'e', 0, 0 }, { "Å„", 'n', 0, 0 }, { "Å›", 's', 0, 0 }, { "Ä‡", 'c', 0, 0 }, { "Åº", 'z', 0, 0 }, { "Å¼", 'z', 0, 0 },
    { "Å‚", 'l', 0, 0 }, { "Ä˜", 'E', 0, 0 }, { "Åƒ", 'N', 0, 0 }, { "Åš", 'S', 0, 0 }, { "Ä†", 'C', 0, 0 }, { "Å¹", 'Z', 0, 0 },
    { "Å»", 'Z', 0, 0 }, { "Å", 'L', 0, 0 }, { "Ä„", 'A', 0, 0 }, { "Ä…", 'a', 0, 0 }, { "Ð‡", 'l', 0, 0 }, { "Ñ—", 'l', 0, 0 },
    { "Ò", 'R', 0, 0 }, { "Ò‘", 'R', 0, 0 },
};

static struct SmCodeGlyph sMxuiGlyphs_JP[1] = { 0 };

struct SmCodeGlyph sMxuiDuplicateGlyphs[] = {
    { "Ð", 'A', 0, 0 }, { "Ð’", 'B', 0, 0 }, { "Ð•", 'E', 0, 0 }, { "Ðš", 'K', 0, 0 }, { "Ðœ", 'M', 0, 0 }, { "Ð", 'H', 0, 0 },
    { "Ðž", 'O', 0, 0 }, { "Ð ", 'P', 0, 0 }, { "Ð¡", 'C', 0, 0 }, { "Ð¢", 'T', 0, 0 }, { "Ð¥", 'X', 0, 0 }, { "Ðµ", 'e', 0, 0 },
    { "Ð¾", 'o', 0, 0 }, { "Ñ€", 'p', 0, 0 }, { "Ñ", 'c', 0, 0 }, { "Ñƒ", 'y', 0, 0 }, { "Ñ…", 'x', 0, 0 },
};

static void* sCharMap = NULL;

static s32 mxui_unicode_count_bytes(char* text) {
    s32 bytes = 0;
    u8 mask = (1 << 7);
    while (*text & mask) {
        bytes++;
        mask >>= 1;
    }
    return bytes ? bytes : 1;
}

static u64 mxui_unicode_to_u64(char* text) {
    s32 bytes = mxui_unicode_count_bytes(text);
    u64 value = (u8)*text;

    if (bytes > 4) { return 0; }

    bytes--;
    while (bytes > 0) {
        value <<= 8;
        value |= (u8)*(++text);
        bytes--;
    }
    return value;
}

void mxui_unicode_init(void) {
    sCharMap = hmap_create(true);

    size_t glyphCount = sizeof(sMxuiGlyphs) / sizeof(sMxuiGlyphs[0]);
    for (size_t i = 0; i < glyphCount; i++) {
        struct SmCodeGlyph* glyph = &sMxuiGlyphs[i];
        glyph->spriteIndex = (128 + i) - SPRITE_INDEX_START_CHAR;

        u64 key = mxui_unicode_to_u64(glyph->unicode);
        s32 bytes = mxui_unicode_count_bytes(glyph->unicode);
        assert(bytes >= 2 && bytes <= 4);
        assert(key > 127);
        hmap_put(sCharMap, key, glyph);
    }

    size_t jpCount = 0;
    for (size_t i = 0; i < jpCount; i++) {
        struct SmCodeGlyph* glyph = &sMxuiGlyphs_JP[i];
        glyph->spriteIndex = 0x010000 + i;
        u64 key = mxui_unicode_to_u64(glyph->unicode);
        s32 bytes = mxui_unicode_count_bytes(glyph->unicode);
        assert(bytes >= 2 && bytes <= 4);
        assert(key > 127);
        hmap_put(sCharMap, key, glyph);
    }

    size_t dupCount = sizeof(sMxuiDuplicateGlyphs) / sizeof(sMxuiDuplicateGlyphs[0]);
    for (size_t i = 0; i < dupCount; i++) {
        struct SmCodeGlyph* glyph = &sMxuiDuplicateGlyphs[i];
        assert((u32)glyph->base < 128);
        assert((u32)glyph->base > SPRITE_INDEX_START_CHAR);
        glyph->spriteIndex = ((u32)glyph->base) - SPRITE_INDEX_START_CHAR;

        u64 key = mxui_unicode_to_u64(glyph->unicode);
        s32 bytes = mxui_unicode_count_bytes(glyph->unicode);
        assert(bytes >= 2 && bytes <= 4);
        assert(key > 127);
        hmap_put(sCharMap, key, glyph);
    }
}

u32 mxui_unicode_get_sprite_index(char* text) {
    if ((u8)*text < 128) {
        if ((u8)*text < SPRITE_INDEX_START_CHAR) {
            return (u8)'?' - SPRITE_INDEX_START_CHAR;
        }
        return (u8)*text - SPRITE_INDEX_START_CHAR;
    }

    u64 key = mxui_unicode_to_u64(text);
    struct SmCodeGlyph* glyph = hmap_get(sCharMap, key);
    if (glyph != NULL) {
        return glyph->spriteIndex;
    }
    return (u8)'?' - SPRITE_INDEX_START_CHAR;
}

f32 mxui_unicode_get_sprite_width(char* text, const f32 font_widths[], f32 unicodeScale) {
    if (text == NULL) { return 0; }

    if ((u8)*text < 128) {
        if ((u8)*text < SPRITE_INDEX_START_CHAR) {
            return font_widths[(u8)'?' - SPRITE_INDEX_START_CHAR];
        }
        return font_widths[(u8)*text - SPRITE_INDEX_START_CHAR];
    }

    u64 key = mxui_unicode_to_u64(text);
    struct SmCodeGlyph* glyph = hmap_get(sCharMap, key);
    if (glyph != NULL) {
        if (glyph->width) {
            return glyph->width / unicodeScale;
        }
        if ((u8)glyph->base < (u8)'!') {
            return glyph->base / unicodeScale;
        }
        return font_widths[(u8)glyph->base - SPRITE_INDEX_START_CHAR];
    }

    return font_widths[(u8)'?' - SPRITE_INDEX_START_CHAR];
}

char* mxui_unicode_next_char(char* text) {
    s32 bytes = mxui_unicode_count_bytes(text);
    while (bytes-- > 0) {
        if (*text == '\0') { return text; }
        text++;
    }
    return text;
}

char* mxui_unicode_at_index(char* text, s32 index) {
    while (index-- > 0) {
        text = mxui_unicode_next_char(text);
    }
    return text;
}

size_t mxui_unicode_len(char* text) {
    size_t len = 0;
    while (*text) {
        text = mxui_unicode_next_char(text);
        len++;
    }
    return len;
}

bool mxui_unicode_valid_char(char* text) {
    if ((u8)*text < 128) {
        return ((u8)*text >= ' ');
    }
    u64 key = mxui_unicode_to_u64(text);
    struct SmCodeGlyph* glyph = hmap_get(sCharMap, key);
    return glyph != NULL;
}

void mxui_unicode_cleanup_end(char* text) {
    s32 slen = strlen(text);
    s32 idx = strlen(text);
    bool foundMulti = false;
    if (idx < 2) { return; }
    idx--;

    while (idx >= 0 && text[idx] & (1 << 7)) {
        foundMulti = true;
        if ((text[idx] & 192) == 192) {
            break;
        }
        idx--;
    }

    if (!foundMulti || idx < 0) { return; }

    s32 bytes = mxui_unicode_count_bytes(&text[idx]);
    if (bytes <= 1) {
        text[idx] = '\0';
        return;
    }

    if ((slen - idx) != bytes) {
        text[idx] = '\0';
    }
}

char mxui_unicode_get_base_char(char* text) {
    if ((u8)*text < ' ') { return '?'; }
    if ((u8)*text < 128) { return *text; }
    if (!sCharMap) { return '?'; }
    u64 key = mxui_unicode_to_u64(text);
    struct SmCodeGlyph* glyph = hmap_get(sCharMap, key);
    return (glyph == NULL) ? '?' : glyph->base;
}

void mxui_unicode_get_char(char* text, char* output) {
    s32 bytes = mxui_unicode_count_bytes(text);
    while (bytes-- > 0) {
        *output = *text;
        output++;
        text++;
    }
    *output = '\0';
}
