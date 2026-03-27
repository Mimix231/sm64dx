#include <stdio.h>
#include <string.h>

#include "pc/ini.h"
#include "pc/platform.h"

#include "mxui_language.h"
#include "mxui_unicode.h"

static ini_t* sMxuiLanguage = NULL;

void mxui_language_shutdown(void) {
    if (sMxuiLanguage != NULL) {
        ini_free(sMxuiLanguage);
        sMxuiLanguage = NULL;
    }
}

bool mxui_language_init(char* lang) {
    char path[SYS_MAX_PATH] = { 0 };

    mxui_language_shutdown();

    if (lang == NULL || lang[0] == '\0') {
        lang = "English";
    }

    snprintf(path, sizeof(path), "%s/lang/%s.ini", sys_resource_path(), lang);
    sMxuiLanguage = ini_load(path);
    return sMxuiLanguage != NULL;
}

bool mxui_language_validate(const char* lang) {
    char copied[SYS_MAX_PATH] = { 0 };
    if (lang != NULL) {
        snprintf(copied, sizeof(copied), "%s", lang);
    }
    return mxui_language_init(copied);
}

char* mxui_language_get(const char* section, const char* key) {
    if (key == NULL) {
        return "";
    }
    if (sMxuiLanguage == NULL) {
        return (char*)key;
    }

    char* value = (char*)ini_get(sMxuiLanguage, section, key);
    return (value != NULL) ? value : (char*)key;
}

char* mxui_language_find_key(const char* section, const char* value) {
    if (sMxuiLanguage == NULL) {
        return NULL;
    }
    return (char*)ini_find_key(sMxuiLanguage, section, value);
}

void mxui_language_replace(char* src, char* dst, int size, char key, char* value) {
    char tmpChar[10] = { 0 };
    char* c = src;
    char* o = dst;
    while (*c != '\0') {
        if (*c == key) {
            snprintf(o, size - (o - dst), "%s", value);
        } else {
            mxui_unicode_get_char(c, tmpChar);
            snprintf(o, size - (o - dst), "%s", tmpChar);
        }
        o = &dst[strlen(dst)];
        c = mxui_unicode_next_char(c);
    }
}

void mxui_language_replace2(char* src, char* dst, int size, char key1, char* value1, char key2, char* value2) {
    char tmpChar[10] = { 0 };
    char* c = src;
    char* o = dst;
    while (*c != '\0') {
        if (*c == key1) {
            snprintf(o, size - (o - dst), "%s", value1);
        } else if (*c == key2) {
            snprintf(o, size - (o - dst), "%s", value2);
        } else {
            mxui_unicode_get_char(c, tmpChar);
            snprintf(o, size - (o - dst), "%s", tmpChar);
        }
        o = &dst[strlen(dst)];
        c = mxui_unicode_next_char(c);
    }
}
