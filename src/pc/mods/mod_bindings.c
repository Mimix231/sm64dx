#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod_bindings.h"
#include "pc/debuglog.h"
#include "pc/fs/fs.h"

struct ModBindingEntry {
    char modPath[SYS_MAX_PATH];
    char bindId[64];
    unsigned int bindValue[MAX_BINDS];
    struct ModBindingEntry* next;
};

static struct ModBindingEntry* sModBindingEntries = NULL;
static bool sModBindingsLoaded = false;

static void mod_bindings_free_entries(void) {
    struct ModBindingEntry* entry = sModBindingEntries;
    while (entry != NULL) {
        struct ModBindingEntry* next = entry->next;
        free(entry);
        entry = next;
    }
    sModBindingEntries = NULL;
}

static bool mod_bindings_encode_hex(const char* source, char* destination, size_t destinationSize) {
    size_t sourceLength = strlen(source);
    if ((sourceLength * 2) + 1 > destinationSize) {
        return false;
    }

    for (size_t i = 0; i < sourceLength; i++) {
        snprintf(destination + (i * 2), destinationSize - (i * 2), "%02X", (unsigned char)source[i]);
    }
    destination[sourceLength * 2] = '\0';
    return true;
}

static int mod_bindings_hex_digit(char value) {
    if (value >= '0' && value <= '9') { return value - '0'; }
    value = (char)toupper((unsigned char)value);
    if (value >= 'A' && value <= 'F') { return 10 + (value - 'A'); }
    return -1;
}

static bool mod_bindings_decode_hex(const char* source, char* destination, size_t destinationSize) {
    size_t sourceLength = strlen(source);
    if ((sourceLength % 2) != 0) {
        return false;
    }

    size_t outputLength = sourceLength / 2;
    if (outputLength + 1 > destinationSize) {
        return false;
    }

    for (size_t i = 0; i < outputLength; i++) {
        int hi = mod_bindings_hex_digit(source[i * 2]);
        int lo = mod_bindings_hex_digit(source[(i * 2) + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        destination[i] = (char)((hi << 4) | lo);
    }
    destination[outputLength] = '\0';
    return true;
}

static struct ModBindingEntry* mod_bindings_find(const char* modPath, const char* bindId) {
    for (struct ModBindingEntry* entry = sModBindingEntries; entry != NULL; entry = entry->next) {
        if (strcmp(entry->modPath, modPath) == 0 && strcmp(entry->bindId, bindId) == 0) {
            return entry;
        }
    }
    return NULL;
}

static void mod_bindings_save_internal(void) {
    FILE* file = fopen(fs_get_write_path(MOD_BINDINGS_FILE), "w");
    if (file == NULL) {
        LOG_ERROR("Could not save mod bindings");
        return;
    }

    fprintf(file, "# SM64 DX mod control bindings\n");
    for (struct ModBindingEntry* entry = sModBindingEntries; entry != NULL; entry = entry->next) {
        char encodedModPath[(SYS_MAX_PATH * 2) + 1] = { 0 };
        char encodedBindId[(64 * 2) + 1] = { 0 };
        if (!mod_bindings_encode_hex(entry->modPath, encodedModPath, sizeof(encodedModPath))) {
            continue;
        }
        if (!mod_bindings_encode_hex(entry->bindId, encodedBindId, sizeof(encodedBindId))) {
            continue;
        }

        fprintf(
            file,
            "%s\t%s\t%04x\t%04x\t%04x\n",
            encodedModPath,
            encodedBindId,
            entry->bindValue[0],
            entry->bindValue[1],
            entry->bindValue[2]
        );
    }

    fclose(file);
}

static void mod_bindings_load(void) {
    if (sModBindingsLoaded) {
        return;
    }

    sModBindingsLoaded = true;

    FILE* file = fopen(fs_get_write_path(MOD_BINDINGS_FILE), "r");
    if (file == NULL) {
        return;
    }

    char line[2048] = { 0 };
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t length = strcspn(line, "\r\n");
        line[length] = '\0';
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        char* encodedModPath = strtok(line, "\t");
        char* encodedBindId = strtok(NULL, "\t");
        char* bind0 = strtok(NULL, "\t");
        char* bind1 = strtok(NULL, "\t");
        char* bind2 = strtok(NULL, "\t");
        if (encodedModPath == NULL || encodedBindId == NULL || bind0 == NULL || bind1 == NULL || bind2 == NULL) {
            continue;
        }

        struct ModBindingEntry* entry = calloc(1, sizeof(struct ModBindingEntry));
        if (entry == NULL) {
            break;
        }

        if (!mod_bindings_decode_hex(encodedModPath, entry->modPath, sizeof(entry->modPath))
            || !mod_bindings_decode_hex(encodedBindId, entry->bindId, sizeof(entry->bindId))) {
            free(entry);
            continue;
        }

        sscanf(bind0, "%x", &entry->bindValue[0]);
        sscanf(bind1, "%x", &entry->bindValue[1]);
        sscanf(bind2, "%x", &entry->bindValue[2]);

        entry->next = sModBindingEntries;
        sModBindingEntries = entry;
    }

    fclose(file);
}

bool mod_bindings_get(const char* modPath, const char* bindId, unsigned int bindValue[MAX_BINDS]) {
    if (modPath == NULL || bindId == NULL || bindValue == NULL) {
        return false;
    }

    mod_bindings_load();

    struct ModBindingEntry* entry = mod_bindings_find(modPath, bindId);
    if (entry == NULL) {
        return false;
    }

    memcpy(bindValue, entry->bindValue, sizeof(entry->bindValue));
    return true;
}

void mod_bindings_set(const char* modPath, const char* bindId, const unsigned int bindValue[MAX_BINDS]) {
    if (modPath == NULL || bindId == NULL || bindValue == NULL) {
        return;
    }

    mod_bindings_load();

    struct ModBindingEntry* entry = mod_bindings_find(modPath, bindId);
    if (entry == NULL) {
        entry = calloc(1, sizeof(struct ModBindingEntry));
        if (entry == NULL) {
            return;
        }
        snprintf(entry->modPath, sizeof(entry->modPath), "%s", modPath);
        snprintf(entry->bindId, sizeof(entry->bindId), "%s", bindId);
        entry->next = sModBindingEntries;
        sModBindingEntries = entry;
    }

    memcpy(entry->bindValue, bindValue, sizeof(entry->bindValue));
    mod_bindings_save_internal();
}

void mod_bindings_shutdown(void) {
    mod_bindings_free_entries();
    sModBindingsLoaded = false;
}
