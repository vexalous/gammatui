#define _XOPEN_SOURCE 700
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

void config_set_defaults(struct cfg *c) {
    if (!c) return;
    *c = (struct cfg){
        .gamma_min = 0.1, .gamma_max = 10.0,
        .bright_min = 0.1, .bright_max = 2.0,
        .sel_r = 255, .sel_g = 255, .sel_b = 255,
        .unsel_r = 148, .unsel_g = 148, .unsel_b = 148,
        .key_up = 'w', .key_down = 's', .key_select = 10, .key_quit = 'q'
    };
    snprintf(c->output, OUTPUT_LEN, "eDP-1");
}

bool config_path_for_exe(char *out, size_t outlen, const char *argv0) {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    
    if (n > 0) {
        buf[n] = '\0';
    } else if (!argv0 || !realpath(argv0, buf)) {
        return false;
    }

    char *slash = strrchr(buf, '/');
    if (slash) *slash = '\0';

    size_t len = strlen(buf);
    bool in_settings = (len >= 9 && strcmp(buf + len - 9, "/settings") == 0);

    return snprintf(out, outlen, "%s/%sconfig.json", buf, in_settings ? "" : "../settings/") < (int)outlen;
}

bool load_config(struct cfg *c, const char *path) {
    FILE *f;
    if (!c || !path || !(f = fopen(path, "r"))) return false;
    
    config_set_defaults(c);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *key = strchr(line, '"');
        if (!key) continue;
        char *key_end = strchr(++key, '"');
        if (!key_end) continue;
        *key_end = '\0';

        char *val = strchr(key_end + 1, ':');
        if (!val) continue;
        val++;

        if (strcmp(key, "output") == 0) {
            char *qs = strchr(val, '"');
            if (qs) {
                char *qe = strchr(qs + 1, '"');
                if (qe) {
                    *qe = '\0';
                    snprintf(c->output, OUTPUT_LEN, "%s", qs + 1);
                }
            }
            continue;
        }

        #define P_DBL(K, V) else if (strcmp(key, K) == 0) c->V = strtod(val, NULL)
        #define P_INT(K, V) else if (strcmp(key, K) == 0) c->V = (int)strtol(val, NULL, 10)

        if (0) {}
        P_DBL("gamma_min", gamma_min);
        P_DBL("gamma_max", gamma_max);
        else if (strcmp(key, "brightness_min") == 0 || strcmp(key, "bright_min") == 0) c->bright_min = strtod(val, NULL);
        else if (strcmp(key, "brightness_max") == 0 || strcmp(key, "bright_max") == 0) c->bright_max = strtod(val, NULL);
        P_INT("sel_r", sel_r); P_INT("sel_g", sel_g); P_INT("sel_b", sel_b);
        P_INT("unsel_r", unsel_r); P_INT("unsel_g", unsel_g); P_INT("unsel_b", unsel_b);
        P_INT("key_up", key_up); P_INT("key_down", key_down);
        P_INT("key_select", key_select); P_INT("key_quit", key_quit);
    }
    fclose(f);
    return true;
}

bool save_config(const struct cfg *c, const char *path) {
    if (!c || !path) return false;

    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        struct stat st = {0};
        if (stat(dir, &st) == -1) mkdir(dir, 0755);
    }

    char tmp[PATH_MAX];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) return false;

    FILE *f = fopen(tmp, "w");
    if (!f) return false;

    fprintf(f, "{\n"
               "    \"output\": \"%s\",\n"
               "    \"gamma_min\": %.3f,\n"
               "    \"gamma_max\": %.3f,\n"
               "    \"brightness_min\": %.3f,\n"
               "    \"brightness_max\": %.3f,\n"
               "    \"sel_r\": %d,\n"
               "    \"sel_g\": %d,\n"
               "    \"sel_b\": %d,\n"
               "    \"unsel_r\": %d,\n"
               "    \"unsel_g\": %d,\n"
               "    \"unsel_b\": %d,\n"
               "    \"key_up\": %d,\n"
               "    \"key_down\": %d,\n"
               "    \"key_select\": %d,\n"
               "    \"key_quit\": %d\n"
               "}\n",
            c->output, c->gamma_min, c->gamma_max, c->bright_min, c->bright_max,
            c->sel_r, c->sel_g, c->sel_b, c->unsel_r, c->unsel_g, c->unsel_b,
            c->key_up, c->key_down, c->key_select, c->key_quit);

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    if (rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }
    return true;
}
