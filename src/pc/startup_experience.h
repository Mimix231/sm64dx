#pragma once

#include <stdbool.h>

#define ACCESSIBILITY_PRESET_CLASSIC 0
#define ACCESSIBILITY_PRESET_COMFORT 1
#define ACCESSIBILITY_PRESET_FOCUSED 2
#define ACCESSIBILITY_PRESET_CUSTOM 3

void startup_experience_bootstrap(void);
void startup_experience_begin_session(void);
void startup_experience_end_session(void);
void startup_experience_apply_accessibility_preset(unsigned int preset);

bool startup_experience_is_safe_mode(void);
bool startup_experience_should_show_first_run(void);
bool startup_experience_has_recovery_profile(void);
const char* startup_experience_get_status_text(void);
