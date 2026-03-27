#include "mxui_theme.h"

#include "pc/configfile.h"

static char* sRainbowColors[] = {
    "\\#ff3030\\",
    "\\#40e740\\",
    "\\#40b0ff\\",
    "\\#ffef40\\",
};

static char* sExCoopRainbowColors[] = {
    "\\#ff0800\\",
    "\\#1be700\\",
    "\\#00b3ff\\",
    "\\#ffef00\\",
};

char* mxui_theme_get_rainbow_string_color(s32 color) {
    int i = (color >= 0 && color <= 3) ? color : 0;
    return configExCoopTheme ? sExCoopRainbowColors[i] : sRainbowColors[i];
}
