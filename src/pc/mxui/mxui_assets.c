#include "mxui_assets.h"

#include "seq_ids.h"

#include "data/dynos_mgr_builtin_externs.h"

extern ALIGNED8 const u8 texture_hud_char_camera[];
extern ALIGNED8 const u8 texture_hud_char_lakitu[];
extern ALIGNED8 const u8 texture_hud_char_no_camera[];
extern ALIGNED8 const u8 texture_hud_char_arrow_up[];
extern ALIGNED8 const u8 texture_hud_char_arrow_down[];
extern ALIGNED8 const u8 texture_hud_char_coin[];
extern ALIGNED8 const u8 texture_hud_char_star[];
extern ALIGNED8 const u8 texture_hud_char_apostrophe[];
extern ALIGNED8 const u8 texture_hud_char_double_quote[];

bool gPanelLanguageOnStartup = false;

struct GlobalTextures gGlobalTextures = {
    .camera       = { .texture = texture_hud_char_camera,       .name = "texture_hud_char_camera",       .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .lakitu       = { .texture = texture_hud_char_lakitu,       .name = "texture_hud_char_lakitu",       .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .no_camera    = { .texture = texture_hud_char_no_camera,    .name = "texture_hud_char_no_camera",    .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .arrow_up     = { .texture = texture_hud_char_arrow_up,     .name = "texture_hud_char_arrow_up",     .width =  8, .height =  8, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .arrow_down   = { .texture = texture_hud_char_arrow_down,   .name = "texture_hud_char_arrow_down",   .width =  8, .height =  8, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .coin         = { .texture = texture_hud_char_coin,         .name = "texture_hud_char_coin",         .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .star         = { .texture = texture_hud_char_star,         .name = "texture_hud_char_star",         .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .apostrophe   = { .texture = texture_hud_char_apostrophe,   .name = "texture_hud_char_apostrophe",   .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .double_quote = { .texture = texture_hud_char_double_quote, .name = "texture_hud_char_double_quote", .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .mario_head   = { .texture = texture_hud_char_mario_head,   .name = "texture_hud_char_mario_head",   .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .luigi_head   = { .texture = texture_hud_char_luigi_head,   .name = "texture_hud_char_luigi_head",   .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .toad_head    = { .texture = texture_hud_char_toad_head,    .name = "texture_hud_char_toad_head",    .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .waluigi_head = { .texture = texture_hud_char_waluigi_head, .name = "texture_hud_char_waluigi_head", .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
    .wario_head   = { .texture = texture_hud_char_wario_head,   .name = "texture_hud_char_wario_head",   .width = 16, .height = 16, .format = G_IM_FMT_RGBA, .size = G_IM_SIZ_16b },
};

void mxui_assets_init(void) {
}
