#include <string.h>

#include "mxui_internal.h"
#include "mxui_popup.h"
#include "pc/network/packets/packet.h"

static void mxui_popup_sanitize(const char* source, char* out, size_t outSize) {
    size_t outIndex = 0;
    bool lastSpace = false;

    if (outSize == 0) {
        return;
    }

    out[0] = '\0';
    if (source == NULL) {
        return;
    }

    for (size_t i = 0; source[i] != '\0' && outIndex + 1 < outSize; i++) {
        if (source[i] == '\\' && source[i + 1] == '#') {
            i += 2;
            while (source[i] != '\0' && source[i] != '\\') {
                i++;
            }
            continue;
        }

        char c = source[i];
        bool isSpace = (c == '\n' || c == '\r' || c == '\t' || c == ' ');
        if (isSpace) {
            if (lastSpace || outIndex == 0) {
                continue;
            }
            out[outIndex++] = ' ';
            lastSpace = true;
            continue;
        }

        out[outIndex++] = c;
        lastSpace = false;
    }

    out[outIndex] = '\0';
}

void mxui_popup_create(const char* message, int lines) {
    char clean[256] = { 0 };
    mxui_popup_sanitize(message, clean, sizeof(clean));
    if (clean[0] == '\0') {
        return;
    }
    mxui_toast(clean, MAX(60, lines * 45));
}

void mxui_popup_create_global(const char* message, int lines) {
    mxui_popup_create(message, lines);
    network_send_global_popup(message, lines);
}
