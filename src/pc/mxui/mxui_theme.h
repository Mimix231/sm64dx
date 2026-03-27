#pragma once

#include "sm64.h"

struct DjuiColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct DjuiInteractableTheme {
    struct DjuiColor textColor;
    struct DjuiColor defaultRectColor;
    struct DjuiColor cursorDownRectColor;
    struct DjuiColor hoveredRectColor;
    struct DjuiColor defaultBorderColor;
    struct DjuiColor cursorDownBorderColor;
    struct DjuiColor hoveredBorderColor;
};

struct DjuiThreePanelTheme {
    struct DjuiColor rectColor;
    struct DjuiColor borderColor;
};

struct DjuiPanelTheme {
    bool hudFontHeader;
};

struct DjuiTheme {
    const char* id;
    const char* name;
    struct DjuiInteractableTheme interactables;
    struct DjuiThreePanelTheme threePanels;
    struct DjuiPanelTheme panels;
};

extern struct DjuiTheme* gDjuiThemes[];

char* mxui_theme_get_rainbow_string_color(s32 color);
