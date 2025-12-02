#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface zwlr_gamma_control_v1_interface;

static const struct wl_interface *wlr_gamma_control_unstable_v1_types[] = {
	&zwlr_gamma_control_v1_interface,
	&wl_output_interface,
};

static const struct wl_message zwlr_gamma_control_manager_v1_requests[] = {
	{ "get_gamma_control", "no", wlr_gamma_control_unstable_v1_types + 0 },
	{ "destroy", "", wlr_gamma_control_unstable_v1_types + 0 },
};

const struct wl_interface zwlr_gamma_control_manager_v1_interface = {
	"zwlr_gamma_control_manager_v1", 1,
	2, zwlr_gamma_control_manager_v1_requests,
	0, NULL,
};

static const struct wl_message zwlr_gamma_control_v1_requests[] = {
	{ "set_gamma", "h", wlr_gamma_control_unstable_v1_types + 0 },
	{ "destroy", "", wlr_gamma_control_unstable_v1_types + 0 },
};

static const struct wl_message zwlr_gamma_control_v1_events[] = {
	{ "gamma_size", "u", wlr_gamma_control_unstable_v1_types + 0 },
	{ "failed", "", wlr_gamma_control_unstable_v1_types + 0 },
};

const struct wl_interface zwlr_gamma_control_v1_interface = {
	"zwlr_gamma_control_v1", 1,
	2, zwlr_gamma_control_v1_requests,
	2, zwlr_gamma_control_v1_events,
};
