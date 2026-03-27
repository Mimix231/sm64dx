#pragma once

#include <stdbool.h>

#define MXUI_CONSOLE_MAX_TMP_BUFFER 4096
#define MXUI_CONSOLE_MESSAGE_INFO 0
#define MXUI_CONSOLE_MESSAGE_WARNING 1
#define MXUI_CONSOLE_MESSAGE_ERROR 2

extern char gMxuiConsoleTmpBuffer[MXUI_CONSOLE_MAX_TMP_BUFFER];

void mxui_console_message_create(const char* message, int level);
void mxui_console_toggle(void);
bool mxui_console_is_open(void);
