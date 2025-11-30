#ifndef GAMMATUI_SETTINGS_H
#define GAMMATUI_SETTINGS_H

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <limits.h>
#include <stdbool.h>

#define OUTPUT_LEN 128

struct cfg {
    char output[OUTPUT_LEN];
    double gamma_min;
    double gamma_max;
    double bright_min;
    double bright_max;
    int sel_r, sel_g, sel_b;
    int unsel_r, unsel_g, unsel_b;
    int key_up;
    int key_down;
    int key_select;
    int key_quit; 
};

bool config_path_for_exe(char *out, size_t outlen, const char *argv0);
void config_set_defaults(struct cfg *c);
bool load_config(struct cfg *c, const char *path);
bool save_config(const struct cfg *c, const char *path);
