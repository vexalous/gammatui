#define _POSIX_C_SOURCE 200809L
#include "gammatui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <math.h>

char display_output[128] = {0};
static RRCrtc crtc_id = 0;

static pthread_mutex_t debounce_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec last_apply = {0,0};
static const long DEBOUNCE_MS = 80;

int detect_output(char *outbuf, size_t outlen) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 0;

    Window root = DefaultRootWindow(dpy);
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res) {
        XCloseDisplay(dpy);
        return 0;
    }

    int found_primary = 0;
    char candidate[128] = {0};
    RRCrtc candidate_crtc = 0;

    for (int i = 0; i < res->noutput; i++) {
        XRROutputInfo *output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
        if (!output_info || output_info->connection != RR_Connected) {
            if (output_info) XRRFreeOutputInfo(output_info);
            continue;
        }

        if (XRRGetOutputPrimary(dpy, root) == res->outputs[i]) {
            strncpy(outbuf, output_info->name, outlen - 1);
            outbuf[outlen - 1] = '\0';
            crtc_id = output_info->crtc;
            found_primary = 1;
            XRRFreeOutputInfo(output_info);
            break;
        }

        if (candidate[0] == '\0') {
            strncpy(candidate, output_info->name, sizeof(candidate) - 1);
            candidate[sizeof(candidate) - 1] = '\0';
            candidate_crtc = output_info->crtc;
        }
        XRRFreeOutputInfo(output_info);
    }

    if (!found_primary && candidate[0]) {
        strncpy(outbuf, candidate, outlen - 1);
        outbuf[outlen-1] = '\0';
        crtc_id = candidate_crtc;
    }

    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);
    return found_primary || (candidate[0] != '\0');
}

struct xr_args {
    char opt[64];
    char val[64];
};

static void *xr_worker(void *arg) {
    struct xr_args *a = arg;
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        free(a);
        return NULL;
    }

    if (crtc_id == 0) {
        detect_output(display_output, sizeof(display_output));
    }
    
    if (crtc_id != 0) {
        int size = XRRGetCrtcGammaSize(dpy, crtc_id);
        if (size > 0) {
            XRRCrtcGamma *gamma = XRRAllocGamma(size);
            if (gamma) {
                if (strcmp(a->opt, "--set") == 0) {
                    float g_val, b_val;
                    if (sscanf(a->val, "%f:%f", &g_val, &b_val) == 2) {
                        for (int i = 0; i < size; i++) {
                            double ramp = (double)i / (double)(size - 1);
                            double v = pow(ramp, 1.0 / g_val) * b_val;
                            
                            if (v > 1.0) v = 1.0;
                            if (v < 0.0) v = 0.0;
                            
                            unsigned short s = (unsigned short)(v * 65535.0 + 0.5);
                            gamma->red[i] = s;
                            gamma->green[i] = s;
                            gamma->blue[i] = s;
                        }
                        XRRSetCrtcGamma(dpy, crtc_id, gamma);
                    }
                }
                XRRFreeGamma(gamma);
            }
        }
    }

    XCloseDisplay(dpy);
    free(a);
    return NULL;
}

void xr_call_async(const char *output, const char *opt, const char *val) {
    (void)output;
    struct xr_args *a = malloc(sizeof(*a));
    if (!a) return;
    strncpy(a->opt, opt, sizeof(a->opt) - 1);
    a->opt[sizeof(a->opt) - 1] = '\0';
    strncpy(a->val, val, sizeof(a->val) - 1);
    a->val[sizeof(a->val) - 1] = '\0';

    pthread_t t;
    if (pthread_create(&t, NULL, xr_worker, a) == 0) {
        pthread_detach(t);
    } else {
        free(a);
    }
}

int debounce_allow(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pthread_mutex_lock(&debounce_mutex);
    long diff_ms = (now.tv_sec - last_apply.tv_sec) * 1000 + (now.tv_nsec - last_apply.tv_nsec) / 1000000;
    if (diff_ms >= DEBOUNCE_MS) {
        last_apply = now;
        pthread_mutex_unlock(&debounce_mutex);
        return 1;
    }
    pthread_mutex_unlock(&debounce_mutex);
    return 0;
}

double clamp_double(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
