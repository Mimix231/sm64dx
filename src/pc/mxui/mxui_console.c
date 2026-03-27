#include <stdio.h>
#include <string.h>

#include "mxui.h"
#include "mxui_console.h"
#include "mxui_internal.h"

char gMxuiConsoleTmpBuffer[MXUI_CONSOLE_MAX_TMP_BUFFER] = { 0 };
static bool sMxuiConsoleOpen = false;

void mxui_console_message_create(const char* message, int level) {
    if (message == NULL || message[0] == '\0') {
        return;
    }

    const char* prefix = "";
    switch (level) {
        case MXUI_CONSOLE_MESSAGE_WARNING: prefix = "[WARN] "; break;
        case MXUI_CONSOLE_MESSAGE_ERROR:   prefix = "[ERROR] "; break;
        default:                      prefix = "[INFO] "; break;
    }

    printf("%s%s\n", prefix, message);

    if (mxui_is_active()) {
        char toast[256] = { 0 };
        snprintf(toast, sizeof(toast), "%s%s", prefix, message);
        mxui_toast(toast, level == MXUI_CONSOLE_MESSAGE_ERROR ? 150 : 90);
    }
}

void mxui_console_toggle(void) {
    sMxuiConsoleOpen = !sMxuiConsoleOpen;
}

bool mxui_console_is_open(void) {
    return sMxuiConsoleOpen;
}
