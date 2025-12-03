#define _XOPEN_SOURCE 700
#include "utils.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char c[PATH_MAX];

static char *g(const char *a) {
    if (*c) return c;
    ssize_t n = readlink("/proc/self/exe", c, sizeof(c) - 1);
    if (n > 0) c[n] = 0;
    else if (!a || !realpath(a, c)) return 0;
    
    char *s = strrchr(c, '/');
    if (s) *s = 0; else { c[0] = '.'; c[1] = 0; }
    return c;
}

bool is_executable_file(const char *p) {
    struct stat s;
    return p && !access(p, X_OK) && !stat(p, &s) && S_ISREG(s.st_mode);
}

static bool b(char *o, size_t l, const char *a, const char *s) {
    char *d = g(a);
    return d && snprintf(o, l, s ? "%s/%s" : "%s", d, s) < (int)l;
}

bool resolve_exe_dir(char *o, size_t l, const char *a) {
    return b(o, l, a, 0);
}

bool build_gammatui_path(char *o, size_t l, const char *a) {
    return b(o, l, a, "../gammatui/gammatui.elf");
}

bool build_settings_path(char *o, size_t l, const char *a) {
    return b(o, l, a, "../settings/brightnesstui.elf");
}
