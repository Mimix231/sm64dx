#pragma once

#include <stdbool.h>

#include "pc/configfile.h"
#include "pc/platform.h"

#define MOD_BINDINGS_FILE "mod-bindings.txt"

bool mod_bindings_get(const char* modPath, const char* bindId, unsigned int bindValue[MAX_BINDS]);
void mod_bindings_set(const char* modPath, const char* bindId, const unsigned int bindValue[MAX_BINDS]);
void mod_bindings_shutdown(void);
