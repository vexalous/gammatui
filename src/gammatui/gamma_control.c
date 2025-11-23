#define _POSIX_C_SOURCE 200809L
#include "gamma_control.h"
#include <stdio.h>

static double cached_gamma = -1.0;
static double cached_bright = -1.0;

void apply_values(double gamma, double bright) {
    if (display_output[0] == '\0') {
        return;
    }

    if (gamma != cached_gamma || bright != cached_bright) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.4f:%.4f", gamma, bright);
        xr_call_async(display_output, "--set", buf);
        
        cached_gamma = gamma;
        cached_bright = bright;
    }
}

void revert_values(void) {
    apply_values(1.0, 1.0);
}
