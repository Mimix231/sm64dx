#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

extern "C" {
#include "platform.h"
#include "mods/mods_utils.h" // for path_ends_with
#include "mods/mod_cache.h"  // for md5 hashing
#include "mods/mods.h"
#include "loading.h"
#include "fs/fs.h"
}

namespace fs = std::filesystem;

bool gRomIsValid = false;
char gRomFilename[SYS_MAX_PATH] = "";
static bool sRomIsCustomHack = false;
static std::string sActiveRomDisplayName = "Super Mario 64";
static std::string sActiveRomSelectionPath;

struct VanillaMD5 {
    const char *localizationName;
    const char *md5;
};

// lookup table for vanilla sm64 roms
static struct VanillaMD5 sVanillaMD5[] = {
    // { "eu", "45676429ef6b90e65b517129b700308e" },
    // { "jp", "85d61f5525af708c9f1e84dce6dc10e9" },
    // { "sh", "2d727c3278aa232d94f2fb45aec4d303" },
    { "us", "20b854b239203baf6c961b850a4a51a2" },
    { NULL, NULL },
};

static constexpr u8 ROM_MAGIC_Z64[] = { 0x80, 0x37, 0x12, 0x40 };
static constexpr const char *ROM_HACK_DIRECTORY = "romhacks";
static constexpr const char *ROM_SELECTION_FILENAME = "active-rom.txt";

inline static void rename_tmp_folder() {
    std::string userPath = fs_get_write_path("");
    std::string oldPath = userPath + "tmp";
    std::string newPath = userPath + TMP_DIRECTORY;
    if (fs::exists(oldPath) && !fs::exists(newPath)) {
#if defined(_WIN32) || defined(_WIN64)
        SetFileAttributesA(oldPath.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
        fs::rename(oldPath, newPath);
    }
}

static std::string rom_selection_path() {
    return fs_get_write_path(ROM_SELECTION_FILENAME);
}

static void save_selected_rom_path(const std::string &romPath) {
    std::ofstream out(rom_selection_path(), std::ios::trunc);
    if (!out.is_open()) {
        return;
    }
    out << romPath;
}

static void clear_selected_rom_path() {
    std::error_code ec;
    fs::remove(rom_selection_path(), ec);
}

static bool load_selected_rom_path(std::string &romPath) {
    std::ifstream in(rom_selection_path());
    if (!in.is_open()) {
        return false;
    }

    std::getline(in, romPath);
    return !romPath.empty();
}

static bool is_z64_rom_header(const std::vector<u8> &header) {
    return header.size() >= sizeof(ROM_MAGIC_Z64) &&
           memcmp(header.data(), ROM_MAGIC_Z64, sizeof(ROM_MAGIC_Z64)) == 0;
}

static std::string trim_string(const std::string &value) {
    size_t begin = 0;
    size_t end = value.size();

    while (begin < end && std::isspace((unsigned char)value[begin])) {
        begin++;
    }
    while (end > begin && std::isspace((unsigned char)value[end - 1])) {
        end--;
    }

    return value.substr(begin, end - begin);
}

static std::string sanitize_filename(const std::string &value) {
    std::string output;
    output.reserve(value.size());

    for (char ch : value) {
        if (std::isalnum((unsigned char)ch) || ch == ' ' || ch == '-' || ch == '_') {
            output.push_back(ch);
        } else {
            output.push_back('_');
        }
    }

    output = trim_string(output);
    if (output.empty()) {
        output = "romhack";
    }
    return output;
}

static bool read_rom_title(const std::string &romPath, std::string &titleOut) {
    std::ifstream file(romPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::vector<u8> header(0x40, 0);
    file.read((char *) header.data(), (std::streamsize) header.size());
    if (!file || !is_z64_rom_header(header)) {
        return false;
    }

    std::string title;
    for (size_t i = 0x20; i < 0x34 && i < header.size(); i++) {
        char ch = (char) header[i];
        if (ch == '\0') {
            break;
        }
        if (std::isprint((unsigned char) ch)) {
            title.push_back(ch);
        }
    }

    titleOut = trim_string(title);
    if (titleOut.empty()) {
        titleOut = trim_string(fs::path(romPath).stem().string());
    }
    return true;
}

static bool activate_rom_path(const std::string &romPath, const std::string &displayName, bool isCustomHack) {
    if (!fs::exists(romPath)) {
        return false;
    }

    snprintf(gRomFilename, SYS_MAX_PATH, "%s", romPath.c_str());
    gRomIsValid = true;
    sRomIsCustomHack = isCustomHack;
    sActiveRomDisplayName = displayName.empty() ? (isCustomHack ? "Custom ROM" : "Super Mario 64") : displayName;
    sActiveRomSelectionPath = romPath;
    return true;
}

static bool copy_rom_if_needed(const std::string &srcPath, const std::string &dstPath) {
    if (srcPath == dstPath) {
        return true;
    }

    std::error_code ec;
    fs::create_directories(fs::path(dstPath).parent_path(), ec);
    ec.clear();
    fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

static bool is_vanilla_rom_valid(const std::string &romPath) {
    u8 dataHash[16] = { 0 };
    mod_cache_md5(romPath.c_str(), dataHash);

    std::stringstream ss;
    for (int i = 0; i < 16; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dataHash[i]);
    }

    for (VanillaMD5 *md5 = sVanillaMD5; md5->localizationName != NULL; md5++) {
        if (md5->md5 == ss.str()) {
            std::string destPath = fs_get_write_path("") + std::string("baserom.") + md5->localizationName + ".z64";

            if (!copy_rom_if_needed(romPath, destPath)) {
                return false;
            }

            clear_selected_rom_path();
            return activate_rom_path(destPath, "Super Mario 64", false);
        }
    }

    return false;
}

static bool is_custom_rom_valid(const std::string &romPath) {
    std::error_code ec;
    if (!fs::exists(romPath, ec) || fs::is_directory(romPath, ec)) {
        return false;
    }

    std::string title;
    if (!read_rom_title(romPath, title)) {
        return false;
    }

    std::string sanitizedTitle = sanitize_filename(title);
    std::string destPath = fs_get_write_path(ROM_HACK_DIRECTORY);
    destPath += "/" + sanitizedTitle + ".z64";

    if (!copy_rom_if_needed(romPath, destPath)) {
        return false;
    }

    save_selected_rom_path(destPath);
    return activate_rom_path(destPath, title, true);
}

static bool try_accept_rom(const std::string &romPath) {
    if (is_vanilla_rom_valid(romPath)) {
        return true;
    }
    return is_custom_rom_valid(romPath);
}

static bool try_load_saved_rom(void) {
    std::string selectedPath;

    if (!load_selected_rom_path(selectedPath)) {
        return false;
    }

    if (!fs::exists(selectedPath)) {
        clear_selected_rom_path();
        return false;
    }

    if (try_accept_rom(selectedPath)) {
        return true;
    }

    clear_selected_rom_path();
    return false;
}

inline static bool scan_path_for_rom(const char *dir) {
    std::error_code ec;
    if (dir == nullptr || dir[0] == '\0' || !fs::exists(dir, ec)) {
        return false;
    }

    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
        std::string path = entry.path().generic_string();
        if (path_ends_with(path.c_str(), ".z64")) {
            if (try_accept_rom(path)) { return true; }
        }
    }
    return false;
}

extern "C" {
void legacy_folder_handler(void) {
    rename_tmp_folder();
}

bool main_rom_handler(void) {
    if (try_load_saved_rom()) { return true; }
    if (scan_path_for_rom(fs_get_write_path(""))) { return true; }
    scan_path_for_rom(sys_exe_path_dir());
    return gRomIsValid;
}

#ifdef LOADING_SCREEN_SUPPORTED
void rom_on_drop_file(const char *path) {
    static bool hasDroppedInvalidFile = false;
    if (strlen(path) > 0 && !try_accept_rom(path) && !hasDroppedInvalidFile) {
        hasDroppedInvalidFile = true;
        strcat(gCurrLoadingSegment.str, "\n\\#ffc000\\The file you last dropped was not a compatible .z64 SM64 rom or ROM hack.");
    }
}
#endif

bool rom_is_using_custom_hack(void) {
    return sRomIsCustomHack;
}

const char *rom_get_active_display_name(void) {
    return sActiveRomDisplayName.c_str();
}
}
