#define _XOPEN_SOURCE 700
#include "utils.h"
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool is_executable_file(const char *p) {
    struct stat s;
    return p && stat(p, &s) == 0 && S_ISREG(s.st_mode) && access(p, X_OK) == 0;
}

bool resolve_exe_dir(char *out, size_t outlen, const char *argv0) {
    char buf[PATH_MAX];
    char *path = NULL;

#if defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        path = buf;
    }
#endif

    if (!path && argv0 && *argv0) {
        if (realpath(argv0, buf)) {
            path = buf;
        }
    }

    if (!path) return false;

    char *d = dirname(path);
    if (!d) return false;

    return snprintf(out, outlen, "%s", d) < (int)outlen;
}

static bool build_sibling_path(char *out, size_t outlen, const char *argv0, const char *rel_path) {
    char dir[PATH_MAX];
    if (!resolve_exe_dir(dir, sizeof(dir), argv0)) return false;

    char tmp[PATH_MAX];
    if (snprintf(tmp, sizeof(tmp), "%s/%s", dir, rel_path) >= (int)sizeof(tmp)) {
        return false;
    }

    return snprintf(out, outlen, "%s", tmp) < (int)outlen;
}

bool build_gammatui_path(char *out, size_t outlen, const char *argv0) {
    return build_sibling_path(out, outlen, argv0, "../gammatui/gammatui.elf");
}

bool build_settings_path(char *out, size_t outlen, const char *argv0) {
    return build_sibling_path(out, outlen, argv0, "../settings/brightnesstui.elf");
}
