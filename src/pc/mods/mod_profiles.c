#include <stdio.h>
#include <string.h>

#include "mods.h"
#include "mods_utils.h"
#include "mod_profiles.h"
#include "pc/debuglog.h"
#include "pc/fs/fs.h"

#define MOD_PROFILE_DIRECTORY "mod-profiles"

static bool mods_profile_directory_path(char* destination, size_t destinationSize) {
    return snprintf(destination, destinationSize, "%s/%s", fs_writepath, MOD_PROFILE_DIRECTORY) >= 0;
}

static bool mods_profile_file_path(char* destination, size_t destinationSize, const char* profileId) {
    char directoryPath[SYS_MAX_PATH] = { 0 };
    if (!mods_profile_directory_path(directoryPath, sizeof(directoryPath))) {
        return false;
    }
    return snprintf(destination, destinationSize, "%s/%s.txt", directoryPath, profileId) >= 0;
}

static bool mods_profile_ensure_directory(void) {
    char directoryPath[SYS_MAX_PATH] = { 0 };
    if (!mods_profile_directory_path(directoryPath, sizeof(directoryPath))) {
        return false;
    }
    if (fs_sys_dir_exists(directoryPath)) {
        return true;
    }
    return fs_sys_mkdir(directoryPath);
}

bool mods_profile_exists(const char* profileId) {
    char profilePath[SYS_MAX_PATH] = { 0 };
    if (!mods_profile_file_path(profilePath, sizeof(profilePath), profileId)) {
        return false;
    }
    return fs_sys_file_exists(profilePath);
}

bool mods_profile_save(const char* profileId) {
    if (!mods_profile_ensure_directory()) {
        LOG_ERROR("Could not create mod profile directory");
        return false;
    }

    char profilePath[SYS_MAX_PATH] = { 0 };
    if (!mods_profile_file_path(profilePath, sizeof(profilePath), profileId)) {
        return false;
    }

    FILE* file = fopen(profilePath, "w");
    if (file == NULL) {
        LOG_ERROR("Could not save mod profile '%s'", profileId);
        return false;
    }

    fprintf(file, "# SM64 DX mod profile\n");
    for (unsigned int i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* mod = gLocalMods.entries[i];
        if (mod == NULL || !mod->enabled) { continue; }
        fprintf(file, "%s\n", mod->relativePath);
    }

    fclose(file);
    return true;
}

bool mods_profile_load(const char* profileId) {
    char profilePath[SYS_MAX_PATH] = { 0 };
    if (!mods_profile_file_path(profilePath, sizeof(profilePath), profileId)) {
        return false;
    }

    FILE* file = fopen(profilePath, "r");
    if (file == NULL) {
        return false;
    }

    mods_disable_all();

    char line[SYS_MAX_PATH] = { 0 };
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t length = strcspn(line, "\r\n");
        line[length] = '\0';
        if (line[0] == '\0' || line[0] == '#') { continue; }
        mods_enable(line);
    }

    fclose(file);
    mods_update_selectable();
    return true;
}

void mods_profile_save_last_session(void) {
    mods_profile_save(MOD_PROFILE_LAST_SESSION);
}
