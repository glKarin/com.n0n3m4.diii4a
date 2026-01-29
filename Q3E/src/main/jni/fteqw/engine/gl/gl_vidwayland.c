//This is my attempt at wayland support for both opengl and vulkan.
//Note that this is sorely under-tested - I haven't tested vulkan-on-wayland at all as none of the drivers for nvidia support it.

//in no particular order...
//TODO: leaks on shutdown
//TODO: proper window decorations - zxdg_decoration_manager_v1
//TODO: kb autorepeat
//TODO: system clipboard
//TODO: drag+drop

/*
kwin unstable protocools:
 zwp_relative_pointer_manager_v1, since 5.28
 zwp_pointer_constraints_v1, since 5.29
 xdg_wm_base, since 5.48
 zxdg_decoration_manager_v1, since 5.54
*/

#include "bothdefs.h"
#ifdef WAYLANDQUAKE
#include "gl_videgl.h"	//define this BEFORE the wayland stuff. This means the EGL types will have their (x11) defaults instead of getting mixed up with wayland. we expect to be able to use the void* verions instead for wayland anyway.
#include <wayland-client.h>
#include <wayland-egl.h>
#include <linux/input.h>	//this is shite.
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include "quakedef.h"
#if defined(GLQUAKE) && defined(USE_EGL)
#include "gl_draw.h"
#endif
#if defined(VKQUAKE)
#include "vk/vkrenderer.h"
#endif

#if WAYLAND_VERSION_MAJOR < 1
#error "wayland headers are too old"
#endif

#include "glquake.h"
#include "shader.h"

extern cvar_t vid_conautoscale;

//protocol names...
#define WP_POINTER_CONSTRAINTS_NAME "zwp_pointer_constraints_v1"
#define WP_RELATIVE_POINTER_MANAGER_NAME "zwp_relative_pointer_manager_v1"
#define ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME "org_kde_kwin_server_decoration_manager" //comment out to disable once the XDG version is tested.
//FIXME: needs testing. #define XDG_DECORATION_MANAGER_NAME "zxdg_decoration_manager_v1"

//#define XDG_SHELL_UNSTABLE
#ifdef XDG_SHELL_UNSTABLE
#define XDG_SHELL_NAME "zxdg_shell_v6"
#else
#define XDG_SHELL_NAME "xdg_wm_base"
#endif
#define WL_SHELL_NAME "wl_shell"	//fall back on this if xdg_shell is missing

#ifndef STATIC_WAYLAND
	#define DYNAMIC_WAYLAND
#endif
#ifdef DYNAMIC_WAYLAND

static struct wl_display *(*pwl_display_connect)(const char *name);
static void (*pwl_display_disconnect)(struct wl_display *display);
static int (*pwl_display_dispatch)(struct wl_display *display);
static int (*pwl_display_dispatch_pending)(struct wl_display *display);
static int (*pwl_display_roundtrip)(struct wl_display *display);

static struct wl_proxy *(*pwl_proxy_marshal_constructor)(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, ...);
static struct wl_proxy *(*pwl_proxy_marshal_constructor_versioned)(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, ...);
static void (*pwl_proxy_destroy)(struct wl_proxy *proxy);
static void (*pwl_proxy_marshal)(struct wl_proxy *p, uint32_t opcode, ...);
static int (*pwl_proxy_add_listener)(struct wl_proxy *proxy, void (**implementation)(void), void *data);

static const struct wl_interface		*pwl_keyboard_interface;
static const struct wl_interface		*pwl_pointer_interface;
static const struct wl_interface		*pwl_compositor_interface;
static const struct wl_interface		*pwl_region_interface;
static const struct wl_interface		*pwl_surface_interface;
#ifdef WL_SHELL_NAME
static const struct wl_interface		*pwl_shell_surface_interface;
static const struct wl_interface		*pwl_shell_interface;
#endif
static const struct wl_interface		*pwl_seat_interface;
static const struct wl_interface		*pwl_registry_interface;
static const struct wl_interface		*pwl_output_interface;
static const struct wl_interface		*pwl_shm_interface;
static const struct wl_interface		*pwl_shm_pool_interface;
static const struct wl_interface		*pwl_buffer_interface;
static const struct wl_interface		*pwl_callback_interface;

static dllfunction_t waylandexports_wl[] =
{
	{(void**)&pwl_display_connect,						"wl_display_connect"},
	{(void**)&pwl_display_disconnect,					"wl_display_disconnect"},
	{(void**)&pwl_display_dispatch,						"wl_display_dispatch"},
	{(void**)&pwl_display_dispatch_pending,				"wl_display_dispatch_pending"},
	{(void**)&pwl_display_roundtrip,					"wl_display_roundtrip"},
	{(void**)&pwl_proxy_marshal_constructor,			"wl_proxy_marshal_constructor"},
	{(void**)&pwl_proxy_marshal_constructor_versioned,	"wl_proxy_marshal_constructor_versioned"},
	{(void**)&pwl_proxy_destroy,						"wl_proxy_destroy"},
	{(void**)&pwl_proxy_marshal,						"wl_proxy_marshal"},
	{(void**)&pwl_proxy_add_listener,					"wl_proxy_add_listener"},
	{(void**)&pwl_keyboard_interface,					"wl_keyboard_interface"},
	{(void**)&pwl_pointer_interface,					"wl_pointer_interface"},
	{(void**)&pwl_compositor_interface,					"wl_compositor_interface"},
	{(void**)&pwl_region_interface,						"wl_region_interface"},
	{(void**)&pwl_surface_interface,					"wl_surface_interface"},
#ifdef WL_SHELL_NAME
	{(void**)&pwl_shell_surface_interface,				"wl_shell_surface_interface"},
	{(void**)&pwl_shell_interface,						"wl_shell_interface"},
#endif
	{(void**)&pwl_seat_interface,						"wl_seat_interface"},
	{(void**)&pwl_registry_interface,					"wl_registry_interface"},
	{(void**)&pwl_output_interface,						"wl_output_interface"},
	{(void**)&pwl_shm_interface,						"wl_shm_interface"},
	{(void**)&pwl_shm_pool_interface,					"wl_shm_pool_interface"},
	{(void**)&pwl_buffer_interface,						"wl_buffer_interface"},
	{(void**)&pwl_callback_interface,					"wl_callback_interface"},
	{NULL, NULL}
};
static dllhandle_t *lib_wayland_wl;
static qboolean WL_InitLibrary(void)
{
	lib_wayland_wl = Sys_LoadLibrary("libwayland-client.so.0", waylandexports_wl);
	if (!lib_wayland_wl)
		return false;
	return true;
}

static dllhandle_t *lib_xkb;
static struct xkb_context *(*pxkb_context_new)(enum xkb_context_flags flags);
static struct xkb_keymap *(*pxkb_keymap_new_from_string)(struct xkb_context *context, const char *string, enum xkb_keymap_format format, enum xkb_keymap_compile_flags flags);
static struct xkb_state *(*pxkb_state_new)(struct xkb_keymap *keymap);
static xkb_keysym_t (*pxkb_state_key_get_one_sym)(struct xkb_state *state, xkb_keycode_t key);
static uint32_t (*pxkb_keysym_to_utf32)(xkb_keysym_t keysym);
static void (*pxkb_state_unref)(struct xkb_state *state);
static void (*pxkb_keymap_unref)(struct xkb_keymap *keymap);
static enum xkb_state_component (*pxkb_state_update_mask)(struct xkb_state *state, xkb_mod_mask_t depressed_mods, xkb_mod_mask_t latched_mods, xkb_mod_mask_t locked_mods, xkb_layout_index_t depressed_layout, xkb_layout_index_t latched_layout, xkb_layout_index_t locked_layout);
static qboolean WL_InitLibraryXKB(void)
{
	static dllfunction_t tab[] =
	{
		{(void**)&pxkb_context_new,						"xkb_context_new"},
		{(void**)&pxkb_keymap_new_from_string,			"xkb_keymap_new_from_string"},
		{(void**)&pxkb_state_new,						"xkb_state_new"},
		{(void**)&pxkb_state_update_mask,				"xkb_state_update_mask"},
		{(void**)&pxkb_state_key_get_one_sym,			"xkb_state_key_get_one_sym"},
		{(void**)&pxkb_keysym_to_utf32,					"xkb_keysym_to_utf32"},
		{(void**)&pxkb_state_unref,						"xkb_state_unref"},
		{(void**)&pxkb_keymap_unref,					"xkb_keymap_unref"},
		{NULL, NULL}
	};
	lib_xkb = Sys_LoadLibrary("libxkbcommon.so.0", tab);
	return !!lib_xkb;
}

#if defined(GLQUAKE) && defined(USE_EGL)
static struct wl_egl_window *(*pwl_egl_window_create)(struct wl_surface *surface, int width, int height);
static void (*pwl_egl_window_destroy)(struct wl_egl_window *egl_window);
static void (*pwl_egl_window_resize)(struct wl_egl_window *egl_window, int width, int height, int dx, int dy);
//static void (*pwl_egl_window_get_attached_size(struct wl_egl_window *egl_window, int *width, int *height);
static dllfunction_t waylandexports_egl[] =
{
	{(void**)&pwl_egl_window_create,		"wl_egl_window_create"},
	{(void**)&pwl_egl_window_destroy,		"wl_egl_window_destroy"},
	{(void**)&pwl_egl_window_resize,		"wl_egl_window_resize"},
//	{(void**)&pwl_egl_window_get_attached_size,	"wl_egl_window_get_attached_size"},
	{NULL, NULL}
};
static dllhandle_t *lib_wayland_egl;
#endif


//I hate wayland.
static inline struct wl_region *pwl_compositor_create_region(struct wl_compositor *wl_compositor)				{return (struct wl_region*)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_compositor, WL_COMPOSITOR_CREATE_REGION, pwl_region_interface, NULL);}
static inline struct wl_surface *pwl_compositor_create_surface(struct wl_compositor *wl_compositor)				{return (struct wl_surface *)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_compositor, WL_COMPOSITOR_CREATE_SURFACE, pwl_surface_interface, NULL);}
static inline void pwl_surface_set_opaque_region(struct wl_surface *wl_surface, struct wl_region *region)		{pwl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_SET_OPAQUE_REGION, region);}
static inline void pwl_region_add(struct wl_region *wl_region, int32_t x, int32_t y, int32_t width, int32_t height)	{pwl_proxy_marshal((struct wl_proxy *) wl_region, WL_REGION_ADD, x, y, width, height);}
#ifdef WL_SHELL_NAME
static inline struct wl_shell_surface *pwl_shell_get_shell_surface(struct wl_shell *wl_shell, struct wl_surface *surface)	{return (struct wl_shell_surface *)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_shell, WL_SHELL_GET_SHELL_SURFACE, pwl_shell_surface_interface, NULL, surface);}
static inline void pwl_shell_surface_set_toplevel(struct wl_shell_surface *wl_shell_surface)					{pwl_proxy_marshal((struct wl_proxy *) wl_shell_surface, WL_SHELL_SURFACE_SET_TOPLEVEL);}
static inline void pwl_shell_surface_set_fullscreen(struct wl_shell_surface *wl_shell_surface, uint32_t method, uint32_t framerate, struct wl_output *output)	{pwl_proxy_marshal((struct wl_proxy *) wl_shell_surface, WL_SHELL_SURFACE_SET_FULLSCREEN, method, framerate, output);}
static inline int pwl_shell_surface_add_listener(struct wl_shell_surface *wl_shell_surface, const struct wl_shell_surface_listener *listener, void *data)		{return pwl_proxy_add_listener((struct wl_proxy *) wl_shell_surface, (void (**)(void)) listener, data);}
static inline void pwl_shell_surface_pong(struct wl_shell_surface *wl_shell_surface, uint32_t serial)			{pwl_proxy_marshal((struct wl_proxy *) wl_shell_surface, WL_SHELL_SURFACE_PONG, serial);}
static inline void pwl_shell_surface_set_title(struct wl_shell_surface *wl_shell_surface, const char *title)	{pwl_proxy_marshal((struct wl_proxy *) wl_shell_surface, WL_SHELL_SURFACE_SET_TITLE, title);}
#endif
static inline struct wl_registry *pwl_display_get_registry(struct wl_display *wl_display)	{return (struct wl_registry *)pwl_proxy_marshal_constructor((struct wl_proxy *) wl_display, WL_DISPLAY_GET_REGISTRY, pwl_registry_interface, NULL);}
static inline void *pwl_registry_bind(struct wl_registry *wl_registry, uint32_t name, const struct wl_interface *interface, uint32_t version)	{return (void*)pwl_proxy_marshal_constructor_versioned((struct wl_proxy *) wl_registry, WL_REGISTRY_BIND, interface, version, name, interface->name, version, NULL);}
static inline int pwl_registry_add_listener(struct wl_registry *wl_registry, const struct wl_registry_listener *listener, void *data)			{return pwl_proxy_add_listener((struct wl_proxy *) wl_registry, (void (**)(void)) listener, data);}
static inline void pwl_keyboard_destroy(struct wl_keyboard *wl_keyboard)			{pwl_proxy_destroy((struct wl_proxy *) wl_keyboard);}
static inline int pwl_keyboard_add_listener(struct wl_keyboard *wl_keyboard, const struct wl_keyboard_listener *listener, void *data)			{return pwl_proxy_add_listener((struct wl_proxy *) wl_keyboard, (void (**)(void)) listener, data);}
static inline void pwl_pointer_destroy(struct wl_pointer *wl_pointer)				{pwl_proxy_destroy((struct wl_proxy *) wl_pointer);}
static inline int pwl_pointer_add_listener(struct wl_pointer *wl_pointer, const struct wl_pointer_listener *listener, void *data)				{return pwl_proxy_add_listener((struct wl_proxy *) wl_pointer, (void (**)(void)) listener, data);}
static inline struct wl_pointer *pwl_seat_get_pointer(struct wl_seat *wl_seat)		{return (struct wl_pointer *)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_seat, WL_SEAT_GET_POINTER, pwl_pointer_interface, NULL);}
static inline struct wl_keyboard *pwl_seat_get_keyboard(struct wl_seat *wl_seat)	{return (struct wl_keyboard *)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_seat, WL_SEAT_GET_KEYBOARD, pwl_keyboard_interface, NULL);}
static inline int pwl_seat_add_listener(struct wl_seat *wl_seat, const struct wl_seat_listener *listener, void *data)	{return pwl_proxy_add_listener((struct wl_proxy *) wl_seat, (void (**)(void)) listener, data);}

static inline struct wl_callback *pwl_surface_frame(struct wl_surface *wl_surface)	{return (struct wl_callback*)(struct wl_proxy *) pwl_proxy_marshal_constructor((struct wl_proxy *) wl_surface, WL_SURFACE_FRAME, pwl_callback_interface, NULL);}
static inline int pwl_callback_add_listener(struct wl_callback *wl_callback, const struct wl_callback_listener *listener, void *data) {return pwl_proxy_add_listener((struct wl_proxy *) wl_callback, (void (**)(void)) listener, data);}

static inline void pwl_pointer_set_cursor(struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, int32_t hotspot_x, int32_t hotspot_y)		{ pwl_proxy_marshal((struct wl_proxy *) wl_pointer, WL_POINTER_SET_CURSOR, serial, surface, hotspot_x, hotspot_y); }
static inline struct wl_shm_pool *pwl_shm_create_pool(struct wl_shm *wl_shm, int32_t fd, int32_t size)		{ return (struct wl_shm_pool *)pwl_proxy_marshal_constructor((struct wl_proxy *) wl_shm, WL_SHM_CREATE_POOL, pwl_shm_pool_interface, NULL, fd, size); }
static inline struct wl_buffer *pwl_shm_pool_create_buffer(struct wl_shm_pool *wl_shm_pool, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format)		{ return (struct wl_buffer *)pwl_proxy_marshal_constructor((struct wl_proxy *) wl_shm_pool, WL_SHM_POOL_CREATE_BUFFER, pwl_buffer_interface, NULL, offset, width, height, stride, format); }
static inline void pwl_shm_pool_destroy(struct wl_shm_pool *wl_shm_pool)		{ pwl_proxy_marshal((struct wl_proxy *) wl_shm_pool, WL_SHM_POOL_DESTROY); pwl_proxy_destroy((struct wl_proxy *) wl_shm_pool); }
static inline void pwl_surface_commit(struct wl_surface *wl_surface)		{ pwl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_COMMIT); }
static inline void pwl_buffer_destroy(struct wl_buffer *wl_buffer)		{ pwl_proxy_marshal((struct wl_proxy *) wl_buffer, WL_BUFFER_DESTROY); pwl_proxy_destroy((struct wl_proxy *) wl_buffer); }
static inline void pwl_surface_attach(struct wl_surface *wl_surface, struct wl_buffer *buffer, int32_t x, int32_t y)		{ pwl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_ATTACH, buffer, x, y); }
static inline void pwl_surface_damage(struct wl_surface *wl_surface, int32_t x, int32_t y, int32_t width, int32_t height)		{ pwl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_DAMAGE, x, y, width, height); }
static inline void pwl_surface_destroy(struct wl_surface *wl_surface)		{ pwl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_DESTROY); pwl_proxy_destroy((struct wl_proxy *) wl_surface); }

#else
#define pwl_keyboard_interface		&wl_keyboard_interface
#define pwl_pointer_interface		&wl_pointer_interface
#define pwl_compositor_interface	&wl_compositor_interface
#define pwl_registry_interface		&wl_registry_interface
#define pwl_region_interface		&wl_region_interface
#define pwl_surface_interface		&wl_surface_interface
#ifdef WL_SHELL_NAME
#define pwl_shell_surface_interface	&wl_shell_surface_interface
#define pwl_shell_interface			&wl_shell_interface
#endif
#define pwl_seat_interface			&wl_seat_interface
#define pwl_output_interface		&wl_output_interface
#define pwl_shm_interface			&wl_shm_interface

#define pwl_display_connect					wl_display_connect
#define pwl_display_disconnect				wl_display_disconnect
#define pwl_display_dispatch				wl_display_dispatch
#define pwl_display_dispatch_pending		wl_display_dispatch_pending
#define pwl_display_roundtrip				wl_display_roundtrip

#define pwl_proxy_marshal					wl_proxy_marshal
#define pwl_proxy_marshal_constructor		wl_proxy_marshal_constructor
#define pwl_proxy_marshal_constructor_versioned		wl_proxy_marshal_constructor_versioned
#define pwl_proxy_destroy					wl_proxy_destroy
#define pwl_proxy_add_listener				wl_proxy_add_listener

#define pwl_compositor_create_region		wl_compositor_create_region
#define pwl_compositor_create_surface		wl_compositor_create_surface
#define pwl_surface_set_opaque_region		wl_surface_set_opaque_region
#define pwl_region_add						wl_region_add
#define pwl_shell_get_shell_surface			wl_shell_get_shell_surface
#define pwl_shell_surface_set_toplevel		wl_shell_surface_set_toplevel
#define pwl_shell_surface_set_fullscreen	wl_shell_surface_set_fullscreen
#define pwl_shell_surface_add_listener		wl_shell_surface_add_listener
#define pwl_shell_surface_pong				wl_shell_surface_pong
#define pwl_shell_surface_set_title			wl_shell_surface_set_title
#define pwl_display_get_registry			wl_display_get_registry
#define pwl_registry_bind					wl_registry_bind
#define pwl_registry_add_listener			wl_registry_add_listener
#define pwl_keyboard_destroy				wl_keyboard_destroy
#define pwl_keyboard_add_listener			wl_keyboard_add_listener
#define pwl_pointer_destroy					wl_pointer_destroy
#define pwl_pointer_add_listener			wl_pointer_add_listener
#define pwl_seat_get_pointer				wl_seat_get_pointer
#define pwl_seat_get_keyboard				wl_seat_get_keyboard
#define pwl_seat_add_listener				wl_seat_add_listener
#define pwl_surface_frame					wl_surface_frame
#define pwl_callback_add_listener			wl_callback_add_listener
#define pwl_pointer_set_cursor				wl_pointer_set_cursor
#define pwl_shm_create_pool					wl_shm_create_pool
#define pwl_shm_pool_create_buffer			wl_shm_pool_create_buffer
#define pwl_shm_pool_destroy				wl_shm_pool_destroy
#define pwl_surface_commit					wl_surface_commit
#define pwl_buffer_destroy					wl_buffer_destroy
#define pwl_surface_attach					wl_surface_attach
#define pwl_surface_damage					wl_surface_damage
#define pwl_surface_destroy					wl_surface_destroy

#define pwl_egl_window_create				wl_egl_window_create
#define pwl_egl_window_destroy				wl_egl_window_destroy
#define pwl_egl_window_resize				wl_egl_window_resize

#define pxkb_context_new					xkb_context_new
#define pxkb_keymap_new_from_string			xkb_keymap_new_from_string
#define pxkb_state_new						xkb_state_new
#define pxkb_state_key_get_one_sym			xkb_state_key_get_one_sym
#define pxkb_keysym_to_utf32				xkb_keysym_to_utf32
#define pxkb_state_unref					xkb_state_unref
#define pxkb_keymap_unref					xkb_keymap_unref
#define pxkb_state_update_mask				xkb_state_update_mask

#endif
static const struct wl_interface *pzwp_relative_pointer_v1_interface;
static const struct wl_interface *pzwp_locked_pointer_v1_interface;

struct cursorinfo_s
{
	struct wl_surface *surf;	//the surface used as a cursor.
	struct wl_buffer *buf;		//can't destroy this too early, or the server bugs out.
	int hot_x, hot_y;			//hotspot stuff
	int width, height;			//for invalidating the surface to avoid server bugs.
};

static struct wdisplay_s
{
	//display stuff
	struct wl_display *display;	//manager
	struct wl_registry *registry;	//manager
	struct wl_compositor *compositor;	//manager
	struct wl_shm *shm;	//manager

	//seat stuff
	void *pointer;
	qboolean cursorfocus;	//says that we 'own' the hardware cursor
	uint32_t cursorserial;	//required for updating the cursor image
	struct cursorinfo_s *cursor;
	struct wl_keyboard *keyboard;
	struct wl_seat *seat;

	//because wayland really doesn't help when it comes to various keymaps
	struct xkb_context *xkb_context;
	struct xkb_state *xkb_state;
	//to try to fix mouse stuff
	qboolean relative_mouse_active;
#ifdef WP_POINTER_CONSTRAINTS_NAME
	struct zwp_pointer_constraints_v1 *pointer_constraints;
	struct zwp_locked_pointer_v1 *locked_pointer;
#endif
#ifdef WP_RELATIVE_POINTER_MANAGER_NAME
	struct zwp_relative_pointer_manager_v1 *relative_pointer_manager;
	struct zwp_relative_pointer_v1 *relative_pointer;
#endif

	//stupid csd crap to work around shitty wayland servers
	int truewidth;	//not really needed, but present for consistency
	int trueheight;
	int csdsize;
	char *csdcaption;
	qboolean hasssd;	//probably false on gnome.
	int mousex,mousey;

	//window stuff
#if defined(GLQUAKE) && defined(USE_EGL)
	struct wl_egl_window *enwindow;
#endif
	struct wl_surface *surface;

#ifdef XDG_SHELL_NAME
	struct xdg_toplevel *xdg_toplevel;
	struct xdg_surface *xdg_surface;
	struct xdg_wm_base *xdg_wm_base;	//manager
#endif
#ifdef WL_SHELL_NAME
	struct wl_shell_surface *ssurface;
	struct wl_shell *shell;	//manager
#endif
	qboolean wait_for_configure;	//WL is being slow and won't let us redraw yet.
	qboolean waylandisblocking;

	//belated sanity stuff
#ifdef XDG_DECORATION_MANAGER_NAME
	struct zxdg_toplevel_decoration_v1 *xdg_decoration;
	struct zxdg_decoration_manager_v1 *xdg_decoration_manager;
#endif
#ifdef ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME
	struct org_kde_kwin_server_decoration *kwin_decoration;
	struct org_kde_kwin_server_decoration_manager *kwin_decoration_manager;
#endif
} w;
static void WL_SetCaption(const char *text);



#ifdef XDG_SHELL_NAME
static const struct wl_interface *pxdg_surface_interface;
static const struct wl_interface *pxdg_toplevel_interface;
static const struct wl_interface *pxdg_wm_base_interface;
static void WL_Setup_XDG_Shell(void)
{
	static const struct wl_interface *types[24];

	static const struct wl_message xdg_wm_base_requests[] = {
		{ "destroy", "", types + 0 },
		{ "create_positioner", "n", types + 4 },
		{ "get_xdg_surface", "no", types + 5 },
		{ "pong", "u", types + 0 },
	};
	static const struct wl_message xdg_wm_base_events[] = {
		{ "ping", "u", types + 0 },
	};

	static const struct wl_interface xdg_wm_base_interface = {
#ifdef XDG_SHELL_UNSTABLE
		"zxdg_wm_base_v6", 1,
#else
		"xdg_wm_base", 2,
#endif
		4, xdg_wm_base_requests,
		1, xdg_wm_base_events,
	};

	static const struct wl_message xdg_positioner_requests[] = {
		{ "destroy", "", types + 0 },
		{ "set_size", "ii", types + 0 },
		{ "set_anchor_rect", "iiii", types + 0 },
		{ "set_anchor", "u", types + 0 },
		{ "set_gravity", "u", types + 0 },
		{ "set_constraint_adjustment", "u", types + 0 },
		{ "set_offset", "ii", types + 0 },
	};

	static const struct wl_interface xdg_positioner_interface = {
#ifdef XDG_SHELL_UNSTABLE
		"zxdg_positioner_v6", 1,
#else
		"xdg_positioner", 2,
#endif
		7, xdg_positioner_requests,
		0, NULL,
	};

	static const struct wl_message xdg_surface_requests[] = {
		{ "destroy", "", types + 0 },
		{ "get_toplevel", "n", types + 7 },
#ifdef XDG_SHELL_UNSTABLE
		{ "get_popup", "noo", types + 8 },
#else
		{ "get_popup", "n?oo", types + 8 },
#endif
		{ "set_window_geometry", "iiii", types + 0 },
		{ "ack_configure", "u", types + 0 },
	};

	static const struct wl_message xdg_surface_events[] = {
		{ "configure", "u", types + 0 },
	};

	static const struct wl_interface xdg_surface_interface = {
#ifdef XDG_SHELL_UNSTABLE
		"zxdg_surface_v6", 1,
#else
		"xdg_surface", 2,
#endif
		5, xdg_surface_requests,
		1, xdg_surface_events,
	};

	static const struct wl_message xdg_toplevel_requests[] = {
		{ "destroy", "", types + 0 },
		{ "set_parent", "?o", types + 11 },
		{ "set_title", "s", types + 0 },
		{ "set_app_id", "s", types + 0 },
		{ "show_window_menu", "ouii", types + 12 },
		{ "move", "ou", types + 16 },
		{ "resize", "ouu", types + 18 },
		{ "set_max_size", "ii", types + 0 },
		{ "set_min_size", "ii", types + 0 },
		{ "set_maximized", "", types + 0 },
		{ "unset_maximized", "", types + 0 },
		{ "set_fullscreen", "?o", types + 21 },
		{ "unset_fullscreen", "", types + 0 },
		{ "set_minimized", "", types + 0 },
	};

	static const struct wl_message xdg_toplevel_events[] = {
		{ "configure", "iia", types + 0 },
		{ "close", "", types + 0 },
	};

	static const struct wl_interface xdg_toplevel_interface = {
#ifdef XDG_SHELL_UNSTABLE
		"zxdg_toplevel_v6", 1,
#else
		"xdg_toplevel", 2,
#endif
		14, xdg_toplevel_requests,
		2, xdg_toplevel_events,
	};

	static const struct wl_message xdg_popup_requests[] = {
		{ "destroy", "", types + 0 },
		{ "grab", "ou", types + 22 },
	};
	static const struct wl_message xdg_popup_events[] = {
		{ "configure", "iiii", types + 0 },
		{ "popup_done", "", types + 0 },
	};

	static const struct wl_interface xdg_popup_interface = {
#ifdef UNSTABLE
		"zxdg_popup_v6", 1,
#else
		"xdg_popup", 2,
#endif
		2, xdg_popup_requests,
		2, xdg_popup_events,
	};

	types[4] = &xdg_positioner_interface;
	types[5] = &xdg_surface_interface;
	types[6] = pwl_surface_interface;
	types[7] = &xdg_toplevel_interface;
	types[8] = &xdg_popup_interface;
	types[9] = &xdg_surface_interface;
	types[10] = &xdg_positioner_interface;
	types[11] = &xdg_toplevel_interface;
	types[12] = pwl_seat_interface;
	types[16] = pwl_seat_interface;
	types[18] = pwl_seat_interface;
	types[21] = pwl_output_interface;
	types[22] = pwl_seat_interface;

	pxdg_surface_interface = &xdg_surface_interface;
	pxdg_toplevel_interface = &xdg_toplevel_interface;
	pxdg_wm_base_interface = &xdg_wm_base_interface;
}
#define XDG_WM_BASE_DESTROY 0
#define XDG_WM_BASE_CREATE_POSITIONER 1
#define XDG_WM_BASE_GET_XDG_SURFACE 2
#define XDG_WM_BASE_PONG 3
struct xdg_wm_base_listener {
    void (*ping)(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
};
static inline int xdg_wm_base_add_listener(struct xdg_wm_base *xdg_wm_base, const struct xdg_wm_base_listener *listener, void *data)		{ return pwl_proxy_add_listener((struct wl_proxy *) xdg_wm_base, (void (**)(void)) listener, data); }
static inline struct xdg_surface *xdg_wm_base_get_xdg_surface(struct xdg_wm_base *xdg_wm_base, struct wl_surface *surface)		{ return (struct xdg_surface *)pwl_proxy_marshal_constructor((struct wl_proxy *) xdg_wm_base, XDG_WM_BASE_GET_XDG_SURFACE, pxdg_surface_interface, NULL, surface); }
static inline void xdg_wm_base_pong(struct xdg_wm_base *xdg_wm_base, uint32_t serial)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_wm_base, XDG_WM_BASE_PONG, serial); }

#define XDG_SURFACE_DESTROY 0
#define XDG_SURFACE_GET_TOPLEVEL 1
#define XDG_SURFACE_GET_POPUP 2
#define XDG_SURFACE_SET_WINDOW_GEOMETRY 3
#define XDG_SURFACE_ACK_CONFIGURE 4
struct xdg_surface_listener {
    void (*configure)(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
};
static inline int xdg_surface_add_listener(struct xdg_surface *xdg_surface, const struct xdg_surface_listener *listener, void *data) { return pwl_proxy_add_listener((struct wl_proxy *) xdg_surface, (void (**)(void)) listener, data); }
static inline struct xdg_toplevel *xdg_surface_get_toplevel(struct xdg_surface *xdg_surface)		{ return (struct xdg_toplevel *)pwl_proxy_marshal_constructor((struct wl_proxy *) xdg_surface, XDG_SURFACE_GET_TOPLEVEL, pxdg_toplevel_interface, NULL); }
static inline void xdg_surface_ack_configure(struct xdg_surface *xdg_surface, uint32_t serial)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_surface, XDG_SURFACE_ACK_CONFIGURE, serial); }

#define XDG_TOPLEVEL_DESTROY 0
#define XDG_TOPLEVEL_SET_PARENT 1
#define XDG_TOPLEVEL_SET_TITLE 2
#define XDG_TOPLEVEL_SET_APP_ID 3
#define XDG_TOPLEVEL_SHOW_WINDOW_MENU 4
#define XDG_TOPLEVEL_MOVE 5
#define XDG_TOPLEVEL_RESIZE 6
#define XDG_TOPLEVEL_SET_MAX_SIZE 7
#define XDG_TOPLEVEL_SET_MIN_SIZE 8
#define XDG_TOPLEVEL_SET_MAXIMIZED 9
#define XDG_TOPLEVEL_UNSET_MAXIMIZED 10
#define XDG_TOPLEVEL_SET_FULLSCREEN 11
#define XDG_TOPLEVEL_UNSET_FULLSCREEN 12
#define XDG_TOPLEVEL_SET_MINIMIZED 13
struct xdg_toplevel_listener;
static inline void xdg_toplevel_set_title(struct xdg_toplevel *xdg_toplevel, const char *title)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_toplevel, XDG_TOPLEVEL_SET_TITLE, title); }
static inline int xdg_toplevel_add_listener(struct xdg_toplevel *xdg_toplevel, const struct xdg_toplevel_listener *listener, void *data)		{ return pwl_proxy_add_listener((struct wl_proxy *) xdg_toplevel, (void (**)(void)) listener, data); }
static inline void xdg_toplevel_set_min_size(struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_toplevel, XDG_TOPLEVEL_SET_MIN_SIZE, width, height); }
static inline void xdg_toplevel_move(struct xdg_toplevel *xdg_toplevel, struct wl_seat *seat, uint32_t serial)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_toplevel, XDG_TOPLEVEL_MOVE, seat, serial); }
static inline void xdg_toplevel_show_window_menu(struct xdg_toplevel *xdg_toplevel, struct wl_seat *seat, uint32_t serial, int32_t x, int32_t y)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_toplevel, XDG_TOPLEVEL_SHOW_WINDOW_MENU, seat, serial, x, y); }
static inline void xdg_toplevel_set_fullscreen(struct xdg_toplevel *xdg_toplevel, struct wl_output *output)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_toplevel, XDG_TOPLEVEL_SET_FULLSCREEN, output); }

static void xdg_wm_base_handle_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener myxdg_wm_base_listener =
{
	xdg_wm_base_handle_ping,
};

static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);

	w.wait_for_configure = false; //all good now
}
static const struct xdg_surface_listener myxdg_surface_listener =
{
	xdg_surface_handle_configure,
};

void xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	if (!width || !height)
	{
		width = w.truewidth;
		height = w.trueheight;
	}

#if defined(GLQUAKE) && defined(USE_EGL)
	if (w.enwindow)
		pwl_egl_window_resize(w.enwindow, width, height, 0, 0);
#endif

	w.truewidth = width;
	w.trueheight = height;
	if (vid.pixelwidth != width || vid.pixelheight != height-w.csdsize)
	{
		vid.pixelwidth = width;
		vid.pixelheight = w.trueheight-w.csdsize;
		Cvar_ForceCallback(&vid_conautoscale);
	}
}
void xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	//fixme: steal focus, bring to top, or just hope the compositor is doing that for us when the user right-clicked the task bar of our minimised app or whatever.
	M_Window_ClosePrompt();
}
static struct xdg_toplevel_listener {
    void (*configure)(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
    void (*close)(void *data, struct xdg_toplevel *xdg_toplevel);
} myxdg_toplevel_listener =
{
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close
};

#endif

#ifdef WL_SHELL_NAME
static void WL_shell_handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
	pwl_shell_surface_pong(shell_surface, serial);
}
static void WL_shell_handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
#if defined(GLQUAKE) && defined(USE_EGL)
	if (w.enwindow)
		pwl_egl_window_resize(w.enwindow, width, height, 0, 0);
#endif

	w.truewidth = width;
	w.trueheight = height;
	if (vid.pixelwidth != width || vid.pixelheight != height-w.csdsize)
	{
		vid.pixelwidth = width;
		vid.pixelheight = height-w.csdsize;
		Cvar_ForceCallback(&vid_conautoscale);
	}

	w.wait_for_configure = false; //all good now
}
static void WL_shell_handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener =
{
	WL_shell_handle_ping,
	WL_shell_handle_configure,
	WL_shell_handle_popup_done
};
#endif

//qkeys are ascii-compatible for the most part.
static unsigned short waylandinputsucksbighairydonkeyballs[] =
{
	0, 	K_ESCAPE,'1','2','3','4','5','6',	//0x
	'7','8','9','0','-','=',K_BACKSPACE,K_TAB,
	'q','w','e','r','t','y','u','i',		//1x
	'o','p','[',']',K_ENTER,K_LCTRL,'a', 's',
	'd','f','g','h','j','k','l',';',		//2x
	'\'','`',K_LSHIFT,'#','z','x','c','v',
	'b','n','m',',','.','/',K_RSHIFT,K_KP_STAR,	//3x
	K_LALT,' ',K_CAPSLOCK,K_F1,K_F2,K_F3,K_F4,K_F5,
	K_F6,K_F7,K_F8,K_F9,K_F10,K_KP_NUMLOCK,K_SCRLCK,K_KP_HOME,//4x
	K_KP_UPARROW,K_KP_PGUP,K_KP_MINUS,K_KP_LEFTARROW,K_KP_5,K_KP_RIGHTARROW,K_KP_PLUS,K_KP_END,
	K_KP_DOWNARROW,K_KP_PGDN,K_KP_INS,K_KP_DEL,0,0,'\\',K_F11,	//5x
	K_F12,0,0,0,0,0,0,0,
	K_KP_ENTER,K_RCTRL,K_KP_SLASH,K_PRINTSCREEN,K_RALT,0,K_HOME,K_UPARROW,	//6x
	K_PGUP,K_LEFTARROW,K_RIGHTARROW,K_END,K_DOWNARROW,K_PGDN,K_INS,K_DEL,
	0,0,0,0,0,0,0,K_PAUSE,	//7x
	0,0,0,0,0,K_LWIN,K_RWIN,K_APP
};
static unsigned short waylandinputsucksbighairydonkeyballsshift[] =
{
	0, 	K_ESCAPE,'!','\"','3','$','%','^',	//0x
	'&','*','(',')','_','+',K_BACKSPACE,K_TAB,
	'Q','W','E','R','T','Y','U','I',		//1x
	'O','P','{','}',K_ENTER,K_LCTRL,'A', 'S',
	'D','F','G','H','J','K','L',':',		//2x
	'@','`',K_LSHIFT,'~','Z','X','C','V',
	'B','N','M','<','>','?',K_RSHIFT,K_KP_STAR,	//3x
	K_LALT,' ',K_CAPSLOCK,K_F1,K_F2,K_F3,K_F4,K_F5,
	K_F6,K_F7,K_F8,K_F9,K_F10,K_KP_NUMLOCK,K_SCRLCK,K_KP_HOME,//4x
	K_KP_UPARROW,K_KP_PGUP,K_KP_MINUS,K_KP_LEFTARROW,K_KP_5,K_KP_RIGHTARROW,K_KP_PLUS,K_KP_END,
	K_KP_DOWNARROW,K_KP_PGDN,K_KP_INS,K_KP_DEL,0,0,'|',K_F11,	//5x
	K_F12,0,0,0,0,0,0,0,
	K_KP_ENTER,K_RCTRL,K_KP_SLASH,K_PRINTSCREEN,K_RALT,0,K_HOME,K_UPARROW,	//6x
	K_PGUP,K_LEFTARROW,K_RIGHTARROW,K_END,K_DOWNARROW,K_PGDN,K_INS,K_DEL,
	0,0,0,0,0,0,0,K_PAUSE,	//7x
	0,0,0,0,0,K_LWIN,K_RWIN,K_APP	
};

static void WL_SetHWCursor(void)
{	//called when the hardware cursor needs to be re-applied

//	struct wl_buffer *buffer;
//	struct wl_cursor *cursor = s->default_cursor;
//	struct wl_cursor_image *image;

	if (!w.cursorfocus)
		return;	//nope, we don't own the cursor, don't try changing it.

	/*'If surface is NULL, the pointer image is hidden'*/
	if (!w.relative_mouse_active && !w.locked_pointer && w.cursor)
	{
		struct cursorinfo_s *c = w.cursor;
		//weston bugs out if we don't redo this junk
		pwl_surface_attach(c->surf, c->buf, 0, 0);
		pwl_surface_damage(c->surf, 0, 0, c->width, c->height);
		pwl_surface_commit(c->surf);

		//now actually set it.
		pwl_pointer_set_cursor(w.pointer, w.cursorserial, c->surf, c->hot_x, c->hot_y);
	}
	else
		pwl_pointer_set_cursor(w.pointer, w.cursorserial, NULL, 0, 0);
}
void *WL_CreateCursor(const qbyte *imagedata, int width, int height, uploadfmt_t format, float hotx, float hoty, float scale)
{
	struct wl_surface *surf = NULL;
	enum wl_shm_format shmfmt;
	int pbytes;
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer = NULL;
	qbyte *outdata;
	size_t size;
	const char *xdg_runtime_dir = getenv ("XDG_RUNTIME_DIR");
	int fd;
	struct cursorinfo_s *c;
	static qboolean allowfmts[PTI_MAX] = {[PTI_BGRX8]=true, [PTI_BGRA8]=true};	//FIXME: populate via wl_shm_add_listener instead of using formats that we THINK will work.
	struct pendingtextureinfo mips = {};

	if (!w.shm)
		return NULL;	//can't shm the image to the server

	mips.mipcount = 1;
	mips.type = PTI_2D;
	mips.encoding = format;
	mips.extrafree = NULL;

	mips.mip[0].width = width*scale;
	mips.mip[0].height = height*scale;
	mips.mip[0].depth = 1;
	mips.mip[0].data = Image_ResampleTexture(format, imagedata, width, height, NULL, mips.mip[0].width, mips.mip[0].height);
	mips.mip[0].needfree = true;
	mips.mip[0].datasize = 0;
	if (!allowfmts[mips.encoding])
		Image_ChangeFormat(&mips, allowfmts, format, "cursor");
	if (!allowfmts[mips.encoding])
		return NULL;	//failure...

	switch(mips.encoding)
	{	//fte favours byte orders, while packed formats are ordered as hex numbers would be
		//wayland formats are as hex (explicitly little-endian, so byteswapped)
	case PTI_RGBA8:	pbytes = 4; shmfmt = WL_SHM_FORMAT_ABGR8888;	break;
	case PTI_RGBX8:	pbytes = 4; shmfmt = WL_SHM_FORMAT_XBGR8888;	break;
	case PTI_LLLA8: //just fall through
	case PTI_BGRA8:	pbytes = 4; shmfmt = WL_SHM_FORMAT_ARGB8888;	break;
	case PTI_LLLX8: //just fall through
	case PTI_BGRX8:	pbytes = 4; shmfmt = WL_SHM_FORMAT_XRGB8888;	break;
	case PTI_RGB8:	pbytes = 3; shmfmt = WL_SHM_FORMAT_BGR888;		break;
	case PTI_BGR8:	pbytes = 3; shmfmt = WL_SHM_FORMAT_RGB888;		break;

	case PTI_RGB565:pbytes = 2; shmfmt = WL_SHM_FORMAT_RGB565;		break;
	case PTI_A2BGR10:pbytes= 4; shmfmt = WL_SHM_FORMAT_ABGR2101010;	break;
	case PTI_L8:
	default:
		Sys_Error("WL_CreateCursor: converted to unsupported pixel format %i", format);
		return NULL;	//failure. can't convert from that format.
	}

	Image_Premultiply(&mips);

	fd = open (xdg_runtime_dir, __O_TMPFILE|O_RDWR|O_EXCL, 0600);
	size = mips.mip[0].width * mips.mip[0].height * pbytes;
	if (fd >= 0)
	{
		ftruncate (fd, size);
		outdata = mmap (NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (outdata)
		{
			memcpy(outdata, mips.mip[0].data, mips.mip[0].width*mips.mip[0].height*pbytes);
			pool = pwl_shm_create_pool (w.shm, fd, size);
			if (pool)
			{
				buffer = pwl_shm_pool_create_buffer (pool, 0, mips.mip[0].width, mips.mip[0].height, mips.mip[0].width*pbytes, shmfmt);
				if (buffer)
				{
					surf = pwl_compositor_create_surface(w.compositor);
					if (surf)
					{	//yay! something worked for once!
						pwl_surface_attach(surf, buffer, 0, 0);
						pwl_surface_damage(surf, 0, 0, mips.mip[0].width, mips.mip[0].height);
						pwl_surface_commit(surf);
					}
					//can't destroy the buffer too early
				}
				pwl_shm_pool_destroy (pool);	//nor that
			}
			munmap(outdata, size);	//should be serverside by now
		}
		close (fd);	//nor that...
	}
	if (surf)
	{
		c = Z_Malloc(sizeof(*c));
		c->surf = surf;
		c->buf = buffer;
		c->hot_x = hotx;
		c->hot_y = hoty;
		c->width = mips.mip[0].width;
		c->height = mips.mip[0].height;
		return c;
	}
	if (buffer)
		pwl_buffer_destroy(buffer);	//don't need to track that any more

	//free it conversions required it.
	if (mips.mip[0].needfree)
		Z_Free(mips.mip[0].data);
	return NULL;
}
qboolean WL_SetCursor(void *cursor)
{
	struct cursorinfo_s *c = cursor;
	w.cursor = c;

	WL_SetHWCursor();	//update it
	return true;
}
void	 WL_DestroyCursor(void *cursor)
{
	struct cursorinfo_s *c = cursor;

	if (c == w.cursor)
	{	//unset the cursor if its active
		w.cursor = NULL;
		WL_SetHWCursor();
	}
	pwl_surface_destroy(c->surf);
	pwl_buffer_destroy(c->buf);
}

static void WL_pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
	struct wdisplay_s *s = data;

	s->cursorfocus = true;
	s->cursorserial = serial;
	WL_SetHWCursor();
}
static void WL_pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
	struct wdisplay_s *s = data;
	s->cursorfocus = false;
	WL_SetHWCursor();
}

static void WL_pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	struct wdisplay_s *s = data;
	double x = wl_fixed_to_double(sx);
	double y = wl_fixed_to_double(sy);
	//wayland is shite shite shite.
	//1.4 still has no relative mouse motion.

	//track this for csd
	s->mousex = x;
	s->mousey = y;
	y -= s->csdsize;

	//and let the game know.
	if (!s->relative_mouse_active)
		IN_MouseMove(0, true, x, y, 0, 0);
}

static void WL_pointer_handle_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct wdisplay_s *s = data;
	int qkey;

	switch(button)
	{
	default:
		return;	//blurgh.
	case BTN_LEFT:
		qkey = K_MOUSE1;
		if ((!s->relative_mouse_active||!vid.activeapp) && s->mousey < s->csdsize)
		{
#ifdef XDG_SHELL_NAME
			if (s->xdg_toplevel)
				xdg_toplevel_move(s->xdg_toplevel, s->seat, serial);
#endif
#ifdef WL_SHELL_NAME
			if (s->ssurface)
				pwl_proxy_marshal((struct wl_proxy *) s->ssurface, WL_SHELL_SURFACE_MOVE, s->seat, serial);
#endif
			return;
		}
		break;
	case BTN_RIGHT:
		qkey = K_MOUSE2;
		if ((!s->relative_mouse_active||!vid.activeapp) && s->mousey < s->csdsize)
		{
#ifdef XDG_SHELL_NAME
			if (s->xdg_toplevel)
				xdg_toplevel_show_window_menu(s->xdg_toplevel, s->seat, serial, s->mousex, s->mousey);
#endif
#ifdef WL_SHELL_NAME
			if (s->ssurface)
				pwl_proxy_marshal((struct wl_proxy *) s->ssurface, WL_SHELL_SURFACE_SET_MAXIMIZED, NULL);
#endif
			return;
		}
		break;
	case BTN_MIDDLE:
		qkey = K_MOUSE3;
		break;
	case BTN_SIDE:
		qkey = K_MOUSE4;
		break;
	case BTN_EXTRA:
		qkey = K_MOUSE5;
		break;
	case BTN_FORWARD:
		qkey = K_MOUSE6;
		break;
	case BTN_BACK:
		qkey = K_MOUSE7;
		break;
	case BTN_TASK:
		qkey = K_MOUSE8;
		break;
	}
	IN_KeyEvent(0, !!state, qkey, 0);
//		pwl_shell_surface_move(display->window->shell_surface, display->seat, serial);
}

static void WL_pointer_handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	if (value < 0)
	{
		IN_KeyEvent(0, 1, K_MWHEELUP, 0);
		IN_KeyEvent(0, 0, K_MWHEELUP, 0);
	}
	else
	{
		IN_KeyEvent(0, 1, K_MWHEELDOWN, 0);
		IN_KeyEvent(0, 0, K_MWHEELDOWN, 0);
	}
}

static const struct wl_pointer_listener pointer_listener =
{
	WL_pointer_handle_enter,
	WL_pointer_handle_leave,
	WL_pointer_handle_motion,
	WL_pointer_handle_button,
	WL_pointer_handle_axis,
};

#ifdef WP_RELATIVE_POINTER_MANAGER_NAME
struct zwp_relative_pointer_v1;
#define ZWP_RELATIVE_POINTER_MANAGER_V1_DESTROY 0
#define ZWP_RELATIVE_POINTER_MANAGER_V1_GET_RELATIVE_POINTER 1
#define ZWP_RELATIVE_POINTER_V1_DESTROY 0

static void WL_pointer_handle_delta(void *data, struct zwp_relative_pointer_v1 *pointer, uint32_t time_hi, uint32_t time_lo, wl_fixed_t dx_w, wl_fixed_t dy_w, wl_fixed_t dx_raw_w, wl_fixed_t dy_raw_w)
{
	if (w.relative_mouse_active)
	{
		double xmove = wl_fixed_to_double(dx_raw_w);
		double ymove = wl_fixed_to_double(dy_raw_w);
		IN_MouseMove(0, false, xmove, ymove, 0, 0);
	}
}
struct zwp_relative_pointer_v1_listener
{
	void (*delta)(void *data, struct zwp_relative_pointer_v1 *pointer, uint32_t time_hi, uint32_t time_lo, wl_fixed_t dx_w, wl_fixed_t dy_w, wl_fixed_t dx_raw_w, wl_fixed_t dy_raw_w);
};
static const struct zwp_relative_pointer_v1_listener relative_pointer_listener =
{
	WL_pointer_handle_delta,
};

static void WL_BindRelativePointerManager(struct wl_registry *registry, uint32_t id)
{	/*oh hey, I wrote lots of code! pay me more! fuck that shit.*/

	static const struct wl_interface *types[8];
	static const struct wl_message zwp_relative_pointer_manager_v1_requests[] = {
		{ "destroy", "", types + 0 },
		{ "get_relative_pointer", "no", types + 6 },
	};
	static const struct wl_interface zwp_relative_pointer_manager_v1_interface = {
		"zwp_relative_pointer_manager_v1", 1,
		2, zwp_relative_pointer_manager_v1_requests,
		0, NULL,
	};
	static const struct wl_message zwp_relative_pointer_v1_requests[] = {
		{ "destroy", "", types + 0 },
	};
	static const struct wl_message zwp_relative_pointer_v1_events[] = {
		{ "relative_motion", "uuffff", types + 0 },
	};
	static const struct wl_interface zwp_relative_pointer_v1_interface = {
		"zwp_relative_pointer_v1", 1,
		1, zwp_relative_pointer_v1_requests,
		1, zwp_relative_pointer_v1_events,
	};

	//fix up types...
	types[6] = &zwp_relative_pointer_v1_interface;
	types[7] = pwl_pointer_interface;

	pzwp_relative_pointer_v1_interface = &zwp_relative_pointer_v1_interface;
	w.relative_pointer_manager = pwl_registry_bind(registry, id, &zwp_relative_pointer_manager_v1_interface, 1);
}
#endif

#ifdef WP_POINTER_CONSTRAINTS_NAME
#define ZWP_POINTER_CONSTRAINTS_V1_DESTROY 0
#define ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER 1
#define ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER 2
#define ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT 1
#define ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT 2
#define ZWP_LOCKED_POINTER_V1_DESTROY 0
#define ZWP_LOCKED_POINTER_V1_SET_CURSOR_POSITION_HINT 1
#define ZWP_LOCKED_POINTER_V1_SET_REGION 2
static void WL_pointer_handle_locked(void *data, struct zwp_locked_pointer_v1 *pointer)
{
	struct wdisplay_s *s = data;
	s->relative_mouse_active = true;	//yay, everything works!...
	WL_SetHWCursor();
}
static void WL_pointer_handle_unlocked(void *data, struct zwp_locked_pointer_v1 *pointer)
{	//this is a one-shot. it gets destroyed automatically, but we still need to null it
	struct wdisplay_s *s = data;
	s->relative_mouse_active = false;	//probably the compositor killed it.
	WL_SetHWCursor();

//	s->locked_pointer = NULL;
}
struct zwp_locked_pointer_v1_listener
{
	void (*locked)(void *data, struct zwp_locked_pointer_v1 *pointer);
	void (*unlocked)(void *data, struct zwp_locked_pointer_v1 *pointer);
};
static const struct zwp_locked_pointer_v1_listener locked_pointer_listener =
{
	WL_pointer_handle_locked,
	WL_pointer_handle_unlocked,
};
static void WL_BindPointerConstraintsManager(struct wl_registry *registry, uint32_t id)
{
	static const struct wl_interface *types[14];
	static const struct wl_message zwp_pointer_constraints_v1_requests[] = {
		{"destroy", "", types+0},
		{"lock_pointer", "noo?ou", types+2},
		{"confine_pointer", "noo?ou", types+7},
	};
	static const struct wl_interface zwp_pointer_constraints_v1_interface = {
		"zwp_pointer_constraints_v1", 1,
		3, zwp_pointer_constraints_v1_requests,
		0, NULL,
	};

	static const struct wl_message zwp_locked_pointer_v1_requests[] = {
		{"destroy", "", types + 0},
		{"set_cursor_position_hint", "ff", types + 0},
		{"set_region", "?o", types + 12},
	};

	static const struct wl_message zwp_locked_pointer_v1_events[] = {
		{"locked", "", types + 0},
		{"unlocked", "", types + 0},
	};

	static const struct wl_interface zwp_locked_pointer_v1_interface = {
		"zwp_locked_pointer_v1", 1,
		3, zwp_locked_pointer_v1_requests,
		2, zwp_locked_pointer_v1_events,
	};

	types[2] = &zwp_locked_pointer_v1_interface;
	types[3] = pwl_surface_interface;
	types[4] = pwl_pointer_interface;
	types[5] = pwl_region_interface;
//	types[7] = &zwp_confined_pointer_v1_interfae;
	types[8] = pwl_surface_interface;
	types[9] = pwl_pointer_interface;
	types[10] = pwl_region_interface;
	types[12] = pwl_region_interface;
	types[13] = pwl_region_interface;

	pzwp_locked_pointer_v1_interface = &zwp_locked_pointer_v1_interface;
	w.pointer_constraints = pwl_registry_bind(registry, id, &zwp_pointer_constraints_v1_interface, 1);
}
static inline int zwp_locked_pointer_v1_add_listener(struct zwp_locked_pointer_v1 *zwp_locked_pointer_v1, const struct zwp_locked_pointer_v1_listener *listener, void *data)		{return pwl_proxy_add_listener((struct wl_proxy *) zwp_locked_pointer_v1, (void (**)(void)) listener, data);}
static inline struct zwp_locked_pointer_v1 *zwp_pointer_constraints_v1_lock_pointer(struct zwp_pointer_constraints_v1 *zwp_pointer_constraints_v1, struct wl_surface *surface, struct wl_pointer *pointer, struct wl_region *region, uint32_t lifetime)		{return (struct zwp_locked_pointer_v1 *) pwl_proxy_marshal_constructor((struct wl_proxy *) zwp_pointer_constraints_v1, ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER, pzwp_locked_pointer_v1_interface, NULL, surface, pointer, region, lifetime);}
#endif

static void WL_keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
	struct wdisplay_s *d = data;
	if (d->xkb_context)
	{
		struct xkb_keymap *keymap;
		char *keymap_string = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
		if (keymap_string != MAP_FAILED)
		{
			keymap = pxkb_keymap_new_from_string (d->xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
			munmap (keymap_string, size);
			pxkb_state_unref (d->xkb_state);
			d->xkb_state = pxkb_state_new (keymap);
			pxkb_keymap_unref (keymap);
		}
	}

	close(fd);
}

static void WL_keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	vid.activeapp = true;
}

static void WL_keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
	vid.activeapp = false;
	IN_KeyEvent(0, false, -1, 0);	//aka: release all, so we're not left with alt held, etc.
}

static void WL_keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	extern int		shift_down;
	struct wdisplay_s *d = data;
	uint32_t qkey;
	uint32_t ukey;

	if (key < sizeof(waylandinputsucksbighairydonkeyballs)/sizeof(waylandinputsucksbighairydonkeyballs[0]))
		qkey = waylandinputsucksbighairydonkeyballs[key];
	else
		qkey = 0;
	if (!qkey)
		Con_DPrintf("WLScancode %#x has no mapping\n", key);

	if (d->xkb_context)
	{
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		{
			xkb_keysym_t keysym = pxkb_state_key_get_one_sym (d->xkb_state, key+8);
			ukey = pxkb_keysym_to_utf32 (keysym);
		}
		else
			ukey = 0;
	}
	else
	{
		//FIXME: this stuff is fucked, especially the ukey stuff.
		if (key < sizeof(waylandinputsucksbighairydonkeyballs)/sizeof(waylandinputsucksbighairydonkeyballs[0]))
		{
			if (shift_down)
				ukey = waylandinputsucksbighairydonkeyballsshift[key];
			else
				ukey = waylandinputsucksbighairydonkeyballs[key];
		}
		else
			ukey = 0;
		if (ukey < ' ' || ukey > 127)
			ukey = 0;
	}

	IN_KeyEvent(0, (state==WL_KEYBOARD_KEY_STATE_PRESSED)?1:0, qkey, ukey);
}

static void WL_keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct wdisplay_s *s = data;
	pxkb_state_update_mask (s->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

static const struct wl_keyboard_listener keyboard_listener =
{
	WL_keyboard_handle_keymap,
	WL_keyboard_handle_enter,
	WL_keyboard_handle_leave,
	WL_keyboard_handle_key,
	WL_keyboard_handle_modifiers
};
static void WL_seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
	struct wdisplay_s *s = data;
	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !s->pointer)
	{
		s->pointer = pwl_seat_get_pointer(seat);
		pwl_pointer_add_listener(s->pointer, &pointer_listener, s);

		if (w.relative_pointer_manager)
		{	//and try and get relative pointer events too. so much fucking boilerplate.
			w.relative_pointer = (struct zwp_relative_pointer_v1 *)pwl_proxy_marshal_constructor((struct wl_proxy *) w.relative_pointer_manager, ZWP_RELATIVE_POINTER_MANAGER_V1_GET_RELATIVE_POINTER, pzwp_relative_pointer_v1_interface, NULL, w.pointer);
			pwl_proxy_add_listener((struct wl_proxy *) w.relative_pointer, (void (**)(void)) &relative_pointer_listener, &w);
		}
	}
	else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && s->pointer)
	{
		pwl_pointer_destroy(s->pointer);
		s->pointer = NULL;

		if (w.relative_pointer)
		{
			pwl_proxy_marshal((struct wl_proxy *) w.relative_pointer, ZWP_RELATIVE_POINTER_V1_DESTROY);
			pwl_proxy_destroy((struct wl_proxy *) w.relative_pointer);
		}
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !s->keyboard)
	{
		s->keyboard = pwl_seat_get_keyboard(seat);
		pwl_keyboard_add_listener(s->keyboard, &keyboard_listener, s);
	}
	else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && s->keyboard)
	{
		pwl_keyboard_destroy(s->keyboard);
		s->keyboard = NULL;
	}
}
static const struct wl_seat_listener seat_listener =
{
	WL_seat_handle_capabilities
};


#ifdef XDG_DECORATION_MANAGER_NAME
static const struct wl_interface *pzxdg_toplevel_decoration_v1_interface;
static void WL_BindDecorationManager(struct wl_registry *registry, uint32_t id)
{	/*oh hey, I wrote lots of code! pay me more! fuck that shit.*/
	static const struct wl_interface *types[3];
	static const struct wl_message zxdg_decoration_manager_v1_requests[] = {
		{ "destroy", "", types + 0 },
		{ "get_toplevel_decoration", "no", types + 1 },
	};
	static const struct wl_interface zxdg_decoration_manager_v1_interface = {
		"zxdg_decoration_manager_v1", 1,
		2, zxdg_decoration_manager_v1_requests,
		0, NULL,
	};
	static const struct wl_message zxdg_toplevel_decoration_v1_requests[] = {
		{ "destroy", "", types + 0 },
		{ "set_mode", "u", types + 0 },
		{ "unset_mode", "", types + 0 },
	};
	static const struct wl_message zxdg_toplevel_decoration_v1_events[] = {
		{ "configure", "u", types + 0 },
	};
	static const struct wl_interface zxdg_toplevel_decoration_v1_interface = {
		"zxdg_toplevel_decoration_v1", 1,
		3, zxdg_toplevel_decoration_v1_requests,
		1, zxdg_toplevel_decoration_v1_events,
	};

	//fix up types...
	types[1] = &zxdg_toplevel_decoration_v1_interface;
	types[2] = pxdg_toplevel_interface;

	pzxdg_toplevel_decoration_v1_interface = &zxdg_toplevel_decoration_v1_interface;
	w.xdg_decoration_manager = pwl_registry_bind(registry, id, &zxdg_decoration_manager_v1_interface, 1);
}

enum xdg_decoration_manager_mode {
    ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE = 1,
    ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE = 2,
};
#define ZXDG_DECORATION_MANAGER_V1_GET_TOPLEVEL_DECORATION 1
static inline struct zxdg_toplevel_decoration_v1 *zxdg_decoration_manager_v1_get_toplevel_decoration(struct zxdg_decoration_manager_v1 *zxdg_decoration_manager_v1, struct xdg_toplevel *toplevel)		{ return (struct zxdg_toplevel_decoration_v1 *) pwl_proxy_marshal_constructor((struct wl_proxy *) zxdg_decoration_manager_v1, ZXDG_DECORATION_MANAGER_V1_GET_TOPLEVEL_DECORATION, pzxdg_toplevel_decoration_v1_interface, NULL, toplevel); }

#define ZXDG_TOPLEVEL_DECORATION_V1_SET_MODE 1
struct zxdg_toplevel_decoration_v1_listener {
    void (*configure)(void *data, struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration_v1, uint32_t mode);
};
static inline void zxdg_toplevel_decoration_v1_set_mode(struct zxdg_toplevel_decoration_v1 *xdg_decoration, uint32_t mode)		{ pwl_proxy_marshal((struct wl_proxy *) xdg_decoration, ZXDG_TOPLEVEL_DECORATION_V1_SET_MODE, mode); }
static inline int zxdg_toplevel_decoration_v1_add_listener(struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration_v1, const struct zxdg_toplevel_decoration_v1_listener *listener, void *data)	{ return pwl_proxy_add_listener((struct wl_proxy *) zxdg_toplevel_decoration_v1, (void (**)(void)) listener, data); }

static void xdg_decoration_mode(void *data, struct zxdg_toplevel_decoration_v1 *xdg_decoration, uint32_t mode)
{
	struct wdisplay_s *d = data;
	d->hasssd = (mode != ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
}
static const struct zxdg_toplevel_decoration_v1_listener myxdg_decoration_listener =
{
	xdg_decoration_mode
};

#endif

#ifdef ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME
static const struct wl_interface *porg_kde_kwin_server_decoration_interface;
static void WL_BindKWinDecorationManager(struct wl_registry *registry, uint32_t id)
{
	static const struct wl_interface *types[3];
	static const struct wl_message org_kde_kwin_server_decoration_manager_requests[] = {
		{ "create", "no", types + 1 },
	};
	static const struct wl_message org_kde_kwin_server_decoration_manager_events[] = {
		{ "default_mode", "u", types + 0 },
	};
	static const struct wl_interface org_kde_kwin_server_decoration_manager_interface = {
		"org_kde_kwin_server_decoration_manager", 1,
		1, org_kde_kwin_server_decoration_manager_requests,
		1, org_kde_kwin_server_decoration_manager_events,
	};
	static const struct wl_message org_kde_kwin_server_decoration_requests[] = {
		{ "release", "", types + 0 },
		{ "request_mode", "u", types + 0 },
	};
	static const struct wl_message org_kde_kwin_server_decoration_events[] = {
		{ "mode", "u", types + 0 },
	};
	static const struct wl_interface org_kde_kwin_server_decoration_interface = {
		"org_kde_kwin_server_decoration", 1,
		2, org_kde_kwin_server_decoration_requests,
		1, org_kde_kwin_server_decoration_events,
	};

	//fix up types...
	types[1] = &org_kde_kwin_server_decoration_interface;
	types[2] = pwl_surface_interface;

	porg_kde_kwin_server_decoration_interface = &org_kde_kwin_server_decoration_interface;
	w.kwin_decoration_manager = pwl_registry_bind(registry, id, &org_kde_kwin_server_decoration_manager_interface, 1);
}
enum org_kde_kwin_server_decoration_manager_mode {
    ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_NONE = 0,
    ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_CLIENT = 1,
    ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER = 2,
};
#define ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_CREATE 0
static inline struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration_manager_create(struct org_kde_kwin_server_decoration_manager *org_kde_kwin_server_decoration_manager, struct wl_surface *surface)		{ return (struct org_kde_kwin_server_decoration *) pwl_proxy_marshal_constructor((struct wl_proxy *) org_kde_kwin_server_decoration_manager, ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_CREATE, porg_kde_kwin_server_decoration_interface, NULL, surface); }
#define ORG_KDE_KWIN_SERVER_DECORATION_REQUEST_MODE 1
struct org_kde_kwin_server_decoration_listener {
    void (*mode)(void *data, struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration, uint32_t mode);
};
static inline void org_kde_kwin_server_decoration_request_mode(struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration, uint32_t mode)		{ pwl_proxy_marshal((struct wl_proxy *) org_kde_kwin_server_decoration, ORG_KDE_KWIN_SERVER_DECORATION_REQUEST_MODE, mode); }
static inline int org_kde_kwin_server_decoration_add_listener(struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration, const struct org_kde_kwin_server_decoration_listener *listener, void *data)		{ return pwl_proxy_add_listener((struct wl_proxy *) org_kde_kwin_server_decoration, (void (**)(void)) listener, data); }

static void kwin_decoration_mode(void *data, struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration, uint32_t mode)
{
	struct wdisplay_s *d = data;
	d->hasssd = (mode != ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_CLIENT);
}
static struct org_kde_kwin_server_decoration_listener myorg_kde_kwin_server_decoration_listener =
{
	kwin_decoration_mode
};
#endif

static void WL_handle_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct wdisplay_s *d = data;
Con_DLPrintf(0, "Wayland Interface %s id %u version %u\n", interface, id, version);
	if (strcmp(interface, "wl_compositor") == 0)
		d->compositor = pwl_registry_bind(registry, id, pwl_compositor_interface, 1);
	else if (strcmp(interface, "wl_shm") == 0)
		d->shm = pwl_registry_bind(registry, id, pwl_shm_interface, 1);
#ifdef WL_SHELL_NAME
	else if (strcmp(interface, WL_SHELL_NAME) == 0)
		d->shell = pwl_registry_bind(registry, id, pwl_shell_interface, 1);
#endif
#ifdef XDG_SHELL_NAME
	else if (strcmp(interface, XDG_SHELL_NAME) == 0 && version > 1)	//old versions of kwin are buggy, sidestep it.
		d->xdg_wm_base = pwl_registry_bind(registry, id, pxdg_wm_base_interface, 2);
#endif
	else if (strcmp(interface, "wl_seat") == 0 && !d->seat)
	{
		d->seat = pwl_registry_bind(registry, id, pwl_seat_interface, 1);
		pwl_seat_add_listener(d->seat, &seat_listener, d);
	}
#ifdef XDG_DECORATION_MANAGER_NAME
	else if (strcmp(interface, XDG_DECORATION_MANAGER_NAME) == 0)
		WL_BindDecorationManager(registry, id);
#endif
#ifdef ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME
	else if (strcmp(interface, ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME) == 0)
		WL_BindKWinDecorationManager(registry, id);
#endif
//	else if (strcmp(interface, "zxdg_decoration_manager_v1") == 0)
//		WL_BindDecorationManager(registry, id);
#ifdef WP_RELATIVE_POINTER_MANAGER_NAME
	else if (strcmp(interface, WP_RELATIVE_POINTER_MANAGER_NAME) == 0)
		WL_BindRelativePointerManager(registry, id);
#endif
#ifdef WP_POINTER_CONSTRAINTS_NAME
	else if (strcmp(interface, WP_POINTER_CONSTRAINTS_NAME) == 0)
		WL_BindPointerConstraintsManager(registry, id);
#endif
/*	else if (!strcmp(interface, "input_device"))
		display_add_input(id);
*/
}

static void WL_handle_global_remove(void *data, struct wl_registry *wl_registry, uint32_t id)
{
	Con_Printf("WL_handle_global_remove: %u\n", id);
}

static const struct wl_registry_listener WL_registry_listener = {
	WL_handle_global,
	WL_handle_global_remove
};

qboolean WL_MayRefresh(void)
{
	if (w.wait_for_configure || w.waylandisblocking)
	{
		if (pwl_display_dispatch(w.display) == -1)
		{
			Sys_Sleep(2);
			Con_Printf(CON_ERROR "wayland connection was lost. Restarting video\n");
			Cbuf_InsertText("vid_restart", RESTRICT_LOCAL, true);
		}
		return false;
	}
	return true;
}
static void frame_handle_done(void *data, struct wl_callback *callback, uint32_t time)
{
	pwl_proxy_destroy((struct wl_proxy *)callback);
	w.waylandisblocking = false;
}
static const struct wl_callback_listener frame_listener = {
	.done = frame_handle_done,
};

static void WL_SwapBuffers(void)
{
	qboolean wantabs;
	TRACE(("WL_SwapBuffers\n"));

	switch(qrenderer)
	{
#if defined(GLQUAKE) && defined(USE_EGL)
	case QR_OPENGL:
		if (w.csdsize && font_default)
		{
			unsigned int vw=vid.width;
			unsigned int vh=vid.height;
			unsigned int rw=vid.rotpixelwidth;
			unsigned int rh=vid.rotpixelheight;
			Matrix4x4_CM_Orthographic(r_refdef.m_projection_std, 0, vid.pixelwidth, w.csdsize, 0, -99999, 99999);
			Matrix4x4_Identity(r_refdef.m_view);
			qglViewport(0, vid.pixelheight, vid.pixelwidth, w.csdsize);
			GL_SetShaderState2D(true);	//responsible for loading the new projection matrix
			vid.rotpixelwidth = vid.width = vid.pixelwidth;
			vid.rotpixelheight = vid.height = w.csdsize;
			R2D_ImageColours(0.05, 0.05, 0.1, 1);
			R2D_FillBlock(0, 0, vid.pixelwidth, w.csdsize);
			R2D_ImageColours(1, 1, 1, 1);
			Draw_FunStringWidth(0, 4, w.csdcaption, vid.pixelwidth, 2, vid.activeapp);
			vid.width=vw;
			vid.height=vh;
			vid.rotpixelwidth=rw;
			vid.rotpixelheight=rh;
		}
		if (R2D_Flush)
			R2D_Flush();

		{
			// Register a frame callback to know when we need to draw the next frame
			struct wl_callback *callback = pwl_surface_frame(w.surface);
			w.waylandisblocking = true;
			pwl_callback_add_listener(callback, &frame_listener, NULL);
		}

		EGL_SwapBuffers();
		//wl_surface_damage(w.surface, 0, 0, vid.pixelwidth, vid.pixelheight);
		if (pwl_display_dispatch_pending(w.display) < 0)
		{
			Con_Printf(CON_ERROR "wayland connection was lost. Restarting video\n");
			Cbuf_InsertText("vid_restart", RESTRICT_LOCAL, true);
			return;
		}
		if (w.hasssd)
			w.csdsize = 0;	//kde's implementation-specific/legacy extension doesn't need us to draw crappy lame CSD junk
		else if (vid.activeapp && w.relative_mouse_active)
			w.csdsize = 0;	//kill the csd while using relative mouse coords.
		else
			w.csdsize = Font_CharPHeight(font_default)+8;

		if (vid.pixelheight != w.trueheight-w.csdsize)
		{
			vid.pixelheight = w.trueheight-w.csdsize;
			Cvar_ForceCallback(&vid_conautoscale);
		}
		break;
#endif
	case QR_VULKAN:
		if (pwl_display_dispatch_pending(w.display) < 0)
		{
			Con_Printf(CON_ERROR "wayland connection was lost. Restarting video\n");
			Cbuf_InsertText("vid_restart", RESTRICT_LOCAL, true);
			return;
		}
		break;
	default:
		break;
	}

	//if the game wants absolute mouse positions...
	wantabs = !vid.activeapp || (!vrui.enabled && Key_MouseShouldBeFree()) || !in_windowed_mouse.value;
	//and force it on if we're lacking one of the plethora of extensions that were needed to get the damn thing actually usable.
	wantabs |= !w.relative_pointer || !w.pointer_constraints;
	if (!wantabs && w.cursorfocus && !w.locked_pointer && w.pointer_constraints)
	{
		w.locked_pointer = zwp_pointer_constraints_v1_lock_pointer(w.pointer_constraints, w.surface, w.pointer, NULL, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
		zwp_locked_pointer_v1_add_listener(w.locked_pointer, &locked_pointer_listener, &w);
	}
	if (wantabs && w.locked_pointer)
	{
		pwl_proxy_marshal((struct wl_proxy *) w.locked_pointer, ZWP_LOCKED_POINTER_V1_DESTROY);
		pwl_proxy_destroy((struct wl_proxy *) w.locked_pointer);
		w.locked_pointer = NULL;
		if (!w.relative_mouse_active)
		{
			w.relative_mouse_active = false;
			WL_SetHWCursor();
		}
	}
}

#ifdef VKQUAKE
static qboolean WLVK_SetupSurface(void)
{
	VkWaylandSurfaceCreateInfoKHR inf = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
    inf.flags = 0;
    inf.display = w.display;
    inf.surface = w.surface;

    if (VK_SUCCESS == vkCreateWaylandSurfaceKHR(vk.instance, &inf, vkallocationcb, &vk.surface))
        return true;
    return false;
}
#endif

static qboolean WL_NameAndShame(void)
{
	//called after the renderer has been initialised, so these messages should be more prominant.
	if (!w.relative_pointer)
		Con_Printf(CON_WARNING "WARNING: Wayland server does not support %s, "CON_ERROR"mouse grabs are not supported\n", WP_RELATIVE_POINTER_MANAGER_NAME);
	else if (!w.pointer_constraints)
		Con_Printf(CON_WARNING "WARNING: Wayland server does not support %s, "CON_ERROR"mouse grabs are not supported\n", WP_POINTER_CONSTRAINTS_NAME);

	if (!w.hasssd)
		Con_Printf(CON_WARNING "WARNING: Wayland server does not support window decorations\n");
	return true;
}

static qboolean WL_Init (rendererstate_t *info, unsigned char *palette)
{
#ifdef DYNAMIC_WAYLAND
	if (!WL_InitLibrary())
	{
		Con_Printf("couldn't load wayland client libraries\n");
		return false;
	}
#endif
	WL_Setup_XDG_Shell();

	switch(qrenderer)
	{
#if defined(GLQUAKE) && defined(USE_EGL)
	case QR_OPENGL:
#ifdef DYNAMIC_WAYLAND
		lib_wayland_egl = Sys_LoadLibrary("libwayland-egl.so.1", waylandexports_egl);
		if (!lib_wayland_egl)
		{
			Con_Printf("couldn't load libwayland-egl.so.1 library\n");
			return false;
		}
#endif

//		setenv("EGL_PLATFORM", "wayland", 1);	//if this actually matters then we're kinda screwed
		if (!EGL_LoadLibrary(info->subrenderer))
		{
			Con_Printf("couldn't load EGL library\n");
			return false;
		}

		break;
#endif
#ifdef VKQUAKE
	case QR_VULKAN:
		#ifdef VK_NO_PROTOTYPES
		{	//vulkan requires that vkGetInstanceProcAddr is set in advance.
			dllfunction_t func[] =
			{
				{(void*)&vkGetInstanceProcAddr,	"vkGetInstanceProcAddr"},
				{NULL,							NULL}
			};

			if (!Sys_LoadLibrary("libvulkan.so.1", func))
			{
				if (!Sys_LoadLibrary("libvulkan.so", func))
				{
					Con_Printf("Couldn't intialise libvulkan.so\nvulkan loader is not installed\n");
					return false;
				}
			}
		}
		#endif
		break;
#endif
	default:
		return false;	//not supported dude...
	}

	memset(&w, 0, sizeof(w));

#ifdef DYNAMIC_WAYLAND
	if (WL_InitLibraryXKB())
#endif
		w.xkb_context = pxkb_context_new(XKB_CONTEXT_NO_FLAGS);

	w.csdsize = 0;
	w.display = pwl_display_connect(NULL);
	if (!w.display)
	{
		Con_Printf("couldn't connect to wayland server\n");
		return false;
	}
	w.registry = pwl_display_get_registry(w.display);
	pwl_registry_add_listener(w.registry, &WL_registry_listener, &w);
	pwl_display_dispatch(w.display);
	pwl_display_roundtrip(w.display);
	if (!w.compositor)
	{
		Con_Printf("no compositor running, apparently\n");
		return false;
	}

	w.surface = pwl_compositor_create_surface(w.compositor);
	if (!w.surface)
	{
		Con_Printf("no compositor running, apparently\n");
		return false;
	}

	if (0)
		;
#ifdef XDG_SHELL_NAME
	else if (w.xdg_wm_base)
	{
		xdg_wm_base_add_listener(w.xdg_wm_base, &myxdg_wm_base_listener, &w);
		w.xdg_surface = xdg_wm_base_get_xdg_surface(w.xdg_wm_base, w.surface);
		if (!w.xdg_surface)
			return false;
		xdg_surface_add_listener(w.xdg_surface, &myxdg_surface_listener, &w);
		w.xdg_toplevel = xdg_surface_get_toplevel(w.xdg_surface);
		if (!w.xdg_toplevel)
			return false;
		xdg_toplevel_add_listener(w.xdg_toplevel, &myxdg_toplevel_listener, &w);
		xdg_toplevel_set_min_size(w.xdg_toplevel, 320, 200);
	}
#endif
#ifdef WL_SHELL_NAME
	else if (w.shell)
	{
		w.ssurface = pwl_shell_get_shell_surface(w.shell, w.surface);
		if (w.ssurface)
			pwl_shell_surface_add_listener(w.ssurface, &shell_surface_listener, &w);
	}
#endif
	else
	{
		Con_Printf("no compositor shell running, apparently\n");
		return false;	//no way to make it fullscreen/top-level means it'll probably stay hidden, so make this fatal.
	}

	WL_SetCaption("FTE Quake");
#ifdef XDG_DECORATION_MANAGER_NAME
	if (w.xdg_decoration_manager)
	{	//decorate it!
		w.xdg_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(w.xdg_decoration_manager, w.xdg_toplevel);
		if (w.xdg_decoration)
		{
			zxdg_toplevel_decoration_v1_add_listener(w.xdg_decoration, &myxdg_decoration_listener, &w);
			zxdg_toplevel_decoration_v1_set_mode(w.xdg_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
	}
#endif
#if defined(XDG_DECORATION_MANAGER_NAME) && defined(ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME)
	else
#endif
#ifdef ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_NAME
	if (w.kwin_decoration_manager)
	{	//decorate it!
		w.kwin_decoration = org_kde_kwin_server_decoration_manager_create(w.kwin_decoration_manager, w.surface);
		if (w.kwin_decoration)
		{
			org_kde_kwin_server_decoration_add_listener(w.kwin_decoration, &myorg_kde_kwin_server_decoration_listener, &w);
			org_kde_kwin_server_decoration_request_mode(w.kwin_decoration, ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER);
		}
	}
#endif
	
#ifdef XDG_SHELL_NAME
	if (w.xdg_wm_base)
	{
		if (info->fullscreen)
			xdg_toplevel_set_fullscreen(w.xdg_toplevel, NULL);
	}
#endif
#ifdef WL_SHELL_NAME
	if (w.ssurface)
	{
		if (info->fullscreen)
			pwl_shell_surface_set_fullscreen(w.ssurface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 60, NULL);
		else
			pwl_shell_surface_set_toplevel(w.ssurface);
	}
#endif

	w.truewidth = info->width?info->width:640;
	w.trueheight = info->height?info->height:480;
pwl_display_roundtrip(w.display);

	{
		struct wl_region *region = pwl_compositor_create_region(w.compositor);
		pwl_region_add(region, 0, 0, w.truewidth, w.trueheight);
		pwl_surface_set_opaque_region(w.surface, region);
		//FIXME: leaks region...
		//pwl_region_destroy(region);
	}
pwl_display_roundtrip(w.display);
	vid.pixelwidth = w.truewidth;
	vid.pixelheight = w.trueheight-w.csdsize;

	vid.activeapp = true;

	//window_set_keyboard_focus_handler(window, WL_handler_keyfocus);
	//window_set_resize_handler(w.surface, WL_handler_resize);

	switch(qrenderer)
	{
	default:
		return false;
#ifdef VKQUAKE
	case QR_VULKAN:
		{
			const char *extnames[] = {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
			if (VK_Init(info, extnames, countof(extnames), WLVK_SetupSurface, NULL))
				return WL_NameAndShame();
			Con_Printf(CON_ERROR "Unable to initialise vulkan-on-wayland.\n");
			return false;
		}
		break;
#endif
#if defined(GLQUAKE) && defined(USE_EGL)
	case QR_OPENGL:
		{
			EGLConfig cfg;
			w.enwindow = pwl_egl_window_create(w.surface, w.truewidth, w.trueheight);
			if (!EGL_InitDisplay(info, EGL_PLATFORM_WAYLAND_KHR, w.display, (EGLNativeDisplayType)w.display, &cfg))
			{
				Con_Printf("couldn't find suitable EGL config\n");
				return false;
			}
			#ifdef XDG_SHELL_NAME
				if (w.xdg_toplevel)
				{
					w.wait_for_configure = true;
					pwl_surface_commit(w.surface);
				}
			#endif
			if (!EGL_InitWindow(info, EGL_PLATFORM_WAYLAND_KHR, w.enwindow, (EGLNativeWindowType)w.enwindow, cfg))
			{
				Con_Printf("couldn't initialise EGL context\n");
				return false;
			}
		}

		pwl_display_dispatch_pending(w.display);

		if (GL_Init(info, &EGL_Proc))
			return WL_NameAndShame();
		Con_Printf(CON_ERROR "Unable to initialise opengl-on-wayland.\n");
		return false;
#endif
	}
	return true;
}
static void WL_DeInit(void)
{
#if defined(GLQUAKE) && defined(USE_EGL)
	EGL_Shutdown();
	if (w.enwindow)
		pwl_egl_window_destroy(w.enwindow);
	EGL_UnloadLibrary();
	GL_ForgetPointers();
#endif
#ifdef WL_SHELL_NAME
	if (w.ssurface)		pwl_proxy_destroy((struct wl_proxy *) w.ssurface);
#endif
	if (w.surface)		pwl_proxy_marshal((struct wl_proxy *) w.surface, WL_SURFACE_DESTROY);
	if (w.surface)		pwl_proxy_destroy((struct wl_proxy *) w.surface);
	if (w.compositor)	pwl_proxy_destroy((struct wl_proxy *) w.compositor);
	if (w.registry)		pwl_proxy_destroy((struct wl_proxy *) w.registry);
	if (w.display)
	{	//let the server know everything before shutting it off
		pwl_display_dispatch(w.display);
		pwl_display_roundtrip(w.display);
		pwl_display_disconnect(w.display);
	}
	Z_Free(w.csdcaption);
	memset(&w, 0, sizeof(w));
}
static qboolean WL_ApplyGammaRamps(unsigned int gammarampsize, unsigned short *ramps)
{
	//not supported
	return false;
}
static void WL_SetCaption(const char *text)
{
#ifdef XDG_SHELL_NAME
	if (w.xdg_toplevel)
		xdg_toplevel_set_title(w.xdg_toplevel, text);
#endif
#ifdef WL_SHELL_NAME
	if (w.ssurface)
		pwl_shell_surface_set_title(w.ssurface, text);
#endif

	Z_StrDupPtr(&w.csdcaption, text);
}

static int WL_GetPriority(void)
{
	//2 = above x11, 0 = below x11.
	char *stype = getenv("XDG_SESSION_TYPE");
	char *dpyname;
	if (stype)
	{
		if (!strcmp(stype, "wayland"))
			return 2;
		if (!strcmp(stype, "x11"))
			return 0;
		if (!strcmp(stype, "tty"))	//FIXME: support this!
			return 0;
	}

	//otherwise if both WAYLAND_DISPLAY and DISPLAY are defined, then we assume that we were started from xwayland wrapper thing, and that the native/preferred windowing system is wayland.
	//(lets just hope our wayland support is comparable)

	dpyname = getenv("WAYLAND_DISPLAY");
	if (dpyname && *dpyname)
		return 2;	//something above X11.
	return 0;	//default.
}

#if defined(GLQUAKE) && defined(USE_EGL)
rendererinfo_t rendererinfo_wayland_gl =
{
	"OpenGL (Wayland)",
	{
		"wlgl",
		"wayland",
	},
	QR_OPENGL,

	GLDraw_Init,
	GLDraw_DeInit,

	GL_UpdateFiltering,
	GL_LoadTextureMips,
	GL_DestroyTexture,

	GLR_Init,
	GLR_DeInit,
	GLR_RenderView,

	WL_Init,
	WL_DeInit,
	WL_SwapBuffers,
	WL_ApplyGammaRamps,
	WL_CreateCursor,
	WL_SetCursor,
	WL_DestroyCursor,
	WL_SetCaption,       //setcaption
	GLVID_GetRGBInfo,


	GLSCR_UpdateScreen,

	GLBE_SelectMode,
	GLBE_DrawMesh_List,
	GLBE_DrawMesh_Single,
	GLBE_SubmitBatch,
	GLBE_GetTempBatch,
	GLBE_DrawWorld,
	GLBE_Init,
	GLBE_GenBrushModelVBO,
	GLBE_ClearVBO,
	GLBE_UpdateLightmaps,
	GLBE_SelectEntity,
	GLBE_SelectDLight,
	GLBE_Scissor,
	GLBE_LightCullModel,

	GLBE_VBO_Begin,
	GLBE_VBO_Data,
	GLBE_VBO_Finish,
	GLBE_VBO_Destroy,

	GLBE_RenderToTextureUpdate2d,

	"",
	WL_GetPriority,
	NULL,
	NULL,
	WL_MayRefresh
};
#endif

#ifdef VKQUAKE
static qboolean WLVK_EnumerateDevices(void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc))
{
	qboolean ret = false;
#ifdef VK_NO_PROTOTYPES
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	void *lib = NULL;
	dllfunction_t func[] =
	{
		{(void*)&vkGetInstanceProcAddr,		"vkGetInstanceProcAddr"},
		{NULL,							NULL}
	};

	if (!lib)
		lib = Sys_LoadLibrary("libvulkan.so.1", func);
	if (!lib)
		lib = Sys_LoadLibrary("libvulkan.so", func);
	if (!lib)
		return false;
#endif
	ret = VK_EnumerateDevices(usercontext, callback, "Vulkan-Wayland-", vkGetInstanceProcAddr);
#ifdef VK_NO_PROTOTYPES
	Sys_CloseLibrary(lib);
#endif
	return ret;
}
rendererinfo_t rendererinfo_wayland_vk =
{
    "Vulkan (Wayland)",
    {
        "wlvk",
        "vk",
        "vulkan",
        "wayland"
    },
    QR_VULKAN,

    VK_Draw_Init,
    VK_Draw_Shutdown,

    VK_UpdateFiltering,
    VK_LoadTextureMips,
    VK_DestroyTexture,

    VK_R_Init,
    VK_R_DeInit,
    VK_R_RenderView,

    WL_Init,
    WL_DeInit,
    WL_SwapBuffers,
    WL_ApplyGammaRamps,

    WL_CreateCursor,
    WL_SetCursor,
    WL_DestroyCursor,
    WL_SetCaption,       //setcaption
    VKVID_GetRGBInfo,


    VK_SCR_UpdateScreen,

    VKBE_SelectMode,
    VKBE_DrawMesh_List,
    VKBE_DrawMesh_Single,
    VKBE_SubmitBatch,
    VKBE_GetTempBatch,
    VKBE_DrawWorld,
    VKBE_Init,
    VKBE_GenBrushModelVBO,
    VKBE_ClearVBO,
    VKBE_UploadAllLightmaps,
    VKBE_SelectEntity,
    VKBE_SelectDLight,
    VKBE_Scissor,
    VKBE_LightCullModel,

    VKBE_VBO_Begin,
    VKBE_VBO_Data,
    VKBE_VBO_Finish,
    VKBE_VBO_Destroy,

    VKBE_RenderToTextureUpdate2d,

    "",
	WL_GetPriority,
	NULL,
	WLVK_EnumerateDevices
};
#endif

#endif

