#pragma once

#include <stdbool.h>

#define MOD_PROFILE_LAST_SESSION "last-session"

bool mods_profile_exists(const char* profileId);
bool mods_profile_save(const char* profileId);
bool mods_profile_load(const char* profileId);
void mods_profile_save_last_session(void);
