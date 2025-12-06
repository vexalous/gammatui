#include "gamma_control.h"
#include <stdio.h>
#include <math.h>

static double cached_gamma = -1.0;
static double cached_bright = -1.0;

void apply_values(double gamma, double bright) {
    if (display_output[0] == '\0') {
        return;
    }

    if (fabs(gamma - cached_gamma) > 0.000001 || fabs(bright - cached_bright) > 0.000001) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%.4f:%.4f", gamma, bright);
        xr_call_async(display_output, "--set", buf);

        cached_gamma = gamma;
        cached_bright = bright;
    }
}

void revert_values(void) {
    apply_values(1.0, 1.0);
}
