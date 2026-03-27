#pragma once

#include <stdbool.h>
#include <PR/ultratypes.h>

extern bool gMxuiDisabled;

void mxui_runtime_init(void);
void mxui_runtime_shutdown(void);
void mxui_runtime_open_main_flow(void);
void mxui_runtime_reset_hud_params(void);
void mxui_runtime_render(void);

void patch_mxui_before(void);
void patch_mxui_interpolated(f32 delta);
