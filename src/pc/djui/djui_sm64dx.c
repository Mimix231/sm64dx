#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <dirent.h>

#include "sm64.h"
#include "djui.h"
#include "djui_panel.h"
#include "djui_sm64dx.h"

#include "game/area.h"
#include "game/characters.h"
#include "game/hardcoded.h"
#include "game/level_info.h"
#include "game/level_update.h"
#include "game/player_palette.h"
#include "game/save_file.h"
#include "pc/configfile.h"
#include "pc/fs/fs.h"
#include "pc/ini.h"
#include "pc/lua/smlua.h"
#include "pc/mods/mods.h"
#include "pc/mods/mods_utils.h"
#include "pc/network/network_player.h"
#include "pc/platform.h"
#include "pc/utils/misc.h"
#include "engine/math_util.h"
#include "audio/external.h"
#include "sounds.h"
#include "data/dynos.c.h"

#define SM64DX_PROFILE_FILENAME "sm64dx_profile.ini"
#define SM64DX_MOONOS_DIRECTORY "moonos"
#define SM64DX_MOONOS_PACKS_DIRECTORY SM64DX_MOONOS_DIRECTORY "/packs"
#define SM64DX_MOONOS_LEGACY_MODS_DIRECTORY MOD_DIRECTORY "/" SM64DX_MOONOS_DIRECTORY "/packs"
#define SM64DX_MAX_PINNED_ITEMS 8

struct Sm64dxSaveProfile {
    u64 lastPlayed;
    u64 playSeconds;
    char sourceType[16];
    char sourceId[64];
    char baseCharacter[32];
    char characterName[64];
    char paletteName[64];
    char moonosPackId[64];
    struct PlayerPalette palette;
};

static struct {
    bool initialized;
    bool dirty;
    bool packCatalogRefreshRequested;
    bool hasGlobalDefaultProfile;
    f64 lastClock;
    f64 lastFlushClock;
    f64 playFractions[NUM_SAVE_FILES];
    int lastSaveSlot;
    struct Sm64dxSaveProfile globalDefault;
    struct Sm64dxSaveProfile saves[NUM_SAVE_FILES];
    char recentPackIds[SM64DX_MAX_PINNED_ITEMS][64];
    char favoritePackIds[SM64DX_MAX_PINNED_ITEMS][64];
    struct Sm64dxMoonosPack packs[SM64DX_MAX_MOONOS_PACKS];
    int packCount;
} sSm64dx = { 0 };

extern struct SaveBuffer gSaveBuffer;

static const char *sm64dx_profile_path(void);
static void sm64dx_profile_save(void);
static void sm64dx_refresh_moonos_packs(void);
static void sm64dx_ensure_pack_catalog(void);
static int sm64dx_resolve_character_index(const char *value);
static const char *sm64dx_character_name_from_index(int index);
static int sm64dx_find_pack_index_by_id(const char *packId);
static void sm64dx_assign_pack_to_profile(struct Sm64dxSaveProfile *profile, const struct Sm64dxMoonosPack *pack,
                                          const struct PlayerPalette *fallbackPalette, const char *fallbackPaletteName);
static const char *sm64dx_get_profile_pack_name(const struct Sm64dxSaveProfile *profile);

static void sm64dx_safe_copy(char *dst, size_t dstSize, const char *src) {
    if (dst == NULL || dstSize == 0) {
        return;
    }
    snprintf(dst, dstSize, "%s", (src != NULL) ? src : "");
}

static void sm64dx_ensure_directory_exists(const char *path) {
    if (path == NULL || path[0] == '\0' || fs_sys_dir_exists(path)) {
        return;
    }
    fs_sys_mkdir(path);
}

static const char *sm64dx_profile_path(void) {
    return fs_get_write_path(SM64DX_PROFILE_FILENAME);
}

static const char *sm64dx_character_name_from_index(int index) {
    if (index < 0 || index >= CT_MAX) {
        index = CT_MARIO;
    }
    return gCharacters[index].name;
}

static int sm64dx_resolve_character_index(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return CT_MARIO;
    }

    for (int i = 0; i < CT_MAX; i++) {
        if (!sys_strcasecmp(value, gCharacters[i].name)) {
            return i;
        }
    }

    int index = atoi(value);
    if (index < 0) {
        return CT_MARIO;
    }
    if (index >= CT_MAX) {
        return CT_MAX - 1;
    }
    return index;
}

static enum Sm64dxMoonosSourceType sm64dx_parse_source_type(const char *value) {
    if (value != NULL && !sys_strcasecmp(value, "dynos")) {
        return SM64DX_MOONOS_SOURCE_DYNOS;
    }
    return SM64DX_MOONOS_SOURCE_NATIVE;
}

static const char *sm64dx_source_type_name(enum Sm64dxMoonosSourceType sourceType) {
    return (sourceType == SM64DX_MOONOS_SOURCE_DYNOS) ? "dynos" : "native";
}

static bool sm64dx_value_is_true(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return false;
    }
    return !sys_strcasecmp(value, "1")
        || !sys_strcasecmp(value, "true")
        || !sys_strcasecmp(value, "yes")
        || !sys_strcasecmp(value, "on");
}

static void sm64dx_reset_save_profile(struct Sm64dxSaveProfile *profile) {
    if (profile == NULL) {
        return;
    }

    memset(profile, 0, sizeof(*profile));
    sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), "native");
    sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId), "Mario");
    sm64dx_safe_copy(profile->baseCharacter, sizeof(profile->baseCharacter), "Mario");
    sm64dx_safe_copy(profile->characterName, sizeof(profile->characterName), "Mario");
    sm64dx_safe_copy(profile->moonosPackId, sizeof(profile->moonosPackId), "Mario");
    profile->palette = DEFAULT_MARIO_PALETTE;
}

static int sm64dx_clamp_slot(int slot) {
    if (slot < 1 || slot > NUM_SAVE_FILES) {
        return 0;
    }
    return slot;
}

static void sm64dx_ensure_palette_cache(void) {
    player_palettes_reset();
    player_palettes_read(sys_package_path(), true);
    player_palettes_read(fs_get_write_path(PALETTES_DIRECTORY), false);
}

static int sm64dx_find_palette_index_by_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return -1;
    }
    for (int i = 0; i < gPresetPaletteCount; i++) {
        if (!sys_strcasecmp(gPresetPalettes[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static void sm64dx_find_palette_name_for_current(char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return;
    }

    buffer[0] = '\0';
    for (int i = 0; i < gPresetPaletteCount; i++) {
        if (memcmp(&configPlayerPalette, &gPresetPalettes[i].palette, sizeof(struct PlayerPalette)) == 0) {
            snprintf(buffer, bufferSize, "%s", gPresetPalettes[i].name);
            return;
        }
    }
}

static void sm64dx_apply_live_player_settings(void) {
    if (configPlayerModel >= CT_MAX) {
        configPlayerModel = CT_MARIO;
    }

    gNetworkPlayers[0].modelIndex = configPlayerModel;
    gNetworkPlayers[0].overrideModelIndex = configPlayerModel;
    gNetworkPlayers[0].palette = configPlayerPalette;
    gNetworkPlayers[0].overridePalette = configPlayerPalette;
    network_player_update_model(0);
}

static u32 sm64dx_get_save_flags_for_slot(int slotZeroBased) {
    if (slotZeroBased < 0 || slotZeroBased >= NUM_SAVE_FILES) {
        return 0;
    }
    return gSaveBuffer.files[slotZeroBased][0].flags;
}

static int sm64dx_count_100_coin_stars(int slotZeroBased) {
    int count = 0;
    for (int course = 0; course < COURSE_STAGES_COUNT; course++) {
        if (save_file_get_course_coin_score(slotZeroBased, course) >= gLevelValues.coinsRequiredForCoinStar) {
            count++;
        }
    }
    return count;
}

static int sm64dx_count_caps(u32 flags) {
    return ((flags & SAVE_FLAG_HAVE_WING_CAP) ? 1 : 0)
         + ((flags & SAVE_FLAG_HAVE_METAL_CAP) ? 1 : 0)
         + ((flags & SAVE_FLAG_HAVE_VANISH_CAP) ? 1 : 0);
}

static int sm64dx_count_keys(u32 flags) {
    return ((flags & (SAVE_FLAG_HAVE_KEY_1 | SAVE_FLAG_UNLOCKED_BASEMENT_DOOR)) ? 1 : 0)
         + ((flags & (SAVE_FLAG_HAVE_KEY_2 | SAVE_FLAG_UNLOCKED_UPSTAIRS_DOOR)) ? 1 : 0);
}

static const char *sm64dx_castle_progress_label(u32 flags, int totalStars) {
    if (flags & SAVE_FLAG_UNLOCKED_50_STAR_DOOR) {
        return "Final Door Open";
    }
    if (flags & SAVE_FLAG_UNLOCKED_UPSTAIRS_DOOR) {
        return "Upstairs Open";
    }
    if (flags & SAVE_FLAG_UNLOCKED_BASEMENT_DOOR) {
        return "Basement Open";
    }
    if (totalStars > 0) {
        return "Main Floor";
    }
    return "Brand New";
}

static void sm64dx_format_play_time(u64 seconds, char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return;
    }

    u64 hours = seconds / 3600ULL;
    u64 minutes = (seconds % 3600ULL) / 60ULL;

    if (seconds == 0) {
        snprintf(buffer, bufferSize, "0m");
    } else if (hours == 0) {
        snprintf(buffer, bufferSize, "%" PRIu64 "m", minutes);
    } else {
        snprintf(buffer, bufferSize, "%" PRIu64 "h %" PRIu64 "m", hours, minutes);
    }
}

static void sm64dx_format_last_played(u64 timestamp, char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return;
    }
    if (timestamp == 0) {
        snprintf(buffer, bufferSize, "Never");
        return;
    }

    time_t rawTime = (time_t) timestamp;
    struct tm *tmInfo = localtime(&rawTime);
    if (tmInfo == NULL) {
        snprintf(buffer, bufferSize, "Unknown");
        return;
    }

    char timeBuffer[32] = { 0 };
    strftime(timeBuffer, sizeof(timeBuffer), "%d %b %Y", tmInfo);
    snprintf(buffer, bufferSize, "%s", timeBuffer);
}

static void sm64dx_format_stars_line(int totalStars, char *buffer, size_t bufferSize) {
    char ribbon[24] = { 0 };
    int ribbonStars = MIN(6, (totalStars + 19) / 20);
    char *cursor = ribbon;
    for (int i = 0; i < 6; i++) {
        *cursor++ = (i < ribbonStars) ? ('~' + 1) : '-';
        if (i < 5) {
            *cursor++ = ' ';
        }
    }
    *cursor = '\0';

    snprintf(buffer, bufferSize, "%c x%d    %s", '~' + 1, totalStars, ribbon);
}

static void sm64dx_read_palette_triplet(const char *value, Color color) {
    int r = 0;
    int g = 0;
    int b = 0;

    if (value != NULL) {
        sscanf(value, "%d,%d,%d", &r, &g, &b);
    }

    if (r < 0) { r = 0; } else if (r > 255) { r = 255; }
    if (g < 0) { g = 0; } else if (g > 255) { g = 255; }
    if (b < 0) { b = 0; } else if (b > 255) { b = 255; }

    color[0] = r;
    color[1] = g;
    color[2] = b;
}

static void sm64dx_normalize_save_profile(struct Sm64dxSaveProfile *profile, int fallbackModelIndex) {
    if (profile == NULL) {
        return;
    }

    if (profile->sourceType[0] == '\0') {
        sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), "native");
    }

    if (sys_strcasecmp(profile->sourceType, "dynos") && sys_strcasecmp(profile->sourceType, "native")) {
        sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), "native");
    }

    if (profile->baseCharacter[0] == '\0') {
        sm64dx_safe_copy(profile->baseCharacter, sizeof(profile->baseCharacter), sm64dx_character_name_from_index(fallbackModelIndex));
    }

    if (profile->sourceId[0] == '\0') {
        if (!sys_strcasecmp(profile->sourceType, "dynos")) {
            sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId),
                             (profile->moonosPackId[0] != '\0') ? profile->moonosPackId : profile->baseCharacter);
        } else {
            sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId), profile->baseCharacter);
        }
    }

    if (profile->characterName[0] == '\0') {
        sm64dx_safe_copy(profile->characterName, sizeof(profile->characterName), profile->baseCharacter);
    }
}

static void sm64dx_assign_pack_to_profile(struct Sm64dxSaveProfile *profile, const struct Sm64dxMoonosPack *pack,
                                          const struct PlayerPalette *fallbackPalette, const char *fallbackPaletteName) {
    struct PlayerPalette palette = DEFAULT_MARIO_PALETTE;
    char paletteName[64] = { 0 };

    if (profile == NULL || pack == NULL) {
        return;
    }

    if (fallbackPalette != NULL) {
        palette = *fallbackPalette;
    }
    if (fallbackPaletteName != NULL && fallbackPaletteName[0] != '\0') {
        sm64dx_safe_copy(paletteName, sizeof(paletteName), fallbackPaletteName);
    }

    sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), sm64dx_source_type_name(pack->sourceType));
    sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId), pack->sourceId);
    sm64dx_safe_copy(profile->baseCharacter, sizeof(profile->baseCharacter), pack->baseCharacter);
    sm64dx_safe_copy(profile->characterName, sizeof(profile->characterName),
                     (pack->characterName[0] != '\0') ? pack->characterName : pack->baseCharacter);
    sm64dx_safe_copy(profile->moonosPackId, sizeof(profile->moonosPackId), pack->id);

    if (pack->paletteName[0] != '\0') {
        int paletteIndex = sm64dx_find_palette_index_by_name(pack->paletteName);
        if (paletteIndex >= 0) {
            palette = gPresetPalettes[paletteIndex].palette;
            sm64dx_safe_copy(paletteName, sizeof(paletteName), gPresetPalettes[paletteIndex].name);
        } else {
            sm64dx_safe_copy(paletteName, sizeof(paletteName), pack->paletteName);
        }
    }

    profile->palette = palette;
    sm64dx_safe_copy(profile->paletteName, sizeof(profile->paletteName), paletteName);
}

static const char *sm64dx_get_profile_pack_name(const struct Sm64dxSaveProfile *profile) {
    int packIndex = -1;

    if (profile == NULL) {
        return "Mario";
    }

    sm64dx_ensure_pack_catalog();

    packIndex = sm64dx_find_pack_index_by_id(profile->moonosPackId);
    if (packIndex >= 0) {
        return sSm64dx.packs[packIndex].name;
    }
    if (profile->characterName[0] != '\0') {
        return profile->characterName;
    }
    if (!sys_strcasecmp(profile->sourceType, "dynos") && profile->sourceId[0] != '\0') {
        return profile->sourceId;
    }
    if (profile->baseCharacter[0] != '\0') {
        return profile->baseCharacter;
    }
    if (profile->sourceId[0] != '\0') {
        return profile->sourceId;
    }
    return "Mario";
}

static void sm64dx_read_profile(void) {
    ini_t *ini = ini_load(sm64dx_profile_path());
    if (ini == NULL) {
        return;
    }

    const char *value = ini_get(ini, "global", "last_slot");
    if (value != NULL) {
        sSm64dx.lastSaveSlot = atoi(value);
    }

    value = ini_get(ini, "global", "default_enabled");
    sSm64dx.hasGlobalDefaultProfile = sm64dx_value_is_true(value);

    value = ini_get(ini, "global", "default_source_type");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.sourceType, sizeof(sSm64dx.globalDefault.sourceType), value);
    }

    value = ini_get(ini, "global", "default_source_id");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.sourceId, sizeof(sSm64dx.globalDefault.sourceId), value);
    }

    value = ini_get(ini, "global", "default_base_character");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.baseCharacter, sizeof(sSm64dx.globalDefault.baseCharacter), value);
    }

    value = ini_get(ini, "global", "default_character_name");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.characterName, sizeof(sSm64dx.globalDefault.characterName), value);
    }

    value = ini_get(ini, "global", "default_palette_name");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.paletteName, sizeof(sSm64dx.globalDefault.paletteName), value);
    }

    value = ini_get(ini, "global", "default_moonos_pack");
    if (value != NULL) {
        sm64dx_safe_copy(sSm64dx.globalDefault.moonosPackId, sizeof(sSm64dx.globalDefault.moonosPackId), value);
    }

    for (int part = 0; part < PLAYER_PART_MAX; part++) {
        char key[32] = { 0 };
        snprintf(key, sizeof(key), "default_palette_%d", part);
        value = ini_get(ini, "global", key);
        if (value != NULL) {
            sm64dx_read_palette_triplet(value, sSm64dx.globalDefault.palette.parts[part]);
        }
    }

    sm64dx_normalize_save_profile(&sSm64dx.globalDefault, CT_MARIO);

    for (int i = 0; i < SM64DX_MAX_PINNED_ITEMS; i++) {
        char key[32] = { 0 };

        snprintf(key, sizeof(key), "recent_pack_%d", i);
        value = ini_get(ini, "global", key);
        if (value != NULL) {
            sm64dx_safe_copy(sSm64dx.recentPackIds[i], sizeof(sSm64dx.recentPackIds[i]), value);
        }

        snprintf(key, sizeof(key), "favorite_pack_%d", i);
        value = ini_get(ini, "global", key);
        if (value != NULL) {
            sm64dx_safe_copy(sSm64dx.favoritePackIds[i], sizeof(sSm64dx.favoritePackIds[i]), value);
        }
    }

    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        struct Sm64dxSaveProfile *profile = &sSm64dx.saves[i];
        char section[16] = { 0 };
        int fallbackModelIndex = CT_MARIO;

        snprintf(section, sizeof(section), "save%d", i + 1);

        value = ini_get(ini, section, "last_played");
        if (value != NULL) {
            profile->lastPlayed = strtoull(value, NULL, 10);
        }

        value = ini_get(ini, section, "play_seconds");
        if (value != NULL) {
            profile->playSeconds = strtoull(value, NULL, 10);
        }

        value = ini_get(ini, section, "model");
        if (value != NULL) {
            fallbackModelIndex = sm64dx_resolve_character_index(value);
        }

        value = ini_get(ini, section, "source_type");
        if (value != NULL) {
            sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), value);
        }

        value = ini_get(ini, section, "source_id");
        if (value != NULL) {
            sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId), value);
        }

        value = ini_get(ini, section, "base_character");
        if (value != NULL) {
            sm64dx_safe_copy(profile->baseCharacter, sizeof(profile->baseCharacter), value);
        }

        value = ini_get(ini, section, "character_name");
        if (value != NULL) {
            sm64dx_safe_copy(profile->characterName, sizeof(profile->characterName), value);
        }

        value = ini_get(ini, section, "palette_name");
        if (value != NULL) {
            sm64dx_safe_copy(profile->paletteName, sizeof(profile->paletteName), value);
        }

        value = ini_get(ini, section, "moonos_pack");
        if (value != NULL) {
            sm64dx_safe_copy(profile->moonosPackId, sizeof(profile->moonosPackId), value);
        }

        for (int part = 0; part < PLAYER_PART_MAX; part++) {
            char key[32] = { 0 };
            snprintf(key, sizeof(key), "palette_%d", part);
            value = ini_get(ini, section, key);
            if (value != NULL) {
                sm64dx_read_palette_triplet(value, profile->palette.parts[part]);
            }
        }

        sm64dx_normalize_save_profile(profile, fallbackModelIndex);
    }

    ini_free(ini);
}

static void sm64dx_profile_save(void) {
    FILE *file = fopen(sm64dx_profile_path(), "w");
    if (file == NULL) {
        return;
    }

    fprintf(file, "[global]\n");
    fprintf(file, "last_slot=%d\n", sSm64dx.lastSaveSlot);
    fprintf(file, "default_enabled=%d\n", sSm64dx.hasGlobalDefaultProfile ? 1 : 0);
    fprintf(file, "default_source_type=%s\n", sSm64dx.globalDefault.sourceType);
    fprintf(file, "default_source_id=%s\n", sSm64dx.globalDefault.sourceId);
    fprintf(file, "default_base_character=%s\n", sSm64dx.globalDefault.baseCharacter);
    fprintf(file, "default_character_name=%s\n", sSm64dx.globalDefault.characterName);
    fprintf(file, "default_palette_name=%s\n", sSm64dx.globalDefault.paletteName);
    fprintf(file, "default_moonos_pack=%s\n", sSm64dx.globalDefault.moonosPackId);
    for (int part = 0; part < PLAYER_PART_MAX; part++) {
        fprintf(file, "default_palette_%d=%u,%u,%u\n",
                part,
                sSm64dx.globalDefault.palette.parts[part][0],
                sSm64dx.globalDefault.palette.parts[part][1],
                sSm64dx.globalDefault.palette.parts[part][2]);
    }

    for (int i = 0; i < SM64DX_MAX_PINNED_ITEMS; i++) {
        if (sSm64dx.recentPackIds[i][0] != '\0') {
            fprintf(file, "recent_pack_%d=%s\n", i, sSm64dx.recentPackIds[i]);
        }
    }

    for (int i = 0; i < SM64DX_MAX_PINNED_ITEMS; i++) {
        if (sSm64dx.favoritePackIds[i][0] != '\0') {
            fprintf(file, "favorite_pack_%d=%s\n", i, sSm64dx.favoritePackIds[i]);
        }
    }

    fprintf(file, "\n");

    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        const struct Sm64dxSaveProfile *profile = &sSm64dx.saves[i];
        int modelIndex = sm64dx_resolve_character_index(profile->baseCharacter);

        fprintf(file, "[save%d]\n", i + 1);
        fprintf(file, "last_played=%" PRIu64 "\n", (uint64_t) profile->lastPlayed);
        fprintf(file, "play_seconds=%" PRIu64 "\n", (uint64_t) profile->playSeconds);
        fprintf(file, "model=%d\n", modelIndex);
        fprintf(file, "source_type=%s\n", profile->sourceType);
        fprintf(file, "source_id=%s\n", profile->sourceId);
        fprintf(file, "base_character=%s\n", profile->baseCharacter);
        fprintf(file, "character_name=%s\n", profile->characterName);
        fprintf(file, "palette_name=%s\n", profile->paletteName);
        fprintf(file, "moonos_pack=%s\n", profile->moonosPackId);
        for (int part = 0; part < PLAYER_PART_MAX; part++) {
            fprintf(file, "palette_%d=%u,%u,%u\n",
                    part,
                    profile->palette.parts[part][0],
                    profile->palette.parts[part][1],
                    profile->palette.parts[part][2]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    sSm64dx.dirty = false;
    sSm64dx.lastFlushClock = clock_elapsed_f64();
}

static bool sm64dx_array_contains(char items[][64], int count, const char *value, int *outIndex) {
    if (value == NULL || value[0] == '\0') {
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (!sys_strcasecmp(items[i], value)) {
            if (outIndex != NULL) {
                *outIndex = i;
            }
            return true;
        }
    }
    return false;
}

static void sm64dx_array_promote_front(char items[][64], int count, const char *value) {
    int existingIndex = -1;
    if (sm64dx_array_contains(items, count, value, &existingIndex)) {
        for (int i = existingIndex; i > 0; i--) {
            sm64dx_safe_copy(items[i], sizeof(items[i]), items[i - 1]);
        }
    } else {
        for (int i = count - 1; i > 0; i--) {
            sm64dx_safe_copy(items[i], sizeof(items[i]), items[i - 1]);
        }
    }
    sm64dx_safe_copy(items[0], sizeof(items[0]), value);
}

static void sm64dx_array_remove(char items[][64], int count, const char *value) {
    int existingIndex = -1;
    if (!sm64dx_array_contains(items, count, value, &existingIndex)) {
        return;
    }

    for (int i = existingIndex; i < count - 1; i++) {
        sm64dx_safe_copy(items[i], sizeof(items[i]), items[i + 1]);
    }
    items[count - 1][0] = '\0';
}

static int sm64dx_find_pack_index_by_id(const char *packId) {
    if (packId == NULL || packId[0] == '\0') {
        return -1;
    }
    for (int i = 0; i < sSm64dx.packCount; i++) {
        if (!sys_strcasecmp(sSm64dx.packs[i].id, packId)) {
            return i;
        }
    }
    return -1;
}

static int sm64dx_find_pack_index_by_source_id(enum Sm64dxMoonosSourceType sourceType, const char *sourceId) {
    if (sourceId == NULL || sourceId[0] == '\0') {
        return -1;
    }

    for (int i = 0; i < sSm64dx.packCount; i++) {
        if (sSm64dx.packs[i].sourceType != sourceType) {
            continue;
        }
        if (!sys_strcasecmp(sSm64dx.packs[i].sourceId, sourceId)) {
            return i;
        }
    }

    return -1;
}

static int sm64dx_find_dynos_pack_index_by_name(const char *packName) {
    if (packName == NULL || packName[0] == '\0') {
        return -1;
    }

    int packCount = dynos_pack_get_count();
    for (int i = 0; i < packCount; i++) {
        const char *name = dynos_pack_get_name(i);
        if (name != NULL && !sys_strcasecmp(name, packName)) {
            return i;
        }
    }
    return -1;
}

static void sm64dx_sync_pack_flags(void) {
    for (int i = 0; i < sSm64dx.packCount; i++) {
        struct Sm64dxMoonosPack *pack = &sSm64dx.packs[i];
        pack->favorite = sm64dx_array_contains(sSm64dx.favoritePackIds, SM64DX_MAX_PINNED_ITEMS, pack->id, NULL);
        pack->recent = sm64dx_array_contains(sSm64dx.recentPackIds, SM64DX_MAX_PINNED_ITEMS, pack->id, NULL);
    }
}

static void sm64dx_sort_packs(void) {
    for (int i = 0; i < sSm64dx.packCount; i++) {
        for (int j = i + 1; j < sSm64dx.packCount; j++) {
            struct Sm64dxMoonosPack *a = &sSm64dx.packs[i];
            struct Sm64dxMoonosPack *b = &sSm64dx.packs[j];
            bool swap = false;

            if (a->favorite != b->favorite) {
                swap = (!a->favorite && b->favorite);
            } else if (a->recent != b->recent) {
                swap = (!a->recent && b->recent);
            } else if (sys_strcasecmp(a->name, b->name) > 0) {
                swap = true;
            }

            if (swap) {
                struct Sm64dxMoonosPack temp = *a;
                *a = *b;
                *b = temp;
            }
        }
    }
}

static bool sm64dx_directory_has_top_level_extension(const char *dirPath, const char *extension) {
    if (dirPath == NULL || extension == NULL || !fs_sys_dir_exists(dirPath)) {
        return false;
    }

    DIR *directory = opendir(dirPath);
    if (directory == NULL) {
        return false;
    }

    bool found = false;
    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_name[0] == '.' || entry->d_name[0] == '\0') {
            continue;
        }
        if (path_ends_with(entry->d_name, extension)) {
            found = true;
            break;
        }
    }

    closedir(directory);
    return found;
}

static bool sm64dx_directory_has_named_prefix(const char *dirPath, const char *prefix) {
    if (dirPath == NULL || prefix == NULL || !fs_sys_dir_exists(dirPath)) {
        return false;
    }

    DIR *directory = opendir(dirPath);
    if (directory == NULL) {
        return false;
    }

    bool found = false;
    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_name[0] == '.' || entry->d_name[0] == '\0') {
            continue;
        }
        if (!strncmp(entry->d_name, prefix, strlen(prefix))) {
            found = true;
            break;
        }
    }

    closedir(directory);
    return found;
}

static bool sm64dx_should_ignore_pack_directory(const char *name) {
    return (name == NULL || name[0] == '\0' || name[0] == '.' || name[0] == '_');
}

static const char *sm64dx_leaf_name_from_path(const char *path) {
    const char *leaf = path;

    if (path == NULL) {
        return "";
    }

    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if ((*cursor == '/' || *cursor == '\\') && *(cursor + 1) != '\0') {
            leaf = cursor + 1;
        }
    }

    return leaf;
}

static bool sm64dx_directory_looks_like_pack(const char *packDir) {
    char path[SYS_MAX_PATH] = { 0 };

    snprintf(path, sizeof(path), "%s/pack.ini", packDir);
    if (fs_sys_file_exists(path)) {
        return true;
    }

    snprintf(path, sizeof(path), "%s/moonos.ini", packDir);
    if (fs_sys_file_exists(path)) {
        return true;
    }

    snprintf(path, sizeof(path), "%s/main.lua", packDir);
    if (fs_sys_file_exists(path)) {
        return true;
    }

    snprintf(path, sizeof(path), "%s/actors", packDir);
    if (fs_sys_dir_exists(path)) {
        return true;
    }

    snprintf(path, sizeof(path), "%s/assets", packDir);
    if (fs_sys_dir_exists(path)) {
        return true;
    }

    return sm64dx_directory_has_top_level_extension(packDir, ".bin")
        || sm64dx_directory_has_top_level_extension(packDir, ".tex");
}

static void sm64dx_detect_pack_features(const char *packDir, struct Sm64dxMoonosPack *pack) {
    char path[SYS_MAX_PATH] = { 0 };

    snprintf(path, sizeof(path), "%s/main.lua", packDir);
    pack->hasLuaScript = pack->hasLuaScript || fs_sys_file_exists(path);

    snprintf(path, sizeof(path), "%s/actors", packDir);
    pack->hasDynosAssets = pack->hasDynosAssets || fs_sys_dir_exists(path);
    snprintf(path, sizeof(path), "%s/assets", packDir);
    pack->hasDynosAssets = pack->hasDynosAssets || fs_sys_dir_exists(path);
    pack->hasDynosAssets = pack->hasDynosAssets
        || sm64dx_directory_has_top_level_extension(packDir, ".bin")
        || sm64dx_directory_has_top_level_extension(packDir, ".tex");

    snprintf(path, sizeof(path), "%s/sound", packDir);
    pack->hasVoices = pack->hasVoices || fs_sys_dir_exists(path);
    snprintf(path, sizeof(path), "%s/voice.lua", packDir);
    pack->hasVoices = pack->hasVoices || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/voices.lua", packDir);
    pack->hasVoices = pack->hasVoices || fs_sys_file_exists(path);

    snprintf(path, sizeof(path), "%s/textures", packDir);
    if (fs_sys_dir_exists(path)) {
        pack->hasLifeIcon = pack->hasLifeIcon || sm64dx_directory_has_named_prefix(path, "icon");
        pack->hasLifeIcon = pack->hasLifeIcon || sm64dx_directory_has_named_prefix(path, "life");
        pack->hasHealthMeter = pack->hasHealthMeter || sm64dx_directory_has_named_prefix(path, "char_select_");
        pack->hasHealthMeter = pack->hasHealthMeter || sm64dx_directory_has_named_prefix(path, "meter");
        pack->hasHealthMeter = pack->hasHealthMeter || sm64dx_directory_has_named_prefix(path, "cake");
    }

    snprintf(path, sizeof(path), "%s/movesets.lua", packDir);
    pack->hasMoveset = pack->hasMoveset || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/moveset.lua", packDir);
    pack->hasMoveset = pack->hasMoveset || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/moves.lua", packDir);
    pack->hasMoveset = pack->hasMoveset || fs_sys_file_exists(path);

    snprintf(path, sizeof(path), "%s/custom-anims.lua", packDir);
    pack->hasAnimations = pack->hasAnimations || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/anims.lua", packDir);
    pack->hasAnimations = pack->hasAnimations || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/animations.lua", packDir);
    pack->hasAnimations = pack->hasAnimations || fs_sys_file_exists(path);

    snprintf(path, sizeof(path), "%s/attacks.lua", packDir);
    pack->hasAttacks = pack->hasAttacks || fs_sys_file_exists(path);
    snprintf(path, sizeof(path), "%s/combat.lua", packDir);
    pack->hasAttacks = pack->hasAttacks || fs_sys_file_exists(path);
}

static void sm64dx_load_pack_metadata(const char *dirName, const char *packDir, struct Sm64dxMoonosPack *pack) {
    char iniPath[SYS_MAX_PATH] = { 0 };
    ini_t *ini = NULL;
    const char *value = NULL;
    const char *leafName = sm64dx_leaf_name_from_path(dirName);

    memset(pack, 0, sizeof(*pack));
    pack->sourceType = SM64DX_MOONOS_SOURCE_NATIVE;
    pack->nativeCharacterIndex = CT_MARIO;
    pack->previewCharacterIndex = CT_MARIO;
    pack->dynosPackIndex = -1;
    sm64dx_safe_copy(pack->id, sizeof(pack->id), dirName);
    sm64dx_safe_copy(pack->name, sizeof(pack->name), leafName);
    sm64dx_safe_copy(pack->author, sizeof(pack->author), "Unknown");
    sm64dx_safe_copy(pack->description, sizeof(pack->description), "Curated MoonOS character pack.");
    sm64dx_safe_copy(pack->compatibility, sizeof(pack->compatibility), "Native");
    sm64dx_safe_copy(pack->sourceId, sizeof(pack->sourceId), leafName);
    sm64dx_safe_copy(pack->baseCharacter, sizeof(pack->baseCharacter), "Mario");
    sm64dx_safe_copy(pack->characterName, sizeof(pack->characterName), leafName);

    snprintf(iniPath, sizeof(iniPath), "%s/pack.ini", packDir);
    if (!fs_sys_file_exists(iniPath)) {
        snprintf(iniPath, sizeof(iniPath), "%s/moonos.ini", packDir);
    }
    if (fs_sys_file_exists(iniPath)) {
        ini = ini_load(iniPath);
    }

    if (ini != NULL) {
        value = ini_get(ini, "PACK", "id");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->id, sizeof(pack->id), value);
        }

        value = ini_get(ini, "PACK", "name");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->name, sizeof(pack->name), value);
        }

        value = ini_get(ini, "PACK", "author");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->author, sizeof(pack->author), value);
        }

        value = ini_get(ini, "PACK", "description");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->description, sizeof(pack->description), value);
        }

        value = ini_get(ini, "PACK", "compatibility");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->compatibility, sizeof(pack->compatibility), value);
        }

        value = ini_get(ini, "PACK", "palette");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->paletteName, sizeof(pack->paletteName), value);
        }

        value = ini_get(ini, "PACK", "tags");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->tags, sizeof(pack->tags), value);
        }

        value = ini_get(ini, "PACK", "source");
        pack->sourceType = sm64dx_parse_source_type(value);

        value = ini_get(ini, "PACK", "source_id");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->sourceId, sizeof(pack->sourceId), value);
        }

        value = ini_get(ini, "PACK", "dynos_pack");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->sourceId, sizeof(pack->sourceId), value);
            pack->sourceType = SM64DX_MOONOS_SOURCE_DYNOS;
        }

        value = ini_get(ini, "PACK", "character");
        if (value != NULL && value[0] != '\0') {
            if (pack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE) {
                sm64dx_safe_copy(pack->sourceId, sizeof(pack->sourceId), value);
            }
            sm64dx_safe_copy(pack->baseCharacter, sizeof(pack->baseCharacter), value);
        }

        value = ini_get(ini, "PACK", "base_character");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->baseCharacter, sizeof(pack->baseCharacter), value);
        }

        value = ini_get(ini, "PACK", "character_name");
        if (value != NULL && value[0] != '\0') {
            sm64dx_safe_copy(pack->characterName, sizeof(pack->characterName), value);
        }

        value = ini_get(ini, "PACK", "preview_character");
        if (value != NULL && value[0] != '\0') {
            pack->previewCharacterIndex = sm64dx_resolve_character_index(value);
        }

        value = ini_get(ini, "FEATURES", "lua");
        pack->hasLuaScript = pack->hasLuaScript || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "dynos");
        pack->hasDynosAssets = pack->hasDynosAssets || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "voices");
        pack->hasVoices = pack->hasVoices || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "life_icon");
        pack->hasLifeIcon = pack->hasLifeIcon || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "health_meter");
        pack->hasHealthMeter = pack->hasHealthMeter || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "moveset");
        pack->hasMoveset = pack->hasMoveset || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "animations");
        pack->hasAnimations = pack->hasAnimations || sm64dx_value_is_true(value);
        value = ini_get(ini, "FEATURES", "attacks");
        pack->hasAttacks = pack->hasAttacks || sm64dx_value_is_true(value);

        ini_free(ini);
    }

    sm64dx_detect_pack_features(packDir, pack);

    if (pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS) {
        if (pack->compatibility[0] == '\0' || !sys_strcasecmp(pack->compatibility, "Native")) {
            sm64dx_safe_copy(pack->compatibility, sizeof(pack->compatibility), "DynOS");
        }
    } else {
        sm64dx_safe_copy(pack->sourceId, sizeof(pack->sourceId),
                         (pack->sourceId[0] != '\0') ? pack->sourceId : pack->baseCharacter);
    }

    if (pack->baseCharacter[0] == '\0') {
        sm64dx_safe_copy(pack->baseCharacter, sizeof(pack->baseCharacter),
                         (pack->sourceType == SM64DX_MOONOS_SOURCE_NATIVE) ? pack->sourceId : "Mario");
    }

    if (pack->characterName[0] == '\0') {
        sm64dx_safe_copy(pack->characterName, sizeof(pack->characterName), pack->baseCharacter);
    }

    pack->nativeCharacterIndex = sm64dx_resolve_character_index(pack->baseCharacter);
    if (pack->previewCharacterIndex >= CT_MAX) {
        pack->previewCharacterIndex = pack->nativeCharacterIndex;
    }

    if (!pack->hasDynosAssets && pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS) {
        pack->hasDynosAssets = true;
    }
}

static void sm64dx_add_pack_from_path(const char *basePath, const char *dirName) {
    char packDir[SYS_MAX_PATH] = { 0 };
    int packIndex = -1;

    if (basePath == NULL || dirName == NULL || dirName[0] == '\0') {
        return;
    }

    if (snprintf(packDir, sizeof(packDir), "%s/%s", basePath, dirName) < 0) {
        return;
    }
    if (!fs_sys_dir_exists(packDir) || !sm64dx_directory_looks_like_pack(packDir)) {
        return;
    }

    packIndex = sm64dx_find_pack_index_by_id(dirName);
    if (packIndex < 0) {
        if (sSm64dx.packCount >= SM64DX_MAX_MOONOS_PACKS) {
            return;
        }
        packIndex = sSm64dx.packCount++;
    }

    sm64dx_load_pack_metadata(dirName, packDir, &sSm64dx.packs[packIndex]);
}

static void sm64dx_scan_pack_directory_recursive(const char *basePath, const char *relativePath) {
    char scanPath[SYS_MAX_PATH] = { 0 };
    DIR *directory = NULL;
    struct dirent *entry = NULL;

    if (basePath == NULL || !fs_sys_dir_exists(basePath)) {
        return;
    }

    if (relativePath != NULL && relativePath[0] != '\0') {
        snprintf(scanPath, sizeof(scanPath), "%s/%s", basePath, relativePath);
    } else {
        snprintf(scanPath, sizeof(scanPath), "%s", basePath);
    }

    if (!fs_sys_dir_exists(scanPath)) {
        return;
    }

    directory = opendir(scanPath);
    if (directory == NULL) {
        return;
    }

    while ((entry = readdir(directory)) != NULL) {
        char childPath[SYS_MAX_PATH] = { 0 };
        char childRelative[SYS_MAX_PATH] = { 0 };

        if (sm64dx_should_ignore_pack_directory(entry->d_name)) {
            continue;
        }

        if (relativePath != NULL && relativePath[0] != '\0') {
            if (snprintf(childRelative, sizeof(childRelative), "%s/%s", relativePath, entry->d_name) < 0) {
                continue;
            }
        } else {
            if (snprintf(childRelative, sizeof(childRelative), "%s", entry->d_name) < 0) {
                continue;
            }
        }

        if (snprintf(childPath, sizeof(childPath), "%s/%s", basePath, childRelative) < 0) {
            continue;
        }
        if (!fs_sys_dir_exists(childPath)) {
            continue;
        }

        if (sm64dx_directory_looks_like_pack(childPath)) {
            sm64dx_add_pack_from_path(basePath, childRelative);
            continue;
        }

        sm64dx_scan_pack_directory_recursive(basePath, childRelative);
    }

    closedir(directory);
}

static void sm64dx_scan_pack_directory(const char *basePath) {
    sm64dx_scan_pack_directory_recursive(basePath, "");
}

static void sm64dx_build_user_moonos_root(char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        return;
    }

    char userMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(userMoonos, sizeof(userMoonos), "%s", fs_get_write_path(SM64DX_MOONOS_DIRECTORY));
    sm64dx_ensure_directory_exists(userMoonos);

    snprintf(buffer, bufferSize, "%s/packs", userMoonos);
    sm64dx_ensure_directory_exists(buffer);
}

static void sm64dx_import_dynos_packs(void) {
    int dynosCount = dynos_pack_get_count();
    for (int i = 0; i < dynosCount; i++) {
        const char *name = dynos_pack_get_name(i);
        int packIndex = -1;

        if (name == NULL || name[0] == '\0' || !dynos_pack_get_exists(i)) {
            continue;
        }

        packIndex = sm64dx_find_pack_index_by_id(name);
        if (packIndex < 0) {
            packIndex = sm64dx_find_pack_index_by_source_id(SM64DX_MOONOS_SOURCE_DYNOS, name);
        }
        if (packIndex < 0) {
            if (sSm64dx.packCount >= SM64DX_MAX_MOONOS_PACKS) {
                continue;
            }

            packIndex = sSm64dx.packCount++;
            memset(&sSm64dx.packs[packIndex], 0, sizeof(sSm64dx.packs[packIndex]));
            sm64dx_safe_copy(sSm64dx.packs[packIndex].id, sizeof(sSm64dx.packs[packIndex].id), name);
            sm64dx_safe_copy(sSm64dx.packs[packIndex].name, sizeof(sSm64dx.packs[packIndex].name), name);
            sm64dx_safe_copy(sSm64dx.packs[packIndex].author, sizeof(sSm64dx.packs[packIndex].author), "Unknown");
            sm64dx_safe_copy(sSm64dx.packs[packIndex].description, sizeof(sSm64dx.packs[packIndex].description),
                             "DynOS pack imported into MoonOS without metadata.");
            sm64dx_safe_copy(sSm64dx.packs[packIndex].compatibility, sizeof(sSm64dx.packs[packIndex].compatibility), "DynOS Raw");
            sm64dx_safe_copy(sSm64dx.packs[packIndex].sourceId, sizeof(sSm64dx.packs[packIndex].sourceId), name);
            sm64dx_safe_copy(sSm64dx.packs[packIndex].baseCharacter, sizeof(sSm64dx.packs[packIndex].baseCharacter), "Mario");
            sm64dx_safe_copy(sSm64dx.packs[packIndex].characterName, sizeof(sSm64dx.packs[packIndex].characterName), "Mario");
            sSm64dx.packs[packIndex].sourceType = SM64DX_MOONOS_SOURCE_DYNOS;
            sSm64dx.packs[packIndex].nativeCharacterIndex = CT_MARIO;
            sSm64dx.packs[packIndex].previewCharacterIndex = CT_MARIO;
        }

        sSm64dx.packs[packIndex].sourceType = SM64DX_MOONOS_SOURCE_DYNOS;
        sSm64dx.packs[packIndex].dynosPackIndex = i;
        sSm64dx.packs[packIndex].hasDynosAssets = true;
        if (sSm64dx.packs[packIndex].sourceId[0] == '\0') {
            sm64dx_safe_copy(sSm64dx.packs[packIndex].sourceId, sizeof(sSm64dx.packs[packIndex].sourceId), name);
        }
    }
}

static void sm64dx_refresh_moonos_packs(void) {
    memset(sSm64dx.packs, 0, sizeof(sSm64dx.packs));
    sSm64dx.packCount = 0;

    char packageMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(packageMoonos, sizeof(packageMoonos), "%s/%s", sys_package_path(), SM64DX_MOONOS_PACKS_DIRECTORY);
    sm64dx_scan_pack_directory(packageMoonos);

    char packageLegacyMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(packageLegacyMoonos, sizeof(packageLegacyMoonos), "%s/%s", sys_package_path(), SM64DX_MOONOS_LEGACY_MODS_DIRECTORY);
    sm64dx_scan_pack_directory(packageLegacyMoonos);

    char packageLegacyDynos[SYS_MAX_PATH] = { 0 };
    snprintf(packageLegacyDynos, sizeof(packageLegacyDynos), "%s/dynos/packs", sys_package_path());
    sm64dx_scan_pack_directory(packageLegacyDynos);

    char resourceMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(resourceMoonos, sizeof(resourceMoonos), "%s/%s", sys_resource_path(), SM64DX_MOONOS_PACKS_DIRECTORY);
    sm64dx_scan_pack_directory(resourceMoonos);

    char resourceLegacyMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(resourceLegacyMoonos, sizeof(resourceLegacyMoonos), "%s/%s", sys_resource_path(), SM64DX_MOONOS_DIRECTORY);
    sm64dx_scan_pack_directory(resourceLegacyMoonos);

    char userMoonos[SYS_MAX_PATH] = { 0 };
    sm64dx_build_user_moonos_root(userMoonos, sizeof(userMoonos));
    sm64dx_scan_pack_directory(userMoonos);

    char userLegacyMoonos[SYS_MAX_PATH] = { 0 };
    snprintf(userLegacyMoonos, sizeof(userLegacyMoonos), "%s/%s", fs_get_write_path(MOD_DIRECTORY), SM64DX_MOONOS_DIRECTORY "/packs");
    sm64dx_scan_pack_directory(userLegacyMoonos);

    char userLegacyMoonosRoot[SYS_MAX_PATH] = { 0 };
    snprintf(userLegacyMoonosRoot, sizeof(userLegacyMoonosRoot), "%s", fs_get_write_path(SM64DX_MOONOS_DIRECTORY));
    sm64dx_scan_pack_directory(userLegacyMoonosRoot);

    sm64dx_import_dynos_packs();
    sm64dx_sync_pack_flags();
    sm64dx_sort_packs();
    sSm64dx.packCatalogRefreshRequested = false;
}

static void sm64dx_ensure_pack_catalog(void) {
    if (sSm64dx.packCatalogRefreshRequested) {
        sm64dx_refresh_moonos_packs();
    }
}

void sm64dx_profile_init(void) {
    if (sSm64dx.initialized) {
        return;
    }

    sm64dx_reset_save_profile(&sSm64dx.globalDefault);
    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        sm64dx_reset_save_profile(&sSm64dx.saves[i]);
    }

    sSm64dx.initialized = true;
    sSm64dx.lastClock = clock_elapsed_f64();
    sSm64dx.lastFlushClock = sSm64dx.lastClock;
    sSm64dx.packCatalogRefreshRequested = true;

    sm64dx_read_profile();
}

void sm64dx_profile_shutdown(void) {
    if (!sSm64dx.initialized) {
        return;
    }

    if (sSm64dx.dirty) {
        sm64dx_profile_save();
    }
}

void sm64dx_profile_update(void) {
    if (!sSm64dx.initialized) {
        return;
    }

    f64 now = clock_elapsed_f64();
    f64 delta = now - sSm64dx.lastClock;
    sSm64dx.lastClock = now;

    if (delta < 0.0 || delta > 5.0) {
        delta = 0.0;
    }

    if (!gDjuiInMainMenu && gCurrSaveFileNum >= 1 && gCurrSaveFileNum <= NUM_SAVE_FILES) {
        int slotIndex = gCurrSaveFileNum - 1;
        sSm64dx.playFractions[slotIndex] += delta;
        while (sSm64dx.playFractions[slotIndex] >= 1.0) {
            sSm64dx.saves[slotIndex].playSeconds++;
            sSm64dx.playFractions[slotIndex] -= 1.0;
            sSm64dx.dirty = true;
        }
    }

    if (sSm64dx.dirty && now - sSm64dx.lastFlushClock >= 3.0) {
        sm64dx_profile_save();
    }
}

void sm64dx_build_save_summary(int slot, struct Sm64dxSaveSummary *outSummary) {
    if (outSummary == NULL) {
        return;
    }

    memset(outSummary, 0, sizeof(*outSummary));
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return;
    }

    int slotIndex = slot - 1;
    u32 flags = sm64dx_get_save_flags_for_slot(slotIndex);
    int totalStars = save_file_get_total_star_count(slotIndex, COURSE_MIN - 1, COURSE_MAX - 1);
    int hundredCoinStars = sm64dx_count_100_coin_stars(slotIndex);
    int capCount = sm64dx_count_caps(flags);
    int keyCount = sm64dx_count_keys(flags);
    char slotLetter = 'A' + slotIndex;
    const char *saveName = configSaveNames[slotIndex][0] != '\0' ? configSaveNames[slotIndex] : "SM64DX";
    const char *packName = sm64dx_get_save_pack_name(slot);

    outSummary->exists = save_file_exists(slotIndex);
    snprintf(outSummary->title, sizeof(outSummary->title), "File %c", slotLetter);
    snprintf(outSummary->name, sizeof(outSummary->name), "%s", outSummary->exists ? saveName : "New Adventure");
    snprintf(outSummary->action, sizeof(outSummary->action), "%s", outSummary->exists ? "Continue" : "New Game");
    sm64dx_format_stars_line(totalStars, outSummary->starsLine, sizeof(outSummary->starsLine));
    snprintf(outSummary->progressionLine, sizeof(outSummary->progressionLine),
             "%s | 100C %d",
             sm64dx_castle_progress_label(flags, totalStars),
             hundredCoinStars);
    snprintf(outSummary->unlocksLine, sizeof(outSummary->unlocksLine),
             "Caps %d/3 | Keys %d/2",
             capCount,
             keyCount);
    snprintf(outSummary->moonosLine, sizeof(outSummary->moonosLine), "%s", packName);
    sm64dx_format_last_played(sSm64dx.saves[slotIndex].lastPlayed, outSummary->lastPlayedLine, sizeof(outSummary->lastPlayedLine));
    sm64dx_format_play_time(sSm64dx.saves[slotIndex].playSeconds, outSummary->playTimeLine, sizeof(outSummary->playTimeLine));
}

bool sm64dx_has_recent_save(void) {
    return sm64dx_get_recent_save_slot() != 0;
}

int sm64dx_get_recent_save_slot(void) {
    if (sSm64dx.lastSaveSlot < 1 || sSm64dx.lastSaveSlot > NUM_SAVE_FILES) {
        return 0;
    }
    if (!save_file_exists(sSm64dx.lastSaveSlot - 1)) {
        return 0;
    }
    return sSm64dx.lastSaveSlot;
}

static void sm64dx_disable_all_moonos_dynos_packs(void) {
    sm64dx_ensure_pack_catalog();
    for (int i = 0; i < sSm64dx.packCount; i++) {
        const struct Sm64dxMoonosPack *pack = &sSm64dx.packs[i];
        if (pack->sourceType != SM64DX_MOONOS_SOURCE_DYNOS || pack->dynosPackIndex < 0) {
            continue;
        }
        dynos_pack_set_enabled(pack->dynosPackIndex, false);
    }
}

static bool sm64dx_lua_charselect_exists(void) {
    lua_State *L = gLuaState;
    bool exists = false;

    if (L == NULL) {
        return false;
    }

    lua_getglobal(L, "charSelectExists");
    exists = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return exists;
}

static bool sm64dx_lua_charselect_get_number_from_name(const char *name, int *outCharNum) {
    lua_State *L = gLuaState;
    int top = 0;
    int rc = 0;
    bool success = false;

    if (L == NULL || name == NULL || name[0] == '\0') {
        return false;
    }

    top = lua_gettop(L);
    lua_getglobal(L, "charSelect");
    if (!lua_istable(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    lua_getfield(L, -1, "character_get_number_from_string");
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, top);
        return false;
    }

    lua_remove(L, -2);
    lua_pushstring(L, name);
    rc = smlua_pcall(L, 1, 1, 0);
    if (rc == 0 && !lua_isnil(L, -1)) {
        if (outCharNum != NULL) {
            *outCharNum = (int) lua_tointeger(L, -1);
        }
        success = true;
    }

    lua_settop(L, top);
    return success;
}

static void sm64dx_lua_charselect_set_current(int charNum) {
    lua_State *L = gLuaState;
    int top = 0;

    if (L == NULL) {
        return;
    }

    top = lua_gettop(L);
    lua_getglobal(L, "charSelect");
    if (!lua_istable(L, -1)) {
        lua_settop(L, top);
        return;
    }

    lua_getfield(L, -1, "character_set_current_number");
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, top);
        return;
    }

    lua_remove(L, -2);
    lua_pushinteger(L, charNum);
    lua_pushinteger(L, 1);
    (void) smlua_pcall(L, 2, 0, 0);
    lua_settop(L, top);
}

static void sm64dx_sync_charselect_character(const char *characterName, const char *fallbackBaseCharacter) {
    int charNum = 0;
    const char *targetName = characterName;

    if (!sm64dx_lua_charselect_exists()) {
        return;
    }

    if (targetName == NULL || targetName[0] == '\0') {
        targetName = fallbackBaseCharacter;
    }
    if (targetName == NULL || targetName[0] == '\0') {
        return;
    }

    if (sm64dx_lua_charselect_get_number_from_name(targetName, &charNum)) {
        sm64dx_lua_charselect_set_current(charNum);
    }
}

static void sm64dx_apply_profile_visuals(const struct Sm64dxSaveProfile *profile) {
    int baseCharacterIndex = CT_MARIO;
    int dynosPackIndex = -1;

    if (profile == NULL) {
        sm64dx_apply_live_player_settings();
        return;
    }

    baseCharacterIndex = sm64dx_resolve_character_index(profile->baseCharacter);
    configPlayerModel = baseCharacterIndex;
    configPlayerPalette = profile->palette;

    sm64dx_disable_all_moonos_dynos_packs();

    if (!sys_strcasecmp(profile->sourceType, "dynos")) {
        dynosPackIndex = sm64dx_find_dynos_pack_index_by_name(profile->sourceId);
        if (dynosPackIndex >= 0) {
            dynos_pack_set_enabled(dynosPackIndex, true);
        }
    }

    sm64dx_apply_live_player_settings();
    sm64dx_sync_charselect_character(profile->characterName, profile->baseCharacter);
}

void sm64dx_apply_save_setup(int slot) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        sm64dx_apply_live_player_settings();
        return;
    }

    sm64dx_ensure_pack_catalog();
    sm64dx_apply_profile_visuals(&sSm64dx.saves[slot - 1]);
}

void sm64dx_capture_current_setup(int slot) {
    struct Sm64dxSaveProfile *profile = NULL;
    int baseCharacterIndex = CT_MARIO;

    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return;
    }

    sm64dx_ensure_palette_cache();

    profile = &sSm64dx.saves[slot - 1];
    profile->palette = configPlayerPalette;
    sm64dx_find_palette_name_for_current(profile->paletteName, sizeof(profile->paletteName));

    if (profile->sourceType[0] == '\0') {
        sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), "native");
    }

    if (profile->moonosPackId[0] == '\0') {
        baseCharacterIndex = (configPlayerModel < CT_MAX) ? configPlayerModel : CT_MARIO;
        sm64dx_safe_copy(profile->sourceType, sizeof(profile->sourceType), "native");
        sm64dx_safe_copy(profile->sourceId, sizeof(profile->sourceId), sm64dx_character_name_from_index(baseCharacterIndex));
        sm64dx_safe_copy(profile->baseCharacter, sizeof(profile->baseCharacter), sm64dx_character_name_from_index(baseCharacterIndex));
        sm64dx_safe_copy(profile->characterName, sizeof(profile->characterName), sm64dx_character_name_from_index(baseCharacterIndex));
    }

    sSm64dx.dirty = true;
}

static void sm64dx_mark_pack_recent(const char *packId) {
    if (packId == NULL || packId[0] == '\0') {
        return;
    }

    sm64dx_array_promote_front(sSm64dx.recentPackIds, SM64DX_MAX_PINNED_ITEMS, packId);
    sSm64dx.dirty = true;
    sm64dx_sync_pack_flags();
    sm64dx_sort_packs();
}

void sm64dx_start_save_slot(int slot, bool playSound) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return;
    }

    stop_demo(NULL);
    configHostSaveSlot = slot;
    sm64dx_apply_save_setup(slot);
    sSm64dx.lastSaveSlot = slot;
    sSm64dx.saves[slot - 1].lastPlayed = (u64) time(NULL);
    sSm64dx.dirty = true;

    djui_panel_shutdown();
    gCurrSaveFileNum = slot;
    gCurrActNum = 0;
    gCurrActStarNum = 0;
    update_all_mario_stars();
    fake_lvl_init_from_save_file();
    gChangeLevel = gLevelValues.entryLevel;
    if (playSound) {
        gDelayedInitSound = CHAR_SOUND_OKEY_DOKEY;
    }
}

void sm64dx_copy_save_slot(int srcSlot, int dstSlot) {
    srcSlot = sm64dx_clamp_slot(srcSlot);
    dstSlot = sm64dx_clamp_slot(dstSlot);
    if (srcSlot == 0 || dstSlot == 0 || srcSlot == dstSlot) {
        return;
    }

    save_file_copy(srcSlot - 1, dstSlot - 1);
    sSm64dx.saves[dstSlot - 1] = sSm64dx.saves[srcSlot - 1];
    snprintf(configSaveNames[dstSlot - 1], MAX_SAVE_NAME_STRING, "%s", configSaveNames[srcSlot - 1]);
    sSm64dx.dirty = true;
}

void sm64dx_erase_save_slot(int slot) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return;
    }

    save_file_erase(slot - 1);
    sm64dx_reset_save_profile(&sSm64dx.saves[slot - 1]);
    sSm64dx.playFractions[slot - 1] = 0.0;
    if (sSm64dx.lastSaveSlot == slot) {
        sSm64dx.lastSaveSlot = 0;
    }
    sSm64dx.dirty = true;
}

const char *sm64dx_get_save_pack_name(int slot) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return "Mario";
    }

    return sm64dx_get_profile_pack_name(&sSm64dx.saves[slot - 1]);
}

int sm64dx_get_moonos_pack_count(void) {
    sm64dx_ensure_pack_catalog();
    return sSm64dx.packCount;
}

const struct Sm64dxMoonosPack *sm64dx_get_moonos_pack(int index) {
    sm64dx_ensure_pack_catalog();
    if (index < 0 || index >= sSm64dx.packCount) {
        return NULL;
    }
    return &sSm64dx.packs[index];
}

int sm64dx_get_selected_moonos_pack_index(int slot) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return -1;
    }

    sm64dx_ensure_pack_catalog();

    const struct Sm64dxSaveProfile *profile = &sSm64dx.saves[slot - 1];
    int packIndex = sm64dx_find_pack_index_by_id(profile->moonosPackId);
    if (packIndex >= 0) {
        return packIndex;
    }

    for (int i = 0; i < sSm64dx.packCount; i++) {
        const struct Sm64dxMoonosPack *pack = &sSm64dx.packs[i];

        if (pack->sourceType == SM64DX_MOONOS_SOURCE_DYNOS) {
            if (sys_strcasecmp(profile->sourceType, "dynos")) {
                continue;
            }
            if (!sys_strcasecmp(pack->sourceId, profile->sourceId)) {
                return i;
            }
        } else {
            if (sys_strcasecmp(profile->sourceType, "native")) {
                continue;
            }
            if (pack->nativeCharacterIndex != sm64dx_resolve_character_index(profile->baseCharacter)) {
                continue;
            }
            if (pack->paletteName[0] != '\0' && profile->paletteName[0] != '\0'
                && sys_strcasecmp(pack->paletteName, profile->paletteName)) {
                continue;
            }
            if (pack->characterName[0] != '\0' && profile->characterName[0] != '\0'
                && sys_strcasecmp(pack->characterName, profile->characterName)) {
                continue;
            }
            return i;
        }
    }

    return -1;
}

void sm64dx_apply_moonos_pack(int slot, int packIndex) {
    struct Sm64dxSaveProfile *profile = NULL;
    const struct Sm64dxMoonosPack *pack = NULL;
    char fallbackPaletteName[64] = { 0 };

    slot = sm64dx_clamp_slot(slot);
    sm64dx_ensure_pack_catalog();
    if (slot == 0 || packIndex < 0 || packIndex >= sSm64dx.packCount) {
        return;
    }

    sm64dx_ensure_palette_cache();

    profile = &sSm64dx.saves[slot - 1];
    pack = &sSm64dx.packs[packIndex];

    sm64dx_find_palette_name_for_current(fallbackPaletteName, sizeof(fallbackPaletteName));
    configPlayerModel = pack->nativeCharacterIndex;
    sm64dx_assign_pack_to_profile(profile, pack, &configPlayerPalette, fallbackPaletteName);
    sm64dx_apply_profile_visuals(profile);
    sm64dx_mark_pack_recent(pack->id);
    sSm64dx.dirty = true;
}

void sm64dx_toggle_moonos_favorite(int packIndex) {
    if (packIndex < 0 || packIndex >= sSm64dx.packCount) {
        return;
    }

    const char *packId = sSm64dx.packs[packIndex].id;
    if (sm64dx_array_contains(sSm64dx.favoritePackIds, SM64DX_MAX_PINNED_ITEMS, packId, NULL)) {
        sm64dx_array_remove(sSm64dx.favoritePackIds, SM64DX_MAX_PINNED_ITEMS, packId);
    } else {
        sm64dx_array_promote_front(sSm64dx.favoritePackIds, SM64DX_MAX_PINNED_ITEMS, packId);
    }

    sSm64dx.dirty = true;
    sm64dx_sync_pack_flags();
    sm64dx_sort_packs();
}

bool sm64dx_is_active_moonos_pack(int slot, int packIndex) {
    return sm64dx_get_selected_moonos_pack_index(slot) == packIndex;
}

bool sm64dx_has_global_default_moonos(void) {
    return sSm64dx.hasGlobalDefaultProfile;
}

const char *sm64dx_get_global_default_pack_name(void) {
    return sm64dx_get_profile_pack_name(&sSm64dx.globalDefault);
}

void sm64dx_set_global_default_moonos_pack(int slot, int packIndex) {
    char fallbackPaletteName[64] = { 0 };

    slot = sm64dx_clamp_slot(slot);
    sm64dx_ensure_pack_catalog();
    if (slot == 0 || packIndex < 0 || packIndex >= sSm64dx.packCount) {
        return;
    }

    sm64dx_assign_pack_to_profile(&sSm64dx.globalDefault,
                                  &sSm64dx.packs[packIndex],
                                  &sSm64dx.saves[slot - 1].palette,
                                  sSm64dx.saves[slot - 1].paletteName);
    if (sSm64dx.globalDefault.paletteName[0] == '\0') {
        sm64dx_safe_copy(fallbackPaletteName, sizeof(fallbackPaletteName), sSm64dx.saves[slot - 1].paletteName);
        sm64dx_safe_copy(sSm64dx.globalDefault.paletteName, sizeof(sSm64dx.globalDefault.paletteName), fallbackPaletteName);
    }
    sSm64dx.hasGlobalDefaultProfile = true;
    sSm64dx.dirty = true;
}

void sm64dx_reset_save_to_global_default(int slot) {
    slot = sm64dx_clamp_slot(slot);
    if (slot == 0) {
        return;
    }

    if (sSm64dx.hasGlobalDefaultProfile) {
        u64 lastPlayed = sSm64dx.saves[slot - 1].lastPlayed;
        u64 playSeconds = sSm64dx.saves[slot - 1].playSeconds;
        sSm64dx.saves[slot - 1] = sSm64dx.globalDefault;
        sSm64dx.saves[slot - 1].lastPlayed = lastPlayed;
        sSm64dx.saves[slot - 1].playSeconds = playSeconds;
    } else {
        u64 lastPlayed = sSm64dx.saves[slot - 1].lastPlayed;
        u64 playSeconds = sSm64dx.saves[slot - 1].playSeconds;
        sm64dx_reset_save_profile(&sSm64dx.saves[slot - 1]);
        sSm64dx.saves[slot - 1].lastPlayed = lastPlayed;
        sSm64dx.saves[slot - 1].playSeconds = playSeconds;
    }

    sm64dx_apply_save_setup(slot);
    sSm64dx.dirty = true;
}
