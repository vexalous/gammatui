#define _POSIX_C_SOURCE 200809L
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>

static bool exe_dir(char *out, size_t outlen, const char *argv0) {
#if defined(linux)
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf));
    if (n > 0) {
        buf[n] = '\0';
        char *last_slash = strrchr(buf, '/');
        if (last_slash) {
            *last_slash = '\0';
            strncpy(out, buf, outlen - 1);
            out[outlen - 1] = '\0';
            return true;
        }
    }
#endif
    if (!argv0 || !strchr(argv0, '/')) {
        return false;
    }
    char tmp[PATH_MAX];
    strncpy(tmp, argv0, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char *last_slash = strrchr(tmp, '/');
    if (last_slash) {
        *last_slash = '\0';
        strncpy(out, tmp, outlen - 1);
        out[outlen - 1] = '\0';
        return true;
    }
    return false;
}

bool config_path_for_exe(char *out, size_t outlen, const char *argv0) {
    char d[PATH_MAX];
    if (!exe_dir(d, sizeof(d), argv0)) return false;
    if (snprintf(out, outlen, "%s/config.json", d) >= (int)outlen) return false;
    return true;
}

bool load_config(struct cfg *c, const char *path) {
    if (!c || !path) return false;
    c->output[0] = '\0';
    c->gamma_min = 0.1;
    c->gamma_max = 10.0;
    c->bright_min = 0.1;
    c->bright_max = 2.0;
    c->selected_color = 7;
    c->unselected_color = 5;

    FILE *f = fopen(path, "r");
    if (!f) return false;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *key = line;
        while (isspace((unsigned char)*key)) key++;

        if (strncmp(key, "\"output\"", 8) == 0) {
            char *p = strchr(key, ':');
            if (!p) continue;
            p++;
            while (isspace((unsigned char)*p)) p++;
            if (*p == '"') {
                p++;
                char *end = strchr(p, '"');
                if (end) {
                    size_t len = (size_t)(end - p);
                    if (len >= OUTPUT_LEN) len = OUTPUT_LEN - 1;
                    memcpy(c->output, p, len);
                    c->output[len] = '\0';
                }
            }
        }
        else if (strncmp(key, "\"gamma_min\"", 11) == 0) {
            char *p = strchr(key, ':');
            if (p) c->gamma_min = strtod(p + 1, NULL);
        }
        else if (strncmp(key, "\"gamma_max\"", 11) == 0) {
            char *p = strchr(key, ':');
            if (p) c->gamma_max = strtod(p + 1, NULL);
        }
        else if (strncmp(key, "\"brightness_min\"", 16) == 0 || strncmp(key, "\"bright_min\"", 12) == 0) {
            char *p = strchr(key, ':');
            if (p) c->bright_min = strtod(p + 1, NULL);
        }
        else if (strncmp(key, "\"brightness_max\"", 16) == 0 || strncmp(key, "\"bright_max\"", 12) == 0) {
            char *p = strchr(key, ':');
            if (p) c->bright_max = strtod(p + 1, NULL);
        }
        else if (strncmp(key, "\"selected_color\"", 16) == 0) {
            char *p = strchr(key, ':');
            if (p) c->selected_color = (int)strtol(p + 1, NULL, 10);
        }
        else if (strncmp(key, "\"unselected_color\"", 18) == 0) {
            char *p = strchr(key, ':');
            if (p) c->unselected_color = (int)strtol(p + 1, NULL, 10);
        }
    }
    fclose(f);
    return true;
}

bool save_config(const struct cfg *c, const char *path) {
    if (!c || !path) return false;
    char tmp[PATH_MAX];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) return false;

    FILE *f = fopen(tmp, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "    \"output\": \"%s\",\n", c->output[0] ? c->output : "");
    fprintf(f, "    \"gamma_min\": %.3f,\n", c->gamma_min);
    fprintf(f, "    \"gamma_max\": %.3f,\n", c->gamma_max);
    fprintf(f, "    \"brightness_min\": %.3f,\n", c->bright_min);
    fprintf(f, "    \"brightness_max\": %.3f,\n", c->bright_max);
    fprintf(f, "    \"selected_color\": %d,\n", c->selected_color);
    fprintf(f, "    \"unselected_color\": %d\n", c->unselected_color);
    fprintf(f, "}\n");

    if (fflush(f) != 0) {
        fclose(f);
        remove(tmp);
        return false;
    }
    fsync(fileno(f));
    fclose(f);

    if (rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }
    return true;
}
