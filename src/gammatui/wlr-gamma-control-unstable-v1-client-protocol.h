#ifndef WLR_GAMMA_CONTROL_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define WLR_GAMMA_CONTROL_UNSTABLE_V1_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_output;
struct zwlr_gamma_control_manager_v1;
struct zwlr_gamma_control_v1;

extern const struct wl_interface zwlr_gamma_control_manager_v1_interface;
extern const struct wl_interface zwlr_gamma_control_v1_interface;

#define ZWLR_GAMMA_CONTROL_MANAGER_V1_GET_GAMMA_CONTROL 0
#define ZWLR_GAMMA_CONTROL_MANAGER_V1_DESTROY 1

#define ZWLR_GAMMA_CONTROL_MANAGER_V1_GET_GAMMA_CONTROL_SINCE_VERSION 1
#define ZWLR_GAMMA_CONTROL_MANAGER_V1_DESTROY_SINCE_VERSION 1

static inline void
zwlr_gamma_control_manager_v1_set_user_data(struct zwlr_gamma_control_manager_v1 *zwlr_gamma_control_manager_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwlr_gamma_control_manager_v1, user_data);
}

static inline void *
zwlr_gamma_control_manager_v1_get_user_data(struct zwlr_gamma_control_manager_v1 *zwlr_gamma_control_manager_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwlr_gamma_control_manager_v1);
}

static inline uint32_t
zwlr_gamma_control_manager_v1_get_version(struct zwlr_gamma_control_manager_v1 *zwlr_gamma_control_manager_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_manager_v1);
}

static inline void
zwlr_gamma_control_manager_v1_destroy(struct zwlr_gamma_control_manager_v1 *zwlr_gamma_control_manager_v1)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_gamma_control_manager_v1,
			 ZWLR_GAMMA_CONTROL_MANAGER_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_manager_v1), WL_MARSHAL_FLAG_DESTROY);
}

static inline struct zwlr_gamma_control_v1 *
zwlr_gamma_control_manager_v1_get_gamma_control(struct zwlr_gamma_control_manager_v1 *zwlr_gamma_control_manager_v1, struct wl_output *output)
{
	struct wl_proxy *id;
	id = wl_proxy_marshal_flags((struct wl_proxy *) zwlr_gamma_control_manager_v1,
			 ZWLR_GAMMA_CONTROL_MANAGER_V1_GET_GAMMA_CONTROL, &zwlr_gamma_control_v1_interface, wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_manager_v1), 0, NULL, output);
	return (struct zwlr_gamma_control_v1 *) id;
}

#ifndef ZWLR_GAMMA_CONTROL_V1_ERROR_ENUM
#define ZWLR_GAMMA_CONTROL_V1_ERROR_ENUM
enum zwlr_gamma_control_v1_error {
	ZWLR_GAMMA_CONTROL_V1_ERROR_INVALID_GAMMA = 1,
};
#endif

struct zwlr_gamma_control_v1_listener {
	void (*gamma_size)(void *data,
			   struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1,
			   uint32_t size);
	void (*failed)(void *data,
		       struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1);
};

static inline int
zwlr_gamma_control_v1_add_listener(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1,
				   const struct zwlr_gamma_control_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_gamma_control_v1,
				     (void (**)(void))(void *)(uintptr_t)listener, data);
}

#define ZWLR_GAMMA_CONTROL_V1_SET_GAMMA 0
#define ZWLR_GAMMA_CONTROL_V1_DESTROY 1

#define ZWLR_GAMMA_CONTROL_V1_GAMMA_SIZE_SINCE_VERSION 1
#define ZWLR_GAMMA_CONTROL_V1_FAILED_SINCE_VERSION 1

#define ZWLR_GAMMA_CONTROL_V1_SET_GAMMA_SINCE_VERSION 1
#define ZWLR_GAMMA_CONTROL_V1_DESTROY_SINCE_VERSION 1

static inline void
zwlr_gamma_control_v1_set_user_data(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwlr_gamma_control_v1, user_data);
}

static inline void *
zwlr_gamma_control_v1_get_user_data(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwlr_gamma_control_v1);
}

static inline uint32_t
zwlr_gamma_control_v1_get_version(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_v1);
}

static inline void
zwlr_gamma_control_v1_destroy(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_gamma_control_v1,
			 ZWLR_GAMMA_CONTROL_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_v1), WL_MARSHAL_FLAG_DESTROY);
}

static inline void
zwlr_gamma_control_v1_set_gamma(struct zwlr_gamma_control_v1 *zwlr_gamma_control_v1, int32_t fd)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_gamma_control_v1,
			 ZWLR_GAMMA_CONTROL_V1_SET_GAMMA, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_gamma_control_v1), 0, fd);
}

#ifdef  __cplusplus
}
#endif

#endif
