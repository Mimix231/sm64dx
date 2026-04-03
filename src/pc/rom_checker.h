#include "types.h"

extern bool gRomIsValid;
extern char gRomFilename[];

void legacy_folder_handler(void);

bool main_rom_handler(void);
void rom_on_drop_file(const char *path);
bool rom_is_using_custom_hack(void);
const char *rom_get_active_display_name(void);
