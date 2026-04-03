#pragma once

#include "djui.h"

#define SM64DX_MAX_MOONOS_PACKS 128

enum Sm64dxMoonosSourceType {
    SM64DX_MOONOS_SOURCE_NATIVE,
    SM64DX_MOONOS_SOURCE_DYNOS,
};

struct Sm64dxSaveSummary {
    bool exists;
    char title[64];
    char name[64];
    char action[32];
    char starsLine[96];
    char progressionLine[128];
    char unlocksLine[128];
    char moonosLine[128];
    char lastPlayedLine[64];
    char playTimeLine[64];
};

struct Sm64dxMoonosPack {
    char id[64];
    char name[64];
    char author[64];
    char description[192];
    char compatibility[32];
    char sourceId[64];
    char baseCharacter[32];
    char characterName[64];
    char paletteName[64];
    char tags[96];
    u8 sourceType;
    u8 nativeCharacterIndex;
    u8 previewCharacterIndex;
    int dynosPackIndex;
    bool hasLuaScript;
    bool hasDynosAssets;
    bool hasVoices;
    bool hasLifeIcon;
    bool hasHealthMeter;
    bool hasMoveset;
    bool hasAnimations;
    bool hasAttacks;
    bool favorite;
    bool recent;
};

void sm64dx_profile_init(void);
void sm64dx_profile_shutdown(void);
void sm64dx_profile_update(void);

void sm64dx_build_save_summary(int slot, struct Sm64dxSaveSummary *outSummary);
bool sm64dx_has_recent_save(void);
int sm64dx_get_recent_save_slot(void);
void sm64dx_apply_save_setup(int slot);
void sm64dx_capture_current_setup(int slot);
void sm64dx_start_save_slot(int slot, bool playSound);
void sm64dx_copy_save_slot(int srcSlot, int dstSlot);
void sm64dx_erase_save_slot(int slot);

const char *sm64dx_get_save_pack_name(int slot);

int sm64dx_get_moonos_pack_count(void);
const struct Sm64dxMoonosPack *sm64dx_get_moonos_pack(int index);
int sm64dx_get_selected_moonos_pack_index(int slot);
void sm64dx_apply_moonos_pack(int slot, int packIndex);
void sm64dx_toggle_moonos_favorite(int packIndex);
bool sm64dx_is_active_moonos_pack(int slot, int packIndex);
bool sm64dx_has_global_default_moonos(void);
const char *sm64dx_get_global_default_pack_name(void);
void sm64dx_set_global_default_moonos_pack(int slot, int packIndex);
void sm64dx_reset_save_to_global_default(int slot);
