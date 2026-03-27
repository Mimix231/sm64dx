#pragma once

#ifdef DLANG
#undef DLANG
#endif
#define DLANG(_SECTION, _KEY) mxui_language_get(#_SECTION, #_KEY)

bool mxui_language_init(char* lang);
char* mxui_language_get(const char* section, const char* key);
char* mxui_language_find_key(const char* section, const char* value);
void mxui_language_replace(char* src, char* dst, int size, char key, char* value);
void mxui_language_replace2(char* src, char* dst, int size, char key1, char* value1, char key2, char* value2);
bool mxui_language_validate(const char* lang);
void mxui_language_shutdown(void);
