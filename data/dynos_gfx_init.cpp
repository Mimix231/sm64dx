#include "dynos.cpp.h"
extern "C" {
#include "pc/loading.h"
}

#define MOD_PATH_LEN 1024

static bool DynOS_ShouldSkipDir(const char *name) {
    return (name == NULL || name[0] == '\0' || name[0] == '.' || name[0] == '_');
}

static bool DynOS_DirHasTopLevelExtension(const SysPath& dirPath, const char *extension) {
    DIR *directory = opendir(dirPath.c_str());
    if (!directory) {
        return false;
    }

    bool found = false;
    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (DynOS_ShouldSkipDir(entry->d_name)) {
            continue;
        }

        s32 nameLength = strlen(entry->d_name);
        s32 extLength = strlen(extension);
        if (nameLength >= extLength && !strncmp(&entry->d_name[nameLength - extLength], extension, extLength)) {
            found = true;
            break;
        }
    }

    closedir(directory);
    return found;
}

static bool DynOS_DirLooksLikePack(const SysPath& path) {
    SysPath _PackIni = fstring("%s/pack.ini", path.c_str());
    if (fs_sys_file_exists(_PackIni.c_str())) {
        return true;
    }

    SysPath _MoonosIni = fstring("%s/moonos.ini", path.c_str());
    if (fs_sys_file_exists(_MoonosIni.c_str())) {
        return true;
    }

    SysPath _MainLua = fstring("%s/main.lua", path.c_str());
    if (fs_sys_file_exists(_MainLua.c_str())) {
        return true;
    }

    if (fs_sys_dir_exists(fstring("%s/actors", path.c_str()).c_str())) { return true; }
    if (fs_sys_dir_exists(fstring("%s/assets", path.c_str()).c_str())) { return true; }
    if (fs_sys_dir_exists(fstring("%s/levels", path.c_str()).c_str())) { return true; }
    if (fs_sys_dir_exists(fstring("%s/data", path.c_str()).c_str())) { return true; }

    return DynOS_DirHasTopLevelExtension(path, ".bin")
        || DynOS_DirHasTopLevelExtension(path, ".tex");
}

void DynOS_Gfx_GenerateModPacks(char* modPath) {
    // If pack folder exists, generate bins
    SysPath _LevelPackFolder = fstring("%s/levels", modPath);
    if (fs_sys_dir_exists(_LevelPackFolder.c_str())) {
        DynOS_Lvl_GeneratePack(_LevelPackFolder);
    }

    SysPath _ActorPackFolder = fstring("%s/actors", modPath);
    if (fs_sys_dir_exists(_ActorPackFolder.c_str())) {
        DynOS_Actor_GeneratePack(_ActorPackFolder);
    }

    SysPath _BehaviorPackFolder = fstring("%s/data", modPath);
    if (fs_sys_dir_exists(_BehaviorPackFolder.c_str())) {
        DynOS_Bhv_GeneratePack(_BehaviorPackFolder);
    }

    SysPath _TexturePackFolder = fstring("%s", modPath);
    SysPath _TexturePackOutputFolder = fstring("%s/textures", modPath);
    if (fs_sys_dir_exists(_TexturePackFolder.c_str())) {
        DynOS_Tex_GeneratePack(_TexturePackFolder, _TexturePackOutputFolder, true);
    }
}

static void DynOS_Gfx_GeneratePackTree(const SysPath& directory) {
    if (!fs_sys_dir_exists(directory.c_str())) {
        return;
    }

    if (DynOS_DirLooksLikePack(directory)) {
        static char sPackPath[MOD_PATH_LEN] = "";
        snprintf(sPackPath, MOD_PATH_LEN, "%s", directory.c_str());
        DynOS_Gfx_GenerateModPacks(sPackPath);
        return;
    }

    DIR *folder = opendir(directory.c_str());
    if (!folder) {
        return;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(folder)) != NULL) {
        if (DynOS_ShouldSkipDir(entry->d_name)) continue;

        SysPath childPath = fstring("%s/%s", directory.c_str(), entry->d_name);
        if (!fs_sys_dir_exists(childPath.c_str())) {
            continue;
        }

        DynOS_Gfx_GeneratePackTree(childPath);
    }

    closedir(folder);
}

void DynOS_Gfx_GeneratePacks(const char* directory) {
    if (configSkipPackGeneration) { return; }

    LOADING_SCREEN_MUTEX(
        loading_screen_reset_progress_bar();
        snprintf(gCurrLoadingSegment.str, 256, "Generating DynOS Packs In Path:\n\\#808080\\%s", directory);
    );

    DIR *modsDir = opendir(directory);
    if (!modsDir) { return; }

    struct dirent *dir = NULL;
    DIR* d = opendir(directory);
    u32 pathCount = 0;
    while ((dir = readdir(d)) != NULL) pathCount++;
    closedir(d);

    for (u32 i = 0; (dir = readdir(modsDir)) != NULL; ++i) {
        if (DynOS_ShouldSkipDir(dir->d_name)) continue;

        SysPath modPath = fstring("%s/%s", directory, dir->d_name);
        if (!fs_sys_dir_exists(modPath.c_str())) {
            continue;
        }

        DynOS_Gfx_GeneratePackTree(modPath);
        LOADING_SCREEN_MUTEX(gCurrLoadingSegment.percentage = (f32) i / (f32) pathCount);
    }

    closedir(modsDir);
}

static void ScanPacksFolder(SysPath _DynosPacksFolder) {
    DIR *_DynosPacksDir = opendir(_DynosPacksFolder.c_str());
    if (!_DynosPacksDir) {
        return;
    }

    struct dirent *_DynosPacksEnt = NULL;
    while ((_DynosPacksEnt = readdir(_DynosPacksDir)) != NULL) {
        if (DynOS_ShouldSkipDir(_DynosPacksEnt->d_name)) continue;

        SysPath _PackFolder = fstring("%s/%s", _DynosPacksFolder.c_str(), _DynosPacksEnt->d_name);
        if (!fs_sys_dir_exists(_PackFolder.c_str())) {
            continue;
        }

        if (DynOS_DirLooksLikePack(_PackFolder)) {
            LOADING_SCREEN_MUTEX(snprintf(gCurrLoadingSegment.str, 256, "Generating DynOS Pack:\n\\#808080\\%s", _PackFolder.c_str()));
            DynOS_Pack_Add(_PackFolder);
            DynOS_Actor_GeneratePack(_PackFolder);
            DynOS_Tex_GeneratePack(_PackFolder, _PackFolder, false);
            continue;
        }

        ScanPacksFolder(_PackFolder);
    }

    closedir(_DynosPacksDir);
}

void DynOS_Gfx_Init() {
    // Scan the DynOS packs folder
    SysPath _DynosPacksFolder = fstring("%s/%s", DYNOS_EXE_FOLDER, DYNOS_PACKS_FOLDER);
    ScanPacksFolder(_DynosPacksFolder);

    // Scan the user path folder
    SysPath _DynosPacksUserFolder = fstring("%s%s", DYNOS_USER_FOLDER, DYNOS_PACKS_FOLDER);
    ScanPacksFolder(_DynosPacksUserFolder);
}
