#include <stdio.h>
#include <string.h>
#include <time.h>

#include "startup_experience.h"
#include "cliopts.h"
#include "configfile.h"
#include "debuglog.h"
#include "fs/fs.h"
#include "mods/mod_profiles.h"

#define SESSION_MARKER_FILE "sm64dx-session.lock"
#define SAVE_BACKUP_FILE "sm64_save_file.backup.bin"

static bool sSafeMode = false;
static char sStatusText[256] = { 0 };

static bool startup_experience_path(char* destination, size_t destinationSize, const char* relativePath) {
    return snprintf(destination, destinationSize, "%s/%s", fs_writepath, relativePath) >= 0;
}

static void startup_experience_copy_file(const char* sourceRelativePath, const char* destinationRelativePath) {
    char sourcePath[SYS_MAX_PATH] = { 0 };
    char destinationPath[SYS_MAX_PATH] = { 0 };
    if (!startup_experience_path(sourcePath, sizeof(sourcePath), sourceRelativePath)) { return; }
    if (!startup_experience_path(destinationPath, sizeof(destinationPath), destinationRelativePath)) { return; }
    if (!fs_sys_file_exists(sourcePath)) { return; }

    FILE* source = fopen(sourcePath, "rb");
    if (source == NULL) { return; }

    FILE* destination = fopen(destinationPath, "wb");
    if (destination == NULL) {
        fclose(source);
        return;
    }

    char buffer[4096];
    size_t readCount = 0;
    while ((readCount = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, readCount, destination);
    }

    fclose(destination);
    fclose(source);
}

void startup_experience_bootstrap(void) {
    char markerPath[SYS_MAX_PATH] = { 0 };
    if (!startup_experience_path(markerPath, sizeof(markerPath), SESSION_MARKER_FILE)) {
        return;
    }

    sSafeMode = fs_sys_file_exists(markerPath);
    if (sSafeMode) {
        gCLIOpts.disableMods = true;
        snprintf(
            sStatusText,
            sizeof(sStatusText),
            "Safe mode is active. Mods were not auto-loaded because the last session did not close cleanly."
        );
    } else {
        sStatusText[0] = '\0';
    }
}

void startup_experience_begin_session(void) {
    startup_experience_copy_file(SAVE_FILENAME, SAVE_BACKUP_FILE);

    char markerPath[SYS_MAX_PATH] = { 0 };
    if (!startup_experience_path(markerPath, sizeof(markerPath), SESSION_MARKER_FILE)) {
        return;
    }

    FILE* marker = fopen(markerPath, "w");
    if (marker == NULL) {
        LOG_ERROR("Could not create session marker");
        return;
    }

    fprintf(marker, "started=%lld\n", (long long)time(NULL));
    fclose(marker);
}

void startup_experience_end_session(void) {
    startup_experience_copy_file(SAVE_FILENAME, SAVE_BACKUP_FILE);

    char markerPath[SYS_MAX_PATH] = { 0 };
    if (!startup_experience_path(markerPath, sizeof(markerPath), SESSION_MARKER_FILE)) {
        return;
    }

    remove(markerPath);
}

void startup_experience_apply_accessibility_preset(unsigned int preset) {
    switch (preset) {
        case ACCESSIBILITY_PRESET_CLASSIC:
            configAccessibilityPreset = ACCESSIBILITY_PRESET_CLASSIC;
            configDjuiScale = 0;
            configReduceCameraShake = false;
            configReduceHudFlash = false;
            configStickDeadzone = 16;
            configRumbleStrength = 50;
            configDisablePopups = false;
            configSkipIntro = false;
            configPauseAnywhere = false;
            break;
        case ACCESSIBILITY_PRESET_COMFORT:
            configAccessibilityPreset = ACCESSIBILITY_PRESET_COMFORT;
            configDjuiScale = 3;
            configReduceCameraShake = true;
            configReduceHudFlash = true;
            configStickDeadzone = 20;
            configRumbleStrength = 25;
            configDisablePopups = false;
            configSkipIntro = true;
            configPauseAnywhere = true;
            break;
        case ACCESSIBILITY_PRESET_FOCUSED:
            configAccessibilityPreset = ACCESSIBILITY_PRESET_FOCUSED;
            configDjuiScale = 4;
            configReduceCameraShake = true;
            configReduceHudFlash = true;
            configStickDeadzone = 24;
            configRumbleStrength = 0;
            configDisablePopups = true;
            configSkipIntro = true;
            configPauseAnywhere = true;
            break;
        default:
            configAccessibilityPreset = ACCESSIBILITY_PRESET_CUSTOM;
            break;
    }
}

bool startup_experience_is_safe_mode(void) {
    return sSafeMode;
}

bool startup_experience_should_show_first_run(void) {
    return !configFirstBootCompleted;
}

bool startup_experience_has_recovery_profile(void) {
    return mods_profile_exists(MOD_PROFILE_LAST_SESSION);
}

const char* startup_experience_get_status_text(void) {
    return sStatusText;
}
