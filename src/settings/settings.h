#ifndef S_H
#define S_H
#include <stddef.h>
#include <stdbool.h>

#define OUTPUT_LEN 128

struct cfg {
    double gamma_min, gamma_max, bright_min, bright_max;
    char output[OUTPUT_LEN];
    int sel_r, sel_g, sel_b, unsel_r, unsel_g, unsel_b;
    int key_up, key_down, key_select, key_quit;
};

bool config_path_for_exe(char *, size_t, const char *);
void config_set_defaults(struct cfg *);
bool load_config(struct cfg *, const char *);
bool save_config(const struct cfg *, const char *);

#endif
