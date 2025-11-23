#define _POSIX_C_SOURCE 200809L
#include "gamma_control.h"
#include <stdio.h>

static double cached_gamma = -1.0;
static double cached_bright = -1.0;

void apply_values(double gamma, double bright) {
    if (display_output[0] == '\0') {
        return;
    }

    char buf[64];

    if (gamma != cached_gamma) {
        snprintf(buf, sizeof(buf), "%.3f:%.3f:%.3f", gamma, gamma, gamma);
        xr_call_async(display_output, "--gamma", buf);
        cached_gamma = gamma;
    }

    if (bright != cached_bright) {
        snprintf(buf, sizeof(buf), "%.3f", bright);
        xr_call_async(display_output, "--brightness", buf);
        cached_bright = bright;
    }
}

void revert_values(void) {
    apply_values(1.0, 1.0);
}
