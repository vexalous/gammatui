#define _POSIX_C_SOURCE 200809L
#include "gammatui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "wlr-gamma-control-unstable-v1-client-protocol.h"
#include <wayland-client.h>

char display_output[128] = {0};
static RRCrtc crtc_id = 0;

static pthread_mutex_t debounce_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec last_apply = {0,0};
static const long DEBOUNCE_MS = 80;

static struct {
    struct wl_display *display;
    struct wl_registry *registry;
    struct zwlr_gamma_control_manager_v1 *manager;
    struct wl_output *output;
    struct zwlr_gamma_control_v1 *control;
    uint32_t gamma_size;
    int initialized;
    int failed;
} wl_state = {0};

static void fill_gamma_table(uint16_t *table, uint32_t size, float gamma, float bright);

static void handle_gamma_size(void *data, struct zwlr_gamma_control_v1 *control, uint32_t size) {
    (void)data; (void)control;
    wl_state.gamma_size = size;
}

static void handle_failed(void *data, struct zwlr_gamma_control_v1 *control) {
    (void)data; (void)control;
    wl_state.failed = 1;
}

static const struct zwlr_gamma_control_v1_listener gamma_listener = {
    handle_gamma_size,
    handle_failed
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    (void)data; (void)version;
    if (strcmp(interface, "zwlr_gamma_control_manager_v1") == 0) {
        wl_state.manager = wl_registry_bind(registry, name, &zwlr_gamma_control_manager_v1_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        if (!wl_state.output) {
            wl_state.output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        }
    }
}

static void registry_handle_remove(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_remove
};

static int is_wayland_session(void) {
    const char *w = getenv("WAYLAND_DISPLAY");
    return w && *w;
}

static void lazy_init_wayland(void) {
    if (wl_state.initialized) return;
    
    wl_state.display = wl_display_connect(NULL);
    if (!wl_state.display) {
        wl_state.failed = 1;
        wl_state.initialized = 1;
        return;
    }

    wl_state.registry = wl_display_get_registry(wl_state.display);
    wl_registry_add_listener(wl_state.registry, &registry_listener, NULL);
    wl_display_roundtrip(wl_state.display);

    if (wl_state.manager && wl_state.output) {
        wl_state.control = zwlr_gamma_control_manager_v1_get_gamma_control(wl_state.manager, wl_state.output);
        zwlr_gamma_control_v1_add_listener(wl_state.control, &gamma_listener, NULL);
        wl_display_roundtrip(wl_state.display);
    } else {
        wl_state.failed = 1;
    }

    if (wl_state.gamma_size == 0) wl_state.failed = 1;
    
    wl_state.initialized = 1;
}

static void wayland_apply(float g, float b) {
    lazy_init_wayland();
    if (wl_state.failed || !wl_state.control || wl_state.gamma_size == 0) return;

    char tmpname[] = "/tmp/gammatui-XXXXXX";
    int fd = mkstemp(tmpname);
    if (fd < 0) return;
    
    unlink(tmpname);
    
    size_t file_size = wl_state.gamma_size * 3 * sizeof(uint16_t);
    
    if (ftruncate(fd, (off_t)file_size) < 0) {
        close(fd);
        return;
    }

    uint16_t *data = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return;
    }

    fill_gamma_table(data, wl_state.gamma_size, g, b);
    munmap(data, file_size);

    zwlr_gamma_control_v1_set_gamma(wl_state.control, fd);
    close(fd);

    wl_display_roundtrip(wl_state.display);
}

int detect_output(char *outbuf, size_t outlen) {
    if (is_wayland_session()) {
        lazy_init_wayland();
        if (!wl_state.failed) {
            if (outbuf && outlen > 0 && !outbuf[0]) {
                strncpy(outbuf, "Wayland-Output", outlen - 1);
            }
            return 1;
        }
        return 0;
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 0;

    Window root = DefaultRootWindow(dpy);
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res) {
        XCloseDisplay(dpy);
        return 0;
    }

    int found = 0;
    char candidate[128] = {0};

    for (int i = 0; i < res->noutput; i++) {
        XRROutputInfo *info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
        if (info && info->connection == RR_Connected) {
            if (XRRGetOutputPrimary(dpy, root) == res->outputs[i]) {
                strncpy(outbuf, info->name, outlen - 1);
                crtc_id = info->crtc;
                found = 1;
                XRRFreeOutputInfo(info);
                break;
            }
            if (!candidate[0]) {
                strncpy(candidate, info->name, sizeof(candidate) - 1);
                crtc_id = info->crtc;
            }
        }
        if (info) XRRFreeOutputInfo(info);
    }
    
    if (!found && candidate[0]) {
        strncpy(outbuf, candidate, outlen - 1);
        found = 1;
    }

    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);
    return found;
}

static void fill_gamma_table(uint16_t *table, uint32_t size, float gamma, float bright) {
    for (uint32_t i = 0; i < size; i++) {
        double v = (double)i / (double)(size - 1);
        v = pow(v, 1.0 / (double)gamma) * (double)bright;
        if (v > 1.0) v = 1.0;
        if (v < 0.0) v = 0.0;
        
        uint16_t val = (uint16_t)(v * 65535.0 + 0.5);
        
        table[i] = val;
        table[i + size] = val;
        table[i + 2 * size] = val;
    }
}

struct xr_args {
    char opt[64];
    char val[64];
    char output[128];
};

static void *xr_worker_x11(void *arg) {
    struct xr_args *a = arg;
    float g_val = 1.0f, b_val = 1.0f;
    sscanf(a->val, "%f:%f", &g_val, &b_val);

    Display *dpy = XOpenDisplay(NULL);
    if (dpy) {
        if (crtc_id == 0) detect_output(display_output, sizeof(display_output));
        if (crtc_id != 0) {
            int size = XRRGetCrtcGammaSize(dpy, crtc_id);
            if (size > 0) {
                XRRCrtcGamma *gamma = XRRAllocGamma(size);
                if (gamma) {
                    for(int i=0; i<size; i++) {
                        double ramp = (double)i / (double)(size - 1);
                        double v = pow(ramp, 1.0/(double)g_val) * (double)b_val;
                        if(v>1.0) v=1.0; 
                        if(v<0.0) v=0.0;
                        unsigned short s = (unsigned short)(v * 65535.0 + 0.5);
                        gamma->red[i] = gamma->green[i] = gamma->blue[i] = s;
                    }
                    XRRSetCrtcGamma(dpy, crtc_id, gamma);
                    XRRFreeGamma(gamma);
                }
            }
        }
        XCloseDisplay(dpy);
    }
    
    free(a);
    return NULL;
}

void xr_call_async(const char *output, const char *opt, const char *val) {
    if (is_wayland_session()) {
        float g_val = 1.0f, b_val = 1.0f;
        sscanf(val, "%f:%f", &g_val, &b_val);
        wayland_apply(g_val, b_val);
        return;
    }

    struct xr_args *a = malloc(sizeof(*a));
    if (!a) return;
    strncpy(a->opt, opt, sizeof(a->opt) - 1);
    a->opt[sizeof(a->opt) - 1] = '\0';
    strncpy(a->val, val, sizeof(a->val) - 1);
    a->val[sizeof(a->val) - 1] = '\0';
    strncpy(a->output, output, sizeof(a->output) - 1);
    a->output[sizeof(a->output) - 1] = '\0';

    pthread_t t;
    if (pthread_create(&t, NULL, xr_worker_x11, a) == 0) pthread_detach(t);
    else free(a);
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
