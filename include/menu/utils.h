#ifndef MENU_UTILS_H
#define MENU_UTILS_H
#include <stdbool.h>
#include <stddef.h>

bool is_executable_file(const char *path);
bool resolve_exe_dir(char *out, size_t outlen, const char *argv0);
bool build_gammagui_path(char *out, size_t outlen, const char *argv0);
bool build_settings_path(char *out, size_t outlen, const char *argv0);

#endif
