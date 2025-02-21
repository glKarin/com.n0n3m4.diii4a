/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
Small note: anything concerning EGL in here is specific to egl-with-x11.
if you want egl-with-framebuffer, look elsewhere.
*/

/*
X11 is a huge pile of shit. I don't mean just the old x11 protocol, but all the _current_ standards that don't even try to fix the issues too.

Its fucking retarded the crap that you have to do to get something to work.
timeouts to ensure alt+tab between two apps doesn't fuck up gamma ramps is NOT a nice thing to have to do.
_MOUSE_ grabs cause alt+tab to fuck up
if I use xinput2 to get raw mouse events (so that I don't have to use some random hack to disable acceleration and risk failing to reset it on crashes), then the mouse still moves outside of our window, and trying to fire then looses focus...
xf86vm extension results in scrolling around a larger viewport. dependant upon the mouse position. even if we constrain the mouse to our window, it'll still scroll.
warping the pointer still triggers 'raw' mouse move events. in what world does that make any sense?!?
alt-tab task lists are a window in their own right. that's okay, but what's not okay is that they destroy that window without giving focus to the new window first, so the old one gets focus and that fucks everything up too. yay for timeouts.
to allow alt-tabbing with window managers that do not respect requests to not shove stuff on us, we have to hide ourselves completely and create a separate window that can still accept focus from the window manager. its fecking vile.
window managers reparent us too, in much the same way. which is a bad thing because we keep getting reparented and that makes a mess of focus events. its a nightmare.

the whole thing is bloody retarded.

none of these issues will be fixed by a compositing window manager, because there's still a window manager there.
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <dlfcn.h>

#include "quakedef.h"

#ifdef NO_X11
	#ifdef VKQUAKE
		rendererinfo_t vkrendererinfo;
	#endif
#else

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifdef VKQUAKE
#include "vk/vkrenderer.h"
#ifdef VK_USE_PLATFORM_XLIB_KHR
static qboolean XVK_SetupSurface_XLib(void);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
static qboolean XVK_SetupSurface_XCB(void);
#endif
#endif
#ifdef GLQUAKE
#include <GL/glx.h>
#ifdef USE_EGL
#include "gl_videgl.h"
#endif
#include "glquake.h"
#include "vr.h"
#endif

#define USE_VMODE
#ifndef NO_X11_RANDR
	#define USE_XRANDR
#endif
#ifndef NO_X11_XSS
	#define USE_XSS
#endif

#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <wchar.h>

static Display *vid_dpy = NULL;
static Cursor vid_nullcursor;	//'cursor' to use when none should be shown
static Cursor vid_newcursor;	//cursor that the game is trying to use
static Cursor vid_activecursor;	//cursor that is currently active
static Window vid_window;
static Window vid_decoywindow;	//for legacy mode, this is a boring window that we can reparent into as needed
static Window vid_root;
#ifdef GLQUAKE
static GLXContext ctx = NULL;
#endif
extern qboolean vid_isfullscreen;
static int scrnum;
static long vid_x_eventmask;
static enum
{
	PSL_NONE,
#ifdef GLQUAKE
#ifdef USE_EGL
	PSL_EGL,
#endif
	PSL_GLX,
#endif
#ifdef VKQUAKE
	PSL_VULKAN,
#endif
} currentpsl;

extern cvar_t vid_conautoscale, vid_vsync;

extern int sys_parentleft;
extern int sys_parenttop;
extern int sys_parentwidth;
extern int sys_parentheight;
extern long    sys_parentwindow;

static qboolean X11_CheckFeature(const char *featurename, qboolean defaultval)
{
	cvar_t *var;
	if (COM_CheckParm(va("-no%s", featurename)))
		return false;
	if (COM_CheckParm(va("-force%s", featurename)))
		return true;
	var = Cvar_Get(va("x11_allow_%s", featurename), defaultval?"1":"0", 0, NULL);
	if (var)
		return !!var->ival;
	return defaultval;
}
static qboolean X11_Clipboard_Notify(XSelectionEvent *xselection);

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask)

#define X_MASK (KEY_MASK | MOUSE_MASK | ResizeRequest | StructureNotifyMask | FocusChangeMask | VisibilityChangeMask)

struct _XrmHashBucketRec;

typedef int (*qXErrorHandler) (Display*, XErrorEvent*);

static struct
{
	void *lib;
	int	 (*pXChangeProperty)(Display *display, Window w, Atom property, Atom type, int format, int mode, unsigned char *data, int nelements);
	int	 (*pXCloseDisplay)(Display *display);
	int 	 (*pXConvertSelection)(Display *display, Atom selection, Atom target, Atom property, Window requestor, Time time);
	Colormap (*pXCreateColormap)(Display *display, Window w, Visual *visual, int alloc);
	GC	 (*pXCreateGC)(Display *display, Drawable d, unsigned long valuemask, XGCValues *values);
	Pixmap	 (*pXCreatePixmap)(Display *display, Drawable d, unsigned int width, unsigned int height, unsigned int depth);
	Cursor	 (*pXCreatePixmapCursor)(Display *display, Pixmap source, Pixmap mask, XColor *foreground_color, XColor *background_color, unsigned int x, unsigned int y);
	Window	 (*pXCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
	int	 (*pXDefineCursor)(Display *display, Window w, Cursor cursor);
	int	 (*pXDeleteProperty)(Display *display, Window w, Atom property);
	int	 (*pXDestroyWindow)(Display *display, Window w);
	int	 (*pXFillRectangle)(Display *display, Drawable d, GC gc, int x, int y, unsigned int width, unsigned int height);
	int 	 (*pXFlush)(Display *display);
	int 	 (*pXFree)(void *data);
	int	 (*pXFreeCursor)(Display *display, Cursor cursor);
	void	 (*pXFreeEventData)(Display *display, XGenericEventCookie *cookie);
	int	 (*pXFreeGC)(Display *display, GC gc);
	int	 (*pXFreePixmap)(Display *display, Pixmap pixmap);
	char	*(*pXGetAtomName)(Display *display, Atom atom);
	Bool	 (*pXGetEventData)(Display *display, XGenericEventCookie *cookie);
	Window 	 (*pXGetSelectionOwner)(Display *display, Atom selection);
	Status	 (*pXGetWindowAttributes)(Display *display, Window w, XWindowAttributes *window_attributes_return);
	int	 (*pXGetWindowProperty)(Display *display, Window w, Atom property, long long_offset, long long_length, Bool delete, Atom req_type, Atom *actual_type_return, int *actual_format_return, unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return);
	int	 (*pXGrabKeyboard)(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time);
	int	 (*pXGrabPointer)(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time);
	Status (*pXInitThreads)(void);
	Atom 	 (*pXInternAtom)(Display *display, char *atom_name, Bool only_if_exists);
	KeySym	 (*pXLookupKeysym)(XKeyEvent *key_event, int index);
	int	 (*pXLookupString)(XKeyEvent *event_struct, char *buffer_return, int bytes_buffer, KeySym *keysym_return, XComposeStatus *status_in_out);
	int	 (*pXMapWindow)(Display *display, Window w);
	int	 (*pXMoveResizeWindow)(Display *display, Window w, int x, int y, unsigned width, unsigned height);
	int	 (*pXMoveWindow)(Display *display, Window w, int x, int y);
	int	 (*pXNextEvent)(Display *display, XEvent *event_return);
	int	 (*pXPeekEvent)(Display *display, XEvent *event_return);
	Display *(*pXOpenDisplay)(char *display_name);
	int 	 (*pXPending)(Display *display);
	Bool 	 (*pXQueryExtension)(Display *display, const char *name, int *major_opcode_return, int *first_event_return, int *first_error_return);
	int 	 (*pXRaiseWindow)(Display *display, Window w);
	int	 (*pXReparentWindow)(Display *display, Window w, Window parent, int x, int y);
	int	 (*pXResizeWindow)(Display *display, Window w, unsigned width, unsigned height);
	int	 (*pXSelectInput)(Display *display, Window w, long event_mask);
	Status 	 (*pXSendEvent)(Display *display, Window w, Bool propagate, long event_mask, XEvent *event_send);
	int	 (*pXSetIconName)(Display *display, Window w, char *icon_name);
	int	 (*pXSetInputFocus)(Display *display, Window focus, int revert_to, Time time);
	int 	 (*pXSetSelectionOwner)(Display *display, Atom selection, Window owner, Time time);
	void	 (*pXSetWMNormalHints)(Display *display, Window w, XSizeHints *hints);
	Status	 (*pXSetWMProtocols)(Display *display, Window w, Atom *protocols, int count);
	int	 (*pXStoreName)(Display *display, Window w, char *window_name);
	int 	 (*pXSync)(Display *display, Bool discard);
	int	 (*pXUndefineCursor)(Display *display, Window w);
	int	 (*pXUngrabKeyboard)(Display *display, Time time);
	int	 (*pXUngrabPointer)(Display *display, Time time);
	int 	 (*pXWarpPointer)(Display *display, Window src_w, Window dest_w, int src_x, int src_y, unsigned int src_width, unsigned int src_height, int dest_x, int dest_y);
	Status (*pXMatchVisualInfo)(Display *display, int screen, int depth, int class, XVisualInfo *vinfo_return);
	XVisualInfo *(*pXGetVisualInfo)(Display *display, long vinfo_mask, XVisualInfo *vinfo_template, int *nitems_return);


	qXErrorHandler (*pXSetErrorHandler)(XErrorHandler);
	int (*pXGetErrorText)(Display *display, int code, char *buffer_return, int length);

	int (*pXGrabServer)(Display *display);
	int (*pXUngrabServer)(Display *display);

	char *(*pXKeysymToString)(KeySym);
	KeySym *(*pXGetKeyboardMapping)(Display *display, KeyCode first_keycode, int keycode_count, int *keysyms_per_keycode_return);

#define XI_RESOURCENAME "FTEQW"
#define XI_RESOURCECLASS "FTEQW"
	char *(*pXSetLocaleModifiers)(char *modifier_list);
	Bool (*pXSupportsLocale)(void); 
	XIM		(*pXOpenIM)(Display *display, struct _XrmHashBucketRec *db, char *res_name, char *res_class);
	char *	(*pXGetIMValues)(XIM im, ...); 
	XIC		(*pXCreateIC)(XIM im, ...);
	void	(*pXSetICFocus)(XIC ic); 
	void	(*pXUnsetICFocus)(XIC ic); 
	char *  (*pXGetICValues)(XIC ic, ...);
	char *  (*pXSetICValues)(XIC ic, ...);
	Bool	(*pXFilterEvent)(XEvent *event, Window w);
	int		(*pXutf8LookupString)(XIC ic, XKeyPressedEvent *event, char *buffer_return, int bytes_buffer, KeySym *keysym_return, Status *status_return);
	int		(*pXwcLookupString)(XIC ic, XKeyPressedEvent *event, wchar_t *buffer_return, int bytes_buffer, KeySym *keysym_return, Status *status_return);
	void	(*pXDestroyIC)(XIC ic);
	Status	(*pXCloseIM)(XIM im);
	qboolean	dounicode;
	int			ime_shown;
	XIC			unicodecontext;
	XIM			inputmethod;
	XPoint		ime_pos;

	struct
	{
		Window	source;	//the source window to send dndFinished to.
		Atom	type;	//the type of the data. usually text/uri-list.
		Atom	myprop;	//the property on our window that we're copying the data to.
	} dnd;
} x11;

static int X11_ErrorHandler(Display *dpy, XErrorEvent *e)
{
	char msg[80];
	*msg = 0;
	x11.pXGetErrorText(dpy, e->error_code, msg, sizeof(msg));
	Con_Printf(CON_ERROR "XLib Error %d (%s): request %d.%d\n", e->error_code, msg, e->request_code, e->minor_code);
	return 0;	//ignored.
}

static qboolean x11_initlib(void)
{
	static dllfunction_t x11_functable[] =
	{
		{(void**)&x11.pXChangeProperty,		"XChangeProperty"},
		{(void**)&x11.pXCloseDisplay,		"XCloseDisplay"},
		{(void**)&x11.pXConvertSelection,	"XConvertSelection"},
		{(void**)&x11.pXCreateColormap,		"XCreateColormap"},
		{(void**)&x11.pXCreateGC,		"XCreateGC"},
		{(void**)&x11.pXCreatePixmap,		"XCreatePixmap"},
		{(void**)&x11.pXCreatePixmapCursor,	"XCreatePixmapCursor"},
		{(void**)&x11.pXCreateWindow,		"XCreateWindow"},
		{(void**)&x11.pXDefineCursor,		"XDefineCursor"},
		{(void**)&x11.pXDeleteProperty,		"XDeleteProperty"},
		{(void**)&x11.pXDestroyWindow,		"XDestroyWindow"},
		{(void**)&x11.pXFillRectangle,		"XFillRectangle"},
		{(void**)&x11.pXFlush,			"XFlush"},
		{(void**)&x11.pXFree,			"XFree"},
		{(void**)&x11.pXFreeCursor,		"XFreeCursor"},
		{(void**)&x11.pXFreeGC,			"XFreeGC"},
		{(void**)&x11.pXFreePixmap,		"XFreePixmap"},
		{(void**)&x11.pXGetAtomName,		"XGetAtomName"},
		{(void**)&x11.pXGetSelectionOwner,	"XGetSelectionOwner"},
		{(void**)&x11.pXGetWindowAttributes,	"XGetWindowAttributes"},
		{(void**)&x11.pXGetWindowProperty,	"XGetWindowProperty"},
		{(void**)&x11.pXGrabKeyboard,		"XGrabKeyboard"},
		{(void**)&x11.pXGrabPointer,		"XGrabPointer"},
		{(void**)&x11.pXInitThreads,		"XInitThreads"},
		{(void**)&x11.pXInternAtom,		"XInternAtom"},
		{(void**)&x11.pXLookupKeysym,		"XLookupKeysym"},
		{(void**)&x11.pXLookupString,		"XLookupString"},
		{(void**)&x11.pXMapWindow,		"XMapWindow"},
		{(void**)&x11.pXMoveResizeWindow,	"XMoveResizeWindow"},
		{(void**)&x11.pXMoveWindow,		"XMoveWindow"},
		{(void**)&x11.pXNextEvent,		"XNextEvent"},
		{(void**)&x11.pXPeekEvent,		"XPeekEvent"},
		{(void**)&x11.pXOpenDisplay,		"XOpenDisplay"},
		{(void**)&x11.pXPending,		"XPending"},
		{(void**)&x11.pXQueryExtension,		"XQueryExtension"},
		{(void**)&x11.pXRaiseWindow,		"XRaiseWindow"},
		{(void**)&x11.pXReparentWindow,		"XReparentWindow"},
		{(void**)&x11.pXResizeWindow,		"XResizeWindow"},
		{(void**)&x11.pXSelectInput,		"XSelectInput"},
		{(void**)&x11.pXSendEvent,		"XSendEvent"},
		{(void**)&x11.pXSetIconName,		"XSetIconName"},
		{(void**)&x11.pXSetInputFocus,		"XSetInputFocus"},
		{(void**)&x11.pXSetSelectionOwner,	"XSetSelectionOwner"},
		{(void**)&x11.pXSetWMNormalHints,	"XSetWMNormalHints"},
		{(void**)&x11.pXSetWMProtocols,		"XSetWMProtocols"},
		{(void**)&x11.pXStoreName,			"XStoreName"},
		{(void**)&x11.pXSync,				"XSync"},
		{(void**)&x11.pXUndefineCursor,		"XUndefineCursor"},
		{(void**)&x11.pXUngrabKeyboard,		"XUngrabKeyboard"},
		{(void**)&x11.pXUngrabPointer,		"XUngrabPointer"},
		{(void**)&x11.pXWarpPointer,		"XWarpPointer"},
		{(void**)&x11.pXMatchVisualInfo,	"XMatchVisualInfo"},
		{(void**)&x11.pXGetVisualInfo,		"XGetVisualInfo"},

		{(void**)&x11.pXKeysymToString,		"XKeysymToString"},
		{(void**)&x11.pXGetKeyboardMapping,	"XGetKeyboardMapping"},

		{(void**)&x11.pXGrabServer,			"XGrabServer"},
		{(void**)&x11.pXUngrabServer,		"XUngrabServer"},

		{NULL, NULL}
	};

	if (!x11.lib)
	{
#ifdef __CYGWIN__
		x11.lib = Sys_LoadLibrary("cygX11-6.dll", x11_functable);
#else
		x11.lib = Sys_LoadLibrary("libX11.so.6", x11_functable);
#endif
		if (!x11.lib)
			x11.lib = Sys_LoadLibrary("libX11", x11_functable);

		//these ones are extensions, and the reason we're doing this.
		if (x11.lib)
		{
			x11.pXSetErrorHandler	= Sys_GetAddressForName(x11.lib, "XSetErrorHandler");
			x11.pXGetErrorText	= Sys_GetAddressForName(x11.lib, "XGetErrorText");

			if (x11.pXSetErrorHandler && x11.pXGetErrorText)
				x11.pXSetErrorHandler(X11_ErrorHandler);

			//raw input (yay mouse deltas)
			x11.pXGetEventData		= Sys_GetAddressForName(x11.lib, "XGetEventData");
			x11.pXFreeEventData		= Sys_GetAddressForName(x11.lib, "XFreeEventData");

			//internationalisation
			x11.pXSetLocaleModifiers = Sys_GetAddressForName(x11.lib, "XSetLocaleModifiers");
			x11.pXSupportsLocale	= Sys_GetAddressForName(x11.lib, "XSupportsLocale");
			x11.pXOpenIM			= Sys_GetAddressForName(x11.lib, "XOpenIM");
			x11.pXGetIMValues		= Sys_GetAddressForName(x11.lib, "XGetIMValues");
			x11.pXCreateIC			= Sys_GetAddressForName(x11.lib, "XCreateIC");
			x11.pXSetICFocus		= Sys_GetAddressForName(x11.lib, "XSetICFocus");
			x11.pXUnsetICFocus		= Sys_GetAddressForName(x11.lib, "XUnsetICFocus");
			x11.pXGetICValues		= Sys_GetAddressForName(x11.lib, "XGetICValues");
			x11.pXSetICValues		= Sys_GetAddressForName(x11.lib, "XSetICValues");
			x11.pXFilterEvent		= Sys_GetAddressForName(x11.lib, "XFilterEvent");
			x11.pXutf8LookupString	= Sys_GetAddressForName(x11.lib, "Xutf8LookupString");
			x11.pXwcLookupString	= Sys_GetAddressForName(x11.lib, "XwcLookupString");
			x11.pXDestroyIC			= Sys_GetAddressForName(x11.lib, "XDestroyIC");
			x11.pXCloseIM			= Sys_GetAddressForName(x11.lib, "XCloseIM");
		}
		else
		{
			Con_Printf("Unable to load libX11\n");
		}
	}
	

	return !!x11.lib;
}

#ifdef USE_XSS
#include <X11/extensions/scrnsaver.h>
static struct {
	void *lib;
	int event, error;
	int major, minor;

	Bool (*pXScreenSaverQueryExtension)(Display *dpy, int *event_base_return, int *error_base_return);
	Status (*pXScreenSaverQueryVersion)(Display *dpy, int *major_version_return, int *minor_version_return);

	void (*pXScreenSaverSuspend)(Display *dpy, Bool suspend);

	qboolean cansuspend;
	qboolean suspending;
} x11xss;
static qboolean X11Xss_Init(void)
{
	static dllfunction_t x11xss_functable[] =
	{
		{(void**)&x11xss.pXScreenSaverQueryExtension,	"XScreenSaverQueryExtension"},
		{(void**)&x11xss.pXScreenSaverQueryVersion,		"XScreenSaverQueryVersion"},
		{(void**)&x11xss.pXScreenSaverSuspend,			"XScreenSaverSuspend"},
	};
	if (!x11xss.lib)
		x11xss.lib = Sys_LoadLibrary("libXss.so.1", x11xss_functable);
	x11xss.cansuspend = false;
	x11xss.suspending = false;	//must be recalled for each new dpy...
	if (x11xss.lib)
	{
		if (x11xss.pXScreenSaverQueryExtension(vid_dpy, &x11xss.event, &x11xss.error))
			if (x11xss.pXScreenSaverQueryVersion(vid_dpy, &x11xss.major, &x11xss.minor))
				if (x11xss.major>1 || (x11xss.major==1&&x11xss.minor>=1))
					x11xss.cansuspend = true;
	}
	return x11xss.cansuspend;
}
static void X11Xss_SuspendSaver(qboolean suspend)
{
	if (!x11xss.cansuspend)
		return; //no can do, sorry
	if (x11xss.suspending != suspend)
	{
		x11xss.suspending = suspend;
		x11xss.pXScreenSaverSuspend(vid_dpy, suspend);	//this is refcounted, but we're asserting.
	}
}
#else
static qboolean X11Xss_Init(void)
{
	return false;
}
static void X11Xss_SuspendSaver(qboolean suspend)
{
}
#endif


#ifdef VK_USE_PLATFORM_XCB_KHR
static struct
{
	void *lib;
	xcb_connection_t *(*pXGetXCBConnection)(Display *dpy);
} x11xcb;
static qboolean x11xcb_initlib(void)
{
	static dllfunction_t x11xcb_functable[] =
	{
		{(void**)&x11xcb.pXGetXCBConnection,		"XGetXCBConnection"},
		{NULL, NULL}
	};

	if (!x11xcb.lib)
	{
		x11xcb.lib = Sys_LoadLibrary("libX11-xcb.so.1", x11xcb_functable);
		if (!x11xcb.lib)
			x11xcb.lib = Sys_LoadLibrary("libX11-xcb", x11xcb_functable);

		if (!x11xcb.lib)
			Con_Printf("Unable to load libX11-xcb\n");
	}
	
	return !!x11xcb.lib;
}
#endif


#define FULLSCREEN_DESKTOP	(1u<<0)	//we didn't need to change video modes at all.
#define FULLSCREEN_VMODE	(1u<<1)	//using xf86 vidmode (we can actually change modes)
#define FULLSCREEN_VMODEACTIVE	(1u<<2)	//xf86 vidmode currently forced
#define FULLSCREEN_XRANDR	(1u<<3)	//using xf86 vidmode (we can actually change modes)
#define FULLSCREEN_XRANDRACTIVE	(1u<<4)	//xf86 vidmode currently forced
#define FULLSCREEN_LEGACY	(1u<<5)	//override redirect used (window is hidden from other programs, with a dummy window for alt+tab detection)
#define FULLSCREEN_WM		(1u<<6)	//fullscreen hint used (desktop environment is expected to 'do the right thing', but it won't change actual video modes)
#define FULLSCREEN_ACTIVE	(1u<<7)	//currently considered fullscreen
#define FULLSCREEN_ANYMODE	(FULLSCREEN_DESKTOP|FULLSCREEN_VMODE|FULLSCREEN_XRANDR)
static unsigned int fullscreenflags;
static int fullscreenx;
static int fullscreeny;
static int fullscreenwidth;
static int fullscreenheight;

void X_GoFullscreen(void);
void X_GoWindowed(void);
/*when alt-tabbing or whatever, the window manager creates a window, then destroys it again, resulting in weird focus events that trigger mode switches and grabs. using a timer reduces the weirdness and allows alt-tab to work properly. or at least better than it was working. that's the theory anyway*/
static unsigned int modeswitchtime;
static int modeswitchpending;

#ifdef USE_VMODE
typedef struct
{
	unsigned int        dotclock;
	unsigned short      hdisplay;
	unsigned short      hsyncstart;
	unsigned short      hsyncend;
	unsigned short      htotal;
	unsigned short      hskew;
	unsigned short      vdisplay;
	unsigned short      vsyncstart;
	unsigned short      vsyncend;
	unsigned short      vtotal;
	unsigned int        flags;
} XF86VidModeModeInfo;	//we don't touch this struct

static struct
{
	int opcode, event, error;
	int vmajor, vminor;
	void *lib;
	Bool (*pXF86VidModeQueryVersion)(Display *dpy, int *majorVersion, int *minorVersion);
	Bool (*pXF86VidModeGetGammaRampSize)(Display *dpy, int screen, int *size);
	Bool (*pXF86VidModeGetGammaRamp)(Display *dpy, int screen, int size, unsigned short *red, unsigned short *green, unsigned short *blue);
	Bool (*pXF86VidModeSetGammaRamp)(Display *dpy, int screen, int size, unsigned short *red, unsigned short *green, unsigned short *blue);
	Bool (*pXF86VidModeSetViewPort)(Display *dpy, int screen, int x, int y);
	Bool (*pXF86VidModeSwitchToMode)(Display *dpy, int screen, XF86VidModeModeInfo *modeline);
	Bool (*pXF86VidModeGetAllModeLines)(Display *dpy, int screen, int *modecount, XF86VidModeModeInfo ***modelinesPtr);

	XF86VidModeModeInfo **modes;
	int num_modes;
	int usemode;
	unsigned short originalramps[3][2048];
	qboolean originalapplied;	//states that the origionalramps arrays are valid, and contain stuff that we should revert to on close
	int originalrampsize;
} vm;
static qboolean VMODE_Init(void)
{
	static dllfunction_t vm_functable[] =
	{
		{(void**)&vm.pXF86VidModeQueryVersion, "XF86VidModeQueryVersion"},
		{(void**)&vm.pXF86VidModeGetGammaRampSize, "XF86VidModeGetGammaRampSize"},
		{(void**)&vm.pXF86VidModeGetGammaRamp, "XF86VidModeGetGammaRamp"},
		{(void**)&vm.pXF86VidModeSetGammaRamp, "XF86VidModeSetGammaRamp"},
		{(void**)&vm.pXF86VidModeSetViewPort, "XF86VidModeSetViewPort"},
		{(void**)&vm.pXF86VidModeSwitchToMode, "XF86VidModeSwitchToMode"},
		{(void**)&vm.pXF86VidModeGetAllModeLines, "XF86VidModeGetAllModeLines"},
		{NULL, NULL}
	};
	vm.vmajor = 0;
	vm.vminor = 0;
	vm.usemode = -1;
	vm.originalapplied = false;

	if (!X11_CheckFeature("vmode", true))
		return false;

	if (!x11.pXQueryExtension(vid_dpy, "XFree86-VidModeExtension", &vm.opcode, &vm.event, &vm.error))
	{
		Con_Printf("VidModeExtension extension not available.\n");
		return false;
	}
	
	if (!vm.lib)
		vm.lib = Sys_LoadLibrary("libXxf86vm", vm_functable);

	if (vm.lib)
	{
		if (vm.pXF86VidModeQueryVersion(vid_dpy, &vm.vmajor, &vm.vminor))
			Con_DPrintf("Using XF86-VidModeExtension Ver. %d.%d\n", vm.vmajor, vm.vminor);
		else
		{
			Con_Printf("No XF86-VidModeExtension support\n");
			vm.vmajor = 0;
			vm.vminor = 0;
		}
	}

	return vm.vmajor;
}

static void VMODE_SelectMode(int *width, int *height, float rate)
{
	if (COM_CheckParm("-current"))
		return;
	if (vm.vmajor)
	{
		int best_fit, best_dist, dist, x, y, z, r, i;

		vm.pXF86VidModeGetAllModeLines(vid_dpy, scrnum, &vm.num_modes, &vm.modes);
		best_dist = 9999999;
		best_fit = -1;
	
		if ((!*width || *width == DisplayWidth(vid_dpy, scrnum)) && (!*height || *height == DisplayHeight(vid_dpy, scrnum)) && !rate)
		{
			Con_DPrintf("XF86VM: mode change not needed\n");
			fullscreenflags |= FULLSCREEN_DESKTOP;
			return;
		}

		for (i = 0; i < vm.num_modes; i++)
		{
			//fixme: check this formula. should be the full refresh rate
			r = vm.modes[i]->dotclock * 1000 / (vm.modes[i]->htotal * vm.modes[i]->vtotal);
			if (*width > vm.modes[i]->hdisplay ||
				*height > vm.modes[i]->vdisplay ||
				rate > r)
				continue;

			x = *width - vm.modes[i]->hdisplay;
			y = *height - vm.modes[i]->vdisplay;
			z = rate?(rate - r):0;
			dist = (x * x) + (y * y) + (z * z);
			if (dist < best_dist)
			{
				best_dist = dist;
				best_fit = i;
			}
		}

		if (best_fit != -1)
		{
			// change to the mode
			if (vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[vm.usemode=best_fit]))
			{
				*width = vm.modes[best_fit]->hdisplay;
				*height = vm.modes[best_fit]->vdisplay;
				// Move the viewport to top left
				vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
				x11.pXSync(vid_dpy, False);

				fullscreenflags |= FULLSCREEN_VMODE | FULLSCREEN_VMODEACTIVE;
				Con_Printf("XF86VM: changed mode\n");
			}
			else
				Con_Printf("Failed to apply mode %i*%i\n", vm.modes[best_fit]->hdisplay, vm.modes[best_fit]->vdisplay);
		}
	}
}
#endif

#ifdef USE_XRANDR
#if 1
	#include <X11/extensions/Xrandr.h>
#else
	//stuff to avoid dependancies
	typedef struct _XRRScreenConfiguration XRRScreenConfiguration;
	typedef unsigned short Rotation;
	typedef unsigned short SizeID;
	typedef unsigned short SubpixelOrder;
	typedef unsigned short Connection;
	#define RR_Connected 0
	typedef XID RROutput;
	typedef XID RRCrtc;
	typedef XID RRMode;
	typedef unsigned long XRRModeFlags;

	typedef struct {
		int width, height;
		int mwidth, mheight;
	} XRRScreenSize;
	typedef struct _XRROutputInfo {
		Time            timestamp;
		RRCrtc          crtc;
		char            *name;
		int             nameLen;
		unsigned long   mm_width;
		unsigned long   mm_height;
		Connection      connection;
		SubpixelOrder   subpixel_order;
		int             ncrtc;
		RRCrtc          *crtcs;
		int             nclone;
		RROutput        *clones;
		int             nmode;
		int             npreferred;
		RRMode  	        *modes;
	} XRROutputInfo;
	typedef struct _XRRModeInfo {
		RRMode              id;
		unsigned int        width;
		unsigned int        height;
		unsigned long       dotClock;
		unsigned int        hSyncStart;
		unsigned int        hSyncEnd;
		unsigned int        hTotal;
		unsigned int        hSkew;
		unsigned int        vSyncStart;
		unsigned int        vSyncEnd;
		unsigned int        vTotal;
		char                *name;
		unsigned int        nameLength;
		XRRModeFlags        modeFlags;
	} XRRModeInfo;
	typedef struct _XRRScreenResources {
    		Time        timestamp;
		Time        configTimestamp;
		int         ncrtc;
		RRCrtc      *crtcs;
		int         noutput;
		RROutput    *outputs;
		int         nmode;
		XRRModeInfo *modes;
	} XRRScreenResources;
	typedef struct _XRRCrtcInfo {
		Time            timestamp;
		int             x, y;
		unsigned int    width, height;
		RRMode          mode;
		Rotation        rotation;
		int             noutput;
		RROutput        *outputs;
		Rotation        rotations;
		int             npossible;
		RROutput        *possible;
	} XRRCrtcInfo;
	typedef struct _XRRCrtcGamma {
		int             size;
		unsigned short  *red;
		unsigned short  *green;
		unsigned short  *blue;
	} XRRCrtcGamma;

	typedef struct _XRRPanning {
		Time            timestamp;
		unsigned int left;
		unsigned int top;
		unsigned int width;
		unsigned int height;
		unsigned int track_left;
		unsigned int track_top;
		unsigned int track_width;
		unsigned int track_height;
		int          border_left;
		int          border_top;
		int          border_right;
		int          border_bottom;
	} XRRPanning;
#endif
static struct
{
	//caps
	qboolean canmodechange11;
	qboolean canmodechange12;
	qboolean cangamma;
//	qboolean cangetmonitor;

	//general stuff
	void *lib;
	int opcode, event, error;
	int vmajor, vminor;
	Bool (*pQueryExtension)				(Display *dpy, int *event_base_return, int *error_base_return);
	Status (*pQueryVersion)				(Display *dpy, int *major_version_return, int *minor_version_return);

	//for v1.1
	//this is only aware of a single screen, so that's a bit poo really.
	//if we go fullscreen, we have no real way to restore multi-display things after
	XRRScreenConfiguration	*screenconfig;
	Time origtime;
	int origmode;
	Rotation origrot;
	int origrate;
	
	int targmode;
	int targrate;

	//stuff to query video modes
	XRRScreenConfiguration *(*pGetScreenInfo)	(Display *dpy, Window window);
	XRRScreenSize *(*pConfigSizes)			(XRRScreenConfiguration *config, int *nsizes);
	short *(*pConfigRates)				(XRRScreenConfiguration *config, int sizeID, int *nrates);
	SizeID (*pConfigCurrentConfiguration)		(XRRScreenConfiguration *config, Rotation *rotation);
	short (*pConfigCurrentRate)			(XRRScreenConfiguration *config);

	//stuff to change modes
	Time (*pConfigTimes)				(XRRScreenConfiguration *config, Time *config_timestamp);
	short (*pSetScreenConfigAndRate)		(Display *dpy, XRRScreenConfiguration *config, Drawable draw, int size_index, Rotation rotation, short rate, Time timestamp);

	//for v1.2
	//we gain gamma and multiple outputs+crts+etc
	XRRScreenResources	*res;		//all the resources on the system (including modes etc)
	XRROutputInfo 		**outputs;	//list of info for all outputs
	XRROutputInfo		*output;	//the output device that we're using
	RRCrtc				crtc;		//the output device's screen that we're focusing on (modes and gamma)
	XRRCrtcInfo			*crtcinfo;	//the info to restore
	XRRModeInfo 		*crtcmode;	//the mode we want to use
	XRRCrtcGamma		*origgamma;

//	Status				(*pGetScreenSizeRange)	(Display *dpy, Window window, int *minwidth, int *minheight, int *maxwidth, int *maxheight);
	void				(*pSetScreenSize)		(Display *dpy, Window window, int width, int height, int mmwidth, int mmheight);
	XRRScreenResources	*(*pGetScreenResources)	(Display *dpy, Window window);
	void				*(*pFreeScreenResources)(XRRScreenResources *);
	XRROutputInfo 		*(*pGetOutputInfo)		(Display *dpy, XRRScreenResources *resources, RROutput output);
	void				*(*pFreeOutputInfo)		(XRROutputInfo *outputinfo);
	XRRCrtcInfo 		*(*pGetCrtcInfo)		(Display *dpy, XRRScreenResources *resources, RRCrtc crtc);
	void				*(*pFreeCrtcInfo)		(XRRCrtcInfo *crtcinfo);
	Status				(*pSetCrtcConfig)		(Display *dpy, XRRScreenResources *resources, RRCrtc crtc, Time timestamp, int x, int y, RRMode mode, Rotation rotation, RROutput *output, int noutputs);
	XRRCrtcGamma *		(*pGetCrtcGamma)		(Display *dpy, RRCrtc crtc);
	void				(*pFreeGamma)			(XRRCrtcGamma *gamma);
	void				(*pSetCrtcGamma)		(Display *dpy, RRCrtc crtc, XRRCrtcGamma *gamma);

	//v1.3 has non-0 primary monitors.
	RROutput			(*pGetOutputPrimary)	(Display *dpy, Window window);
	Status				(*pSetPanning)			(Display *dpy, XRRScreenResources *resources, RRCrtc crtc, XRRPanning *panning);	//we need this just in case.
	XRRPanning *		(*pGetPanning)			(Display *dpy, XRRScreenResources *resources, RRCrtc crtc);
	void				(*pFreePanning)			(XRRPanning *panning);
	int pan[4];			//for restoring panning. pan region the screen may move to
	int pantrack[4];	//screen region where the mouse may be for the mouse to be tracked in
	int panborder[4];	//border region of crtc for panning to take place in. typically >=0...

	int nvidiabug;	//nvidia completely ignores panning requests, which fucks over fullscreen gameplay. we have to work around it by shrinking the screen which doesn't work with multiple displays (and risks fucking over everything else)
	int origscreenwidth,  origscreenwidthmm;
	int origscreenheight, origscreenheightmm;
} xrandr;
static qboolean XRandR_Init(void)
{
	static dllfunction_t xrandr_functable[] =
	{
		{(void**)&xrandr.pQueryExtension,				"XRRQueryExtension"},
		{(void**)&xrandr.pQueryVersion,					"XRRQueryVersion"},

		//1.0
		{(void**)&xrandr.pGetScreenInfo,				"XRRGetScreenInfo"},
		{(void**)&xrandr.pConfigTimes,					"XRRConfigTimes"},
		{(void**)&xrandr.pConfigSizes,					"XRRConfigSizes"},
		{(void**)&xrandr.pConfigRates,					"XRRConfigRates"},
		{(void**)&xrandr.pConfigCurrentConfiguration,	"XRRConfigCurrentConfiguration"},
		{(void**)&xrandr.pConfigCurrentRate,			"XRRConfigCurrentRate"},
		//1.1
		{(void**)&xrandr.pSetScreenConfigAndRate,		"XRRSetScreenConfigAndRate"},

		{NULL, NULL}
	};
	xrandr.vmajor = 0;
	xrandr.vminor = 0;
	xrandr.canmodechange11 = false;
	xrandr.canmodechange12 = false;
//	xrandr.cangamma = false;
	xrandr.crtcinfo = NULL;
	xrandr.res = NULL;
	xrandr.outputs = NULL;

	//enable by default once this is properly tested, and supports hwgamma.
	if (!X11_CheckFeature("xrandr", true))
		return false;

	if (!xrandr.lib)
		xrandr.lib = Sys_LoadLibrary("libXrandr", xrandr_functable);

	if (xrandr.lib)
	{
		if (xrandr.pQueryExtension(vid_dpy, &xrandr.event, &xrandr.error))
		{
			xrandr.pQueryVersion(vid_dpy, &xrandr.vmajor, &xrandr.vminor);
			if (xrandr.vmajor > 1 || (xrandr.vmajor == 1 && xrandr.vminor >= 1))
				xrandr.canmodechange11 = true;
			if (xrandr.vmajor > 1 || (xrandr.vmajor == 1 && xrandr.vminor >= 2))
			{	//1.2 functions
				xrandr.pGetScreenResources	= Sys_GetAddressForName(xrandr.lib, "XRRGetScreenResources");
				xrandr.pFreeScreenResources	= Sys_GetAddressForName(xrandr.lib, "XRRFreeScreenResources");
				xrandr.pGetOutputInfo		= Sys_GetAddressForName(xrandr.lib, "XRRGetOutputInfo");
				xrandr.pFreeOutputInfo		= Sys_GetAddressForName(xrandr.lib, "XRRFreeOutputInfo");
				xrandr.pGetCrtcInfo		= Sys_GetAddressForName(xrandr.lib, "XRRGetCrtcInfo");
				xrandr.pFreeCrtcInfo		= Sys_GetAddressForName(xrandr.lib, "XRRFreeCrtcInfo");
				xrandr.pSetCrtcConfig		= Sys_GetAddressForName(xrandr.lib, "XRRSetCrtcConfig");
				xrandr.pGetCrtcGamma		= Sys_GetAddressForName(xrandr.lib, "XRRGetCrtcGamma");
				xrandr.pFreeGamma		= Sys_GetAddressForName(xrandr.lib, "XRRFreeGamma");
				xrandr.pSetCrtcGamma		= Sys_GetAddressForName(xrandr.lib, "XRRSetCrtcGamma");

				xrandr.pSetScreenSize		= Sys_GetAddressForName(xrandr.lib, "XRRSetScreenSize");

				if (	xrandr.pGetScreenResources && xrandr.pFreeScreenResources && xrandr.pFreeOutputInfo
				    &&	xrandr.pGetCrtcInfo && xrandr.pFreeCrtcInfo && xrandr.pSetCrtcConfig
				    &&	xrandr.pGetCrtcGamma && xrandr.pFreeGamma && xrandr.pSetCrtcGamma
				    	)
					xrandr.canmodechange12 = true;
			}
			if (xrandr.vmajor > 1 || (xrandr.vmajor == 1 && xrandr.vminor >= 3))
			{
				xrandr.pGetOutputPrimary		= Sys_GetAddressForName(xrandr.lib, "XRRGetOutputPrimary");
				xrandr.pSetPanning				= Sys_GetAddressForName(xrandr.lib, "XRRSetPanning");
				xrandr.pGetPanning				= Sys_GetAddressForName(xrandr.lib, "XRRGetPanning");
				xrandr.pFreePanning				= Sys_GetAddressForName(xrandr.lib, "XRRFreePanning");
			}

			xrandr.nvidiabug = false; //hopeful...
			{
				int op, firstev, firsterr;
				if (x11.pXQueryExtension(vid_dpy, "NV-CONTROL", &op, &firstev, &firsterr))
					xrandr.nvidiabug = true; //our dreams are so cruely shattered.
			}

			//FIXME: query monitor sizes and calculate dpi for vid.dpy_[x|y]
			return true;
		}
	}
	else
		Con_Printf("XRandR library not available.\n");

	return false;
}

static float XRandR_CalcRate(XRRModeInfo *mode)
{
	if (!mode->hTotal || !mode->vTotal)
		return 0;
	return mode->dotClock / ((float)mode->hTotal*mode->vTotal);
}
static XRRModeInfo *XRandR_FindMode(RRMode mode)
{	//just looks up the right mode info.
	int i;
	for (i = 0; i < xrandr.res->nmode; i++)
	{
		if (xrandr.res->modes[i].id == mode)
			return &xrandr.res->modes[i];
	}
	return NULL;
}
static XRRModeInfo *XRandR_FindBestMode(int width, int height, int rate)
{
	int best_dist, dist, x, y, z, r, i;
	XRRModeInfo *mode, *best_mode;

	best_dist = 9999999;
	best_mode = NULL;
	for (i = 0; i < xrandr.output->nmode; i++)
	{
		mode = XRandR_FindMode(xrandr.output->modes[i]);
		if (!mode)
			continue;
		r = XRandR_CalcRate(mode);
		if (width > mode->width ||
			height > mode->height ||
			rate > r)
			continue;

		//FIXME: do rates differently - match width+height then come back for rate
		x = width - mode->width;
		y = height - mode->height;
		z = rate - r;
		dist = (x * x) + (y * y) + (z * z);
		if (dist < best_dist)
		{
			best_dist = dist;
			best_mode = mode;
		}
	}
	return best_mode;
}

static void XRandR_RevertMode(void)
{
	Time config_timestamp;
	if (fullscreenflags & FULLSCREEN_XRANDRACTIVE)
	{
		XRRCrtcInfo *c = xrandr.crtcinfo;
		if (c)
		{
			x11.pXGrabServer(vid_dpy);
			if (xrandr.nvidiabug == 1)
			{	//attempt to undo at least part of the damage we inflicted to work around nvidia's defects.
				if (Success == xrandr.pSetCrtcConfig(vid_dpy, xrandr.res, xrandr.crtc, CurrentTime, c->x, c->y, None, c->rotation, NULL, 0))
					xrandr.pSetScreenSize(vid_dpy, DefaultRootWindow(vid_dpy), xrandr.origscreenwidth, xrandr.origscreenheight, xrandr.origscreenwidthmm, xrandr.origscreenheightmm);
			}

			if (Success == xrandr.pSetCrtcConfig(vid_dpy, xrandr.res, xrandr.crtc, CurrentTime, c->x, c->y, c->mode, c->rotation, c->outputs, c->noutput))
			{
				if (xrandr.pSetPanning)
				{	//and try to reset panning back to its original values.
					XRRPanning panning;
					panning.timestamp		= c->timestamp;
					panning.left			= xrandr.pan[0];
					panning.top				= xrandr.pan[1];
					panning.width			= xrandr.pan[2];
					panning.height			= xrandr.pan[3];
					panning.track_left		= xrandr.pantrack[0];
					panning.track_top		= xrandr.pantrack[1];
					panning.track_width		= xrandr.pantrack[2];
					panning.track_height	= xrandr.pantrack[3];
					panning.border_left		= xrandr.panborder[0];
					panning.border_top		= xrandr.panborder[1];
					panning.border_right	= xrandr.panborder[2];
					panning.border_bottom	= xrandr.panborder[3];
					if (Success != xrandr.pSetPanning(vid_dpy, xrandr.res, xrandr.crtc, &panning))
						Con_Printf("Revert panning configuration failed\n");
					else
						Con_DPrintf("Panning configuration succeeded\n");
				}
				Con_DPrintf("Reverted mode\n");
			}
			else
				Con_Printf("Couldn't revert XRandR mode!\n");
			x11.pXUngrabServer(vid_dpy);
		}
		else
		{
			if (!xrandr.pSetScreenConfigAndRate(vid_dpy, xrandr.screenconfig, DefaultRootWindow(vid_dpy), xrandr.origmode, xrandr.origrot, xrandr.origrate, xrandr.origtime))
				xrandr.origtime = xrandr.pConfigTimes(xrandr.screenconfig, &config_timestamp);
		}
		fullscreenflags &= ~FULLSCREEN_XRANDRACTIVE;
	}
}
static qboolean XRandR_ApplyMode(void)
{
	qboolean ret = false;
	Time config_timestamp;
	if (!(fullscreenflags & FULLSCREEN_XRANDRACTIVE))
	{
		XRRCrtcInfo *c = xrandr.crtcinfo;
		if (c)
		{
			if (xrandr.crtcmode)
			{
				x11.pXGrabServer(vid_dpy);
				if (xrandr.nvidiabug == 1)
				{
					/*	nvidia's drivers are a bit shite and behave differently from every other driver
						unlike other drivers they force panning enabled (and refuse to disable it), which then bugs out in SDL and Wine too.
						(specifically, the panning width+height values are recalculated (completely ignoring any passed args), but left+top are not, the tracking+border areas are preserved but affect nothing)
						the workaround is to:
							lock the server (so other programs don't bug out too much),
							disable the screen (for the next step to work),
							change the virtual size to one that won't cause a problem with panning,
							re-enable the screen with the desired video mode,
							and unlock the server again.
						This will not help with multimonitor setups, xrandr video mode switches are disabled entirely there.
					*/
					int screen_width = c->x + xrandr.crtcmode->width;
					int screen_height = c->y + xrandr.crtcmode->height;
					if (Success == xrandr.pSetCrtcConfig(vid_dpy, xrandr.res, xrandr.crtc, CurrentTime, c->x, c->y, None, c->rotation, NULL, 0))
						xrandr.pSetScreenSize(vid_dpy, DefaultRootWindow(vid_dpy), screen_width, screen_height, (screen_width*xrandr.origscreenwidthmm)/xrandr.origscreenwidth, (screen_height*xrandr.origscreenheightmm)/xrandr.origscreenheight);
				}
				if (Success == xrandr.pSetCrtcConfig(vid_dpy, xrandr.res, xrandr.crtc, CurrentTime, c->x, c->y, xrandr.crtcmode->id, c->rotation, c->outputs, c->noutput))
				{
					if (xrandr.pSetPanning)
					{	//disable panning, in case panning was previously enabled via exterior means
						XRRPanning panning;
						panning.timestamp = c->timestamp;
						panning.left = c->x;
						panning.top = c->y;
						panning.width = panning.height = 0; //disables panning - "but RRSetScreenSize will silently enable panning if the screen size is increased. This does not happen if set to 0."

						//set the tracking area inside of the border area, to make doubly sure
						panning.track_left = c->x + 1;
						panning.track_top = c->y + 1;
						panning.track_width = xrandr.crtcmode->width-2;
						panning.track_height = xrandr.crtcmode->height-2;
						panning.border_left = panning.border_top = panning.border_right = panning.border_bottom = -16384; //border area is the region of this screen in which panning might be triggered.
						if (Success != xrandr.pSetPanning(vid_dpy, xrandr.res, xrandr.crtc, &panning))
							Con_Printf("Panning configuration failed\n");
						else
							Con_DPrintf("Panning configuration succeeded\n");
					}

					Con_DPrintf("Applied mode\n");
					ret = true;
				}
				else
					Con_Printf("Couldn't apply mode\n");

				x11.pXUngrabServer(vid_dpy);
			}
		}
		else
		{
			if (!xrandr.pSetScreenConfigAndRate(vid_dpy, xrandr.screenconfig, DefaultRootWindow(vid_dpy), xrandr.targmode, xrandr.origrot, xrandr.targrate, xrandr.origtime))
				xrandr.origtime = xrandr.pConfigTimes(xrandr.screenconfig, &config_timestamp);
		}
		fullscreenflags |= FULLSCREEN_XRANDRACTIVE;
	}
	return ret;
}

static void XRandR_Shutdown(void)
{
	int i;
	if (xrandr.origgamma)
	{
		xrandr.pSetCrtcGamma(vid_dpy, xrandr.crtc, xrandr.origgamma);
		xrandr.pFreeGamma(xrandr.origgamma);
		xrandr.origgamma = NULL;
	}
	if (vid_dpy)
		XRandR_RevertMode();
	if (xrandr.outputs)
	{
		for (i = 0; i < xrandr.res->noutput; i++)
			xrandr.pFreeOutputInfo(xrandr.outputs[i]);
		Z_Free(xrandr.outputs);
		xrandr.outputs = NULL;
	}
	if (xrandr.crtcinfo)
	{
		xrandr.pFreeCrtcInfo(xrandr.crtcinfo);
		xrandr.crtcinfo = NULL;
	}
	if (xrandr.res)
	{
		xrandr.pFreeScreenResources(xrandr.res);
		xrandr.res = NULL;
	}
}
static qboolean XRandR_FindOutput(const char *name)
{
	RROutput p;
	int i;
	if (!xrandr.canmodechange12)
		return false;
	if (!xrandr.res)
		xrandr.res = xrandr.pGetScreenResources(vid_dpy, DefaultRootWindow(vid_dpy));
	if (!xrandr.res)
		return false;
	if (!xrandr.outputs)
	{
		xrandr.outputs = Z_Malloc(sizeof(*xrandr.outputs) * xrandr.res->noutput);
		for (i = 0; i < xrandr.res->noutput; i++)
			xrandr.outputs[i] = xrandr.pGetOutputInfo(vid_dpy, xrandr.res, xrandr.res->outputs[i]);
	}
	if (xrandr.pGetOutputPrimary)
		p = xrandr.pGetOutputPrimary(vid_dpy, DefaultRootWindow(vid_dpy));
	else if (xrandr.res->noutput)
		p = xrandr.res->outputs[0];
	else
		p = 0;
	xrandr.output = NULL;
	xrandr.crtc = None;
	if (xrandr.origgamma)
	{
		xrandr.pSetCrtcGamma(vid_dpy, xrandr.crtc, xrandr.origgamma);
		xrandr.pFreeGamma(xrandr.origgamma);
		xrandr.origgamma = NULL;
	}
	if (xrandr.crtcinfo)
		xrandr.pFreeCrtcInfo(xrandr.crtcinfo);
	xrandr.crtcinfo = NULL;
	for (i = 0; i < xrandr.res->noutput; i++)
	{
		if (xrandr.outputs[i]->connection != RR_Connected || !xrandr.outputs[i]->ncrtc)
			continue;	//not usable...
		if (!xrandr.output && xrandr.res->outputs[i] == p)
			xrandr.output = xrandr.outputs[i];
		if (*name && !strncmp(xrandr.outputs[i]->name, name, xrandr.outputs[i]->nameLen))
		{	//this is the one they asked for
			xrandr.output = xrandr.outputs[i];
			break;
		}
	}
	if (xrandr.output)
	{
		xrandr.crtc = xrandr.output->crtc;
		xrandr.crtcinfo = xrandr.pGetCrtcInfo(vid_dpy, xrandr.res, xrandr.crtc);
		if (xrandr.crtcinfo)
		{
			xrandr.origgamma = xrandr.pGetCrtcGamma(vid_dpy, xrandr.crtc);
			return true;
		}
	}
	return false;
}

//called if we're not using randr to change video modes.
//(sets up crtc data 
static qboolean XRandr_PickScreen(const char *devicename, int *x, int *y, int *width, int *height)
{
	if (xrandr.crtcinfo || XRandR_FindOutput(devicename))
	{
		XRRCrtcInfo *c = xrandr.crtcinfo;
		*x = c->x;
		*y = c->y;
		*width = c->width;
		*height = c->height;
		Con_Printf("Found monitor %s %ix%i %i,%i\n", xrandr.output->name, c->width, c->height, c->x, c->y);
		return true;
	}
	return false;
}

//called when first changing video mode
static void XRandR_SelectMode(const char *devicename, int *x, int *y, int *width, int *height, int rate)
{
	if (COM_CheckParm("-current"))
		return;

	if (xrandr.crtcinfo)
	{
		XRRCrtcInfo *c;
		xrandr.crtcmode = XRandR_FindBestMode(*width, *height, rate);
		c = xrandr.crtcinfo;
		if (!*width || !*height || (xrandr.crtcmode&&c->mode == xrandr.crtcmode->id))
		{
			fullscreenflags |= FULLSCREEN_DESKTOP;
			Con_Printf("XRRSetCrtcConfig not needed\n");
		}
		else if (!xrandr.crtcmode)
			Con_Printf("XRRSetCrtcConfig failed\n");
		else
		{
			xrandr.origscreenwidth = DisplayWidth(vid_dpy, scrnum);
			xrandr.origscreenheight = DisplayHeight(vid_dpy, scrnum);
			xrandr.origscreenwidthmm = DisplayWidthMM(vid_dpy, scrnum);
			xrandr.origscreenheightmm = DisplayHeightMM(vid_dpy, scrnum);

			if (xrandr.pGetPanning)
			{
				XRRPanning *panning = xrandr.pGetPanning(vid_dpy, xrandr.res, xrandr.crtc);
				if (panning)
				{
					xrandr.pan[0] = panning->left?panning->left:c->x;	//apparently some drivers can be buggy and forget left+top when panning was previously disabled, snappnig them to the wrong place when re-enabled.
					xrandr.pan[1] = panning->top?panning->top:c->y;
					xrandr.pan[2] = panning->width;
					xrandr.pan[3] = panning->height;
					xrandr.pantrack[0] = panning->track_left;
					xrandr.pantrack[1] = panning->track_top;
					xrandr.pantrack[2] = panning->track_width;
					xrandr.pantrack[3] = panning->track_height;
					xrandr.panborder[0] = panning->border_left;
					xrandr.panborder[1] = panning->border_top;
					xrandr.panborder[2] = panning->border_right;
					xrandr.panborder[3] = panning->border_bottom;
					xrandr.pFreePanning(panning);
				}
			}

			if (xrandr.nvidiabug && (c->x != 0 || c->y != 0 || c->noutput>1))
			{
				Con_Printf("Nvidia and multimonitor detected. XRandR cannot be used safely in this situation.\n");
				xrandr.crtcmode = NULL;
			}
			else
			{
				if (xrandr.nvidiabug)
					Con_Printf(CON_ERROR "Attempting NVIDIA panning workaround. Try 'xrandr --output foo --auto' to fix if this breaks things.\n");

				fullscreenflags |= FULLSCREEN_XRANDR;
				if (XRandR_ApplyMode())
				{	//worked
					*x = c->x;
					*y = c->y;
					*width = xrandr.crtcmode->width;
					*height = xrandr.crtcmode->height;
				}
				else
					fullscreenflags &= ~FULLSCREEN_XRANDR;
			}
		}
	}
	else if (xrandr.canmodechange11)
	{
		int best_fit, best_dist, best_rate, dist, x, y;
		int i, nummodes;
		XRRScreenSize *modes;
		Time config_timestamp;
		Drawable draw = DefaultRootWindow(vid_dpy);

		xrandr.screenconfig = xrandr.pGetScreenInfo(vid_dpy, DefaultRootWindow(vid_dpy));
		xrandr.origtime = xrandr.pConfigTimes(xrandr.screenconfig, &config_timestamp);
		xrandr.origmode = xrandr.pConfigCurrentConfiguration(xrandr.screenconfig, &xrandr.origrot);
		xrandr.origrate = xrandr.pConfigCurrentRate(xrandr.screenconfig);
		modes = xrandr.pConfigSizes(xrandr.screenconfig, &nummodes);

		best_dist = 9999999;
		best_fit = -1;
		best_rate = -1;

		for (i = 0; i < nummodes; i++)
		{
			//fixme: check this formula. should be the full refresh rate
			if (*width > modes[i].width || *height > modes[i].height)
				continue;

			x = *width - modes[i].width;
			y = *height - modes[i].height;
			dist = (x * x) + (y * y);
			if (dist < best_dist)
			{
				best_dist = dist;
				best_fit = i;
			}
		}

		if (best_fit != -1)
		{
			//okay, there's a usable mode, and now we need to figure out what rate to use...
			//pick the higest rate under the target rate
			short *rates = xrandr.pConfigRates(xrandr.screenconfig, best_fit, &nummodes);
			for (i = 0; i < nummodes; i++)
			{
				if (rate>0 && rates[i] > rate)
					continue;
				if (best_rate < rates[i])
					best_rate = rates[i];
			}

			//change to the mode
			Con_DPrintf("Setting XRandR Mode %i: %i*%i@%i\n", best_fit, modes[best_fit].width, modes[best_fit].height, best_rate);
			if (!xrandr.pSetScreenConfigAndRate(vid_dpy, xrandr.screenconfig, draw, best_fit, xrandr.origrot, best_rate, xrandr.origtime))
			{
				xrandr.targmode = best_fit;
				xrandr.targrate = best_rate;
				*width = modes[best_fit].width;
				*height = modes[best_fit].height;
				x11.pXSync(vid_dpy, False);

				fullscreenflags |= FULLSCREEN_XRANDR | FULLSCREEN_XRANDRACTIVE;
			}
			else
				Con_Printf("Failed to apply mode %i*%i@%i\n", modes[best_fit].width, modes[best_fit].height, best_rate);
		}
	}
}
#endif



static float mouse_grabbed = 0;

static enum
{
	XIM_ORIG,
	XIM_DGA,
	XIM_XI2,
} x11_input_method;
int x11_mouseqdev = 0;

#define XF86DGADirectMouse		0x0004
static struct
{
	int opcode, event, error;
	void *lib;
	Status (*pXF86DGADirectVideo) (Display *dpy, int screen, int enable);
} dgam;
static qboolean DGAM_Init(void)
{
	static dllfunction_t dgam_functable[] =
	{
		{(void**)&dgam.pXF86DGADirectVideo, "XF86DGADirectVideo"},
		{NULL, NULL}
	};

	if (!x11.pXQueryExtension(vid_dpy, "XFree86-DGA", &dgam.opcode, &dgam.event, &dgam.error))
	{
		Con_Printf("DGA extension not available.\n");
		return false;
	}
	
	if (!dgam.lib)
		dgam.lib = Sys_LoadLibrary("libXxf86dga", dgam_functable);
	return !!dgam.lib;
}

#if 0
#include <X11/extensions/XInput2.h>
#else
#define XISetMask(ptr, event)   (((unsigned char*)(ptr))[(event)>>3] |=  (1 << ((event) & 7)))
#define XIMaskIsSet(ptr, event) (((unsigned char*)(ptr))[(event)>>3] &   (1 << ((event) & 7)))
#define XIMaskLen(event)        (((event + 7) >> 3))
typedef struct {
	int				mask_len;
	unsigned char	*mask;
	double			*values;
} XIValuatorState;
typedef struct
{
	int					deviceid;
	int					mask_len;
	unsigned char*		mask;
} XIEventMask;
#define XIMasterPointer		1
#define XIMasterKeyboard	2
#define XISlavePointer		3
#define XISlaveKeyboard		4
#define XIAllDevices		0
#define XIAllMasterDevices	1
#define XI_RawButtonPress	15
#define XI_RawButtonRelease	16
#define XI_RawMotion		17
#define XI_LASTEVENT XI_RawMotion
typedef struct {
	int				type;			/* GenericEvent */
	unsigned long	serial;			/* # of last request processed by server */
	Bool			send_event;		/* true if this came from a SendEvent request */
	Display			*display;		/* Display the event was read from */
	int				extension;		/* XI extension offset */
	int				evtype;			/* XI_RawKeyPress, XI_RawKeyRelease, etc. */
	Time			time;
	int				deviceid;
	int				sourceid;		/* Bug: Always 0. https://bugs.freedesktop.org//show_bug.cgi?id=34240 */
	int				detail;
	int				flags;
	XIValuatorState	valuators;
	double			*raw_values;
} XIRawEvent;
typedef struct
{
	int         type;
	int         sourceid;
} XIAnyClassInfo;
typedef struct
{
	int                 deviceid;
	char                *name;
	int                 use;
	int                 attachment;
	Bool                enabled;
	int                 num_classes;
	XIAnyClassInfo      **classes;
} XIDeviceInfo;
#define XIValuatorClass		2
typedef struct
{
	int         type;
	int         sourceid;
	int         number;
	Atom        label;
	double      min;
	double      max;
	double      value;
	int         resolution;
	int         mode;
} XIValuatorClassInfo;
#define XIModeAbsolute		1
#endif
static struct
{
	int opcode, event, error;
	int vmajor, vminor;
	void *libxi;

	int devicegroup;
	size_t ndeviceinfos;
	struct xidevinfo
	{
		int qdev;
		struct
		{
			qboolean abs;
			double min, max;
			float old;
		} axis[2]; //the meaning of any other axis is unknown. beware that they DO happen.
		qboolean abs;
	} *deviceinfo;
	int nextqdev;

	Status (*pXIQueryVersion)( Display *display, int *major_version_inout, int *minor_version_inout);
	int (*pXISelectEvents)(Display *dpy, Window win, XIEventMask *masks, int num_masks);
	XIDeviceInfo *(*pXIQueryDevice)(Display *dpy, int deviceid, int *ndevices_return);
	void (*pXIFreeDeviceInfo)(XIDeviceInfo *info);
} xi2;
static struct xidevinfo *XI2_GetDeviceInfo(int devid)
{
	if (devid >= xi2.ndeviceinfos)
	{
		struct xidevinfo *n = Z_Malloc((devid+1) * sizeof(*xi2.deviceinfo));
		memcpy(n, xi2.deviceinfo, xi2.ndeviceinfos*sizeof(*xi2.deviceinfo));
		Z_Free(xi2.deviceinfo);
		xi2.deviceinfo = n;
		while (xi2.ndeviceinfos <= devid)
		{
			xi2.deviceinfo[xi2.ndeviceinfos].qdev = DEVID_UNSET;
			if (devid >= 2)
			{
				int devs;
				XIDeviceInfo *dev = xi2.pXIQueryDevice(vid_dpy, xi2.ndeviceinfos, &devs);
				if (dev)
				{
					if (devs==1)
					{
						int j;
						for (j = 0; j < dev->num_classes; j++)
						{
							if (dev->classes[j]->sourceid == xi2.ndeviceinfos && dev->classes[j]->type == XIValuatorClass)
							{
								XIValuatorClassInfo *v = (XIValuatorClassInfo*)dev->classes[j];
								if (v->mode == XIModeAbsolute && v->number >= 0 && v->number < countof(xi2.deviceinfo[xi2.ndeviceinfos].axis))
								{
									xi2.deviceinfo[xi2.ndeviceinfos].abs = xi2.deviceinfo[xi2.ndeviceinfos].axis[v->number].abs = true;
									xi2.deviceinfo[xi2.ndeviceinfos].axis[v->number].min = v->min;
									xi2.deviceinfo[xi2.ndeviceinfos].axis[v->number].max = v->max;
								}
							}
						}
					}
					xi2.pXIFreeDeviceInfo(dev);
				}
			}
			
			xi2.ndeviceinfos++;
		}
	}
		
	return &xi2.deviceinfo[devid];
}
static qboolean XI2_Init(void)
{
	static dllfunction_t xi2_functable[] =
	{
		{(void**)&xi2.pXIQueryVersion, "XIQueryVersion"},
		{(void**)&xi2.pXISelectEvents, "XISelectEvents"},
		{(void**)&xi2.pXIQueryDevice, "XIQueryDevice"},
		{(void**)&xi2.pXIFreeDeviceInfo, "XIFreeDeviceInfo"},
		{NULL, NULL}
	};
	XIEventMask evm;
	unsigned char maskbuf[XIMaskLen(XI_LASTEVENT)];

	if (!x11.pXQueryExtension(vid_dpy, "XInputExtension", &xi2.opcode, &xi2.event, &xi2.error))
	{
		Con_Printf("XInput extension not available.\n");
		return false;
	}

	if (!xi2.libxi)
	{
#ifdef __CYGWIN__
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("cygXi-6.dll", xi2_functable);
#endif
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("libXi.so.6", xi2_functable);
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("libXi", xi2_functable);
		if (!xi2.libxi)
			Con_Printf("XInput library not available or too old.\n");
	}
	if (xi2.libxi)
	{
//		xi2.nextqdev = 0;	//start with 0, for player0
//		xi2.nqdevices = 0;
		xi2.vmajor = 2;
		xi2.vminor = 0;
//		xi2.devicegroup = XIAllMasterDevices;
		xi2.devicegroup = XIAllDevices;
		if (xi2.pXIQueryVersion(vid_dpy, &xi2.vmajor, &xi2.vminor))
		{
			Con_Printf("XInput library or server is too old\n");
			return false;
		}
		evm.deviceid = xi2.devicegroup;
		evm.mask_len = sizeof(maskbuf);
		evm.mask = maskbuf;
		memset(maskbuf, 0, sizeof(maskbuf));
		XISetMask(maskbuf, XI_RawMotion);
		XISetMask(maskbuf, XI_RawButtonPress);
		XISetMask(maskbuf, XI_RawButtonRelease);
/*		if (xi2.vmajor >= 2 && xi2.vminor >= 2)
		{
			XISetMask(maskbuf, XI_RawTouchBegin);
			XISetMask(maskbuf, XI_RawTouchUpdate);
			XISetMask(maskbuf, XI_RawTouchEnd);
		}
*/		xi2.pXISelectEvents(vid_dpy, DefaultRootWindow(vid_dpy), &evm, 1);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------*/

//qboolean is8bit = false;
//qboolean isPermedia = false;
extern qboolean sys_gracefulexit;

char *clipboard_buffer[2];


/*-----------------------------------------------------------------------*/

#ifdef GLQUAKE
static struct
{
	dllhandle_t *gllibrary;

	const char *glxextensions;

	XVisualInfo* (*ChooseVisual) (Display *dpy, int screen, int *attribList);
	void (*SwapBuffers) (Display *dpy, GLXDrawable drawable);
	Bool (*MakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
	GLXContext (*CreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
	void (*DestroyContext) (Display *dpy, GLXContext ctx);

	const char * (*QueryExtensionsString)(Display * dpy,  int screen);
	void *(*GetProcAddress) (char *name);
	void (*SwapIntervalSGI) (int interval);				//FFS!
	void (*SwapIntervalMESA) (unsigned int interval);	//FFS!
	void (*SwapIntervalEXT) (Display *dpy, GLXDrawable drawable, int interval);
	qboolean swaptear;

	GLXFBConfig *(*ChooseFBConfig)(Display *dpy, int screen, const int *attrib_list, int *nelements);
	int (*GetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int * value);
	XVisualInfo *(*GetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
	GLXContext (*CreateContextAttribs)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);

	int swapint;
} glx;

/*Note: closing the GLX library is unsafe as nvidia's drivers like to crash once its reloaded.
static void GLX_CloseLibrary(void)
{
	Sys_CloseLibrary(glx.gllibrary);
	glx.gllibrary = NULL;
}
*/

#if 0//def _DEBUG
//this is a list of the functions that exist in opengles2, as well as wglCreateContextAttribsARB.
//functions not in this list *should* be stubs that just return errors, but we can't always depend on drivers for that... they shouldn't get called.
//this list is just to make it easier to test+debug android gles2 stuff using windows.
static char *gles2funcs[] =
{
#define f(n) #n,
		f(glActiveTexture)
		f(glAttachShader)
		f(glBindAttribLocation)
		f(glBindBuffer)
		f(glBindFramebuffer)
		f(glBindRenderbuffer)
		f(glBindTexture)
		f(glBlendColor)
		f(glBlendEquation)
		f(glBlendEquationSeparate)
		f(glBlendFunc)
		f(glBlendFuncSeparate)
		f(glBufferData)
		f(glBufferSubData)
		f(glCheckFramebufferStatus)
		f(glClear)
		f(glClearColor)
		f(glClearDepthf)
		f(glClearStencil)
		f(glColorMask)
		f(glCompileShader)
		f(glCompressedTexImage2D)
		f(glCompressedTexSubImage2D)
		f(glCopyTexImage2D)
		f(glCopyTexSubImage2D)
		f(glCreateProgram)
		f(glCreateShader)
		f(glCullFace)
		f(glDeleteBuffers)
		f(glDeleteFramebuffers)
		f(glDeleteProgram)
		f(glDeleteRenderbuffers)
		f(glDeleteShader)
		f(glDeleteTextures)
		f(glDepthFunc)
		f(glDepthMask)
		f(glDepthRangef)
		f(glDetachShader)
		f(glDisable)
		f(glDisableVertexAttribArray)
		f(glDrawArrays)
		f(glDrawElements)
		f(glEnable)
		f(glEnableVertexAttribArray)
		f(glFinish)
		f(glFlush)
		f(glFramebufferRenderbuffer)
		f(glFramebufferTexture2D)
		f(glFrontFace)
		f(glGenBuffers)
		f(glGenerateMipmap)
		f(glGenFramebuffers)
 		f(glGenRenderbuffers)
		f(glGenTextures)
		f(glGetActiveAttrib)
		f(glGetActiveUniform)
		f(glGetAttachedShaders)
		f(glGetAttribLocation)
		f(glGetBooleanv)
		f(glGetBufferParameteriv)
		f(glGetError)
		f(glGetFloatv)
		f(glGetFramebufferAttachmentParameteriv)
		f(glGetIntegerv)
		f(glGetProgramiv)
		f(glGetProgramInfoLog)
		f(glGetRenderbufferParameteriv)
		f(glGetShaderiv)
		f(glGetShaderInfoLog)
		f(glGetShaderPrecisionFormat)
		f(glGetShaderSource)
		f(glGetString)
		f(glGetTexParameterfv)
		f(glGetTexParameteriv)
		f(glGetUniformfv)
		f(glGetUniformiv)
		f(glGetUniformLocation)
		f(glGetVertexAttribfv)
		f(glGetVertexAttribiv)
		f(glGetVertexAttribPointerv)
		f(glHint)
		f(glIsBuffer)
		f(glIsEnabled)
		f(glIsFramebuffer)
		f(glIsProgram)
		f(glIsRenderbuffer)
		f(glIsShader)
		f(glIsTexture)
		f(glLineWidth)
		f(glLinkProgram)
		f(glPixelStorei)
		f(glPolygonOffset)
		f(glReadPixels)
		f(glReleaseShaderCompiler)
		f(glRenderbufferStorage)
		f(glSampleCoverage)
		f(glScissor)
		f(glShaderBinary)
		f(glShaderSource)
		f(glStencilFunc)
		f(glStencilFuncSeparate)
		f(glStencilMask)
		f(glStencilMaskSeparate)
		f(glStencilOp)
		f(glStencilOpSeparate)
		f(glTexImage2D)
		f(glTexParameterf)
		f(glTexParameterfv)
		f(glTexParameteri)
		f(glTexParameteriv)
		f(glTexSubImage2D)
		f(glUniform1f)
		f(glUniform1fv)
		f(glUniform1i)
		f(glUniform1iv)
		f(glUniform2f)
		f(glUniform2fv)
		f(glUniform2i)
		f(glUniform2iv)
		f(glUniform3f)
		f(glUniform3fv)
		f(glUniform3i)
		f(glUniform3iv)
		f(glUniform4f)
		f(glUniform4fv)
		f(glUniform4i)
		f(glUniform4iv)
		f(glUniformMatrix2fv)
		f(glUniformMatrix3fv)
		f(glUniformMatrix4fv)
		f(glUseProgram)
		f(glValidateProgram)
		f(glVertexAttrib1f)
		f(glVertexAttrib1fv)
		f(glVertexAttrib2f)
		f(glVertexAttrib2fv)
		f(glVertexAttrib3f)
		f(glVertexAttrib3fv)
		f(glVertexAttrib4f)
		f(glVertexAttrib4fv)
		f(glVertexAttribPointer)
		f(glViewport)
		f(wglCreateContextAttribsARB)
		NULL
};

//this is a list of the functions that exist in opengles2, as well as wglCreateContextAttribsARB.
//functions not in this list *should* be stubs that just return errors, but we can't always depend on drivers for that... they shouldn't get called.
//this list is just to make it easier to test+debug android gles2 stuff using windows.
static char *gles1funcs[] =
{
#define f(n) #n,

		/* Available only in Common profile */
		f(glAlphaFunc)
		f(glClearColor)
		f(glClearDepthf)
		f(glClipPlanef)
		f(glColor4f)
		f(glDepthRangef)
		f(glFogf)
		f(glFogfv)
		f(glFrustumf)
		f(glGetClipPlanef)
		f(glGetFloatv)
		f(glGetLightfv)
		f(glGetMaterialfv)
		f(glGetTexEnvfv)
		f(glGetTexParameterfv)
		f(glLightModelf)
		f(glLightModelfv)
		f(glLightf)
		f(glLightfv)
		f(glLineWidth)
		f(glLoadMatrixf)
		f(glMaterialf)
		f(glMaterialfv)
		f(glMultMatrixf)
		f(glMultiTexCoord4f)
		f(glNormal3f)
		f(glOrthof)
		f(glPointParameterf)
		f(glPointParameterfv)
		f(glPointSize)
		f(glPolygonOffset)
		f(glRotatef)
		f(glScalef)
		f(glTexEnvf)
		f(glTexEnvfv)
		f(glTexParameterf)
		f(glTexParameterfv)
		f(glTranslatef)

		/* Available in both Common and Common-Lite profiles */
		f(glActiveTexture)
		f(glAlphaFuncx)
		f(glBindBuffer)
		f(glBindTexture)
		f(glBlendFunc)
		f(glBufferData)
		f(glBufferSubData)
		f(glClear)
		f(glClearColorx)
		f(glClearDepthx)
		f(glClearStencil)
		f(glClientActiveTexture)
		f(glClipPlanex)
		f(glColor4ub)
		f(glColor4x)
		f(glColorMask)
		f(glColorPointer)
		f(glCompressedTexImage2D)
		f(glCompressedTexSubImage2D)
		f(glCopyTexImage2D)
		f(glCopyTexSubImage2D)
		f(glCullFace)
		f(glDeleteBuffers)
		f(glDeleteTextures)
		f(glDepthFunc)
		f(glDepthMask)
		f(glDepthRangex)
		f(glDisable)
		f(glDisableClientState)
		f(glDrawArrays)
		f(glDrawElements)
		f(glEnable)
		f(glEnableClientState)
		f(glFinish)
		f(glFlush)
		f(glFogx)
		f(glFogxv)
		f(glFrontFace)
		f(glFrustumx)
		f(glGetBooleanv)
		f(glGetBufferParameteriv)
		f(glGetClipPlanex)
		f(glGenBuffers)
		f(glGenTextures)
		f(glGetError)
		f(glGetFixedv)
		f(glGetIntegerv)
		f(glGetLightxv)
		f(glGetMaterialxv)
		f(glGetPointerv)
		f(glGetString)
		f(glGetTexEnviv)
		f(glGetTexEnvxv)
		f(glGetTexParameteriv)
		f(glGetTexParameterxv)
		f(glHint)
		f(glIsBuffer)
		f(glIsEnabled)
		f(glIsTexture)
		f(glLightModelx)
		f(glLightModelxv)
		f(glLightx)
		f(glLightxv)
		f(glLineWidthx)
		f(glLoadIdentity)
		f(glLoadMatrixx)
		f(glLogicOp)
		f(glMaterialx)
		f(glMaterialxv)
		f(glMatrixMode)
		f(glMultMatrixx)
		f(glMultiTexCoord4x)
		f(glNormal3x)
		f(glNormalPointer)
		f(glOrthox)
		f(glPixelStorei)
		f(glPointParameterx)
		f(glPointParameterxv)
		f(glPointSizex)
		f(glPolygonOffsetx)
		f(glPopMatrix)
		f(glPushMatrix)
		f(glReadPixels)
		f(glRotatex)
		f(glSampleCoverage)
		f(glSampleCoveragex)
		f(glScalex)
		f(glScissor)
		f(glShadeModel)
		f(glStencilFunc)
		f(glStencilMask)
		f(glStencilOp)
		f(glTexCoordPointer)
		f(glTexEnvi)
		f(glTexEnvx)
		f(glTexEnviv)
		f(glTexEnvxv)
		f(glTexImage2D)
		f(glTexParameteri)
		f(glTexParameterx)
		f(glTexParameteriv)
		f(glTexParameterxv)
		f(glTexSubImage2D)
		f(glTranslatex)
		f(glVertexPointer)
		f(glViewport)

		/*required to switch stuff around*/
		f(wglCreateContextAttribsARB)
		f(glXGetProcAddress)
		f(glXQueryExtensionsString)
		f(glXChooseFBConfig)
		f(glXGetFBConfigAttrib)
		f(glXGetVisualFromFBConfig)
		f(glXCreateContextAttribsARB)
		NULL
};
#endif

static void *GLX_GetSymbol(char *name)
{
	void *symb;

#if 0//def _DEBUG
	if (1)
	{
		int i;
		for (i = 0; gles1funcs[i]; i++)
		{
			if (!strcmp(name, gles1funcs[i]))
				break;
		}
		if (!gles1funcs[i])
			return NULL;	//not in the list
	}
#endif

	if (glx.GetProcAddress)
		symb = glx.GetProcAddress(name);
	else
		symb = NULL;

	if (!symb)
		symb = Sys_GetAddressForName(glx.gllibrary, name);
	return symb;
}

static qboolean GLX_CheckExtension(const char *ext)
{
	const char *e = glx.glxextensions, *n;
	size_t el = strlen(ext);
	while(e && *e)
	{
		while (*e == ' ')
			e++;
		n = strchr(e, ' ');
		if (!n)
			n = n+strlen(e);

		if (n-e == el && !strncmp(ext, e, el))
		{
			Con_DPrintf("GLX: Found %s\n", ext);
			return true;
		}
		e = n;
	}
	Con_DPrintf("GLX: Missing %s\n", ext);
	return false;
}

static qboolean GLX_InitLibrary(char *driver)
{
	dllfunction_t funcs[] =
	{
		{(void*)&glx.ChooseVisual,		"glXChooseVisual"},
		{(void*)&glx.SwapBuffers,		"glXSwapBuffers"},
		{(void*)&glx.MakeCurrent,		"glXMakeCurrent"},
		{(void*)&glx.CreateContext,		"glXCreateContext"},
		{(void*)&glx.DestroyContext,	"glXDestroyContext"},
		{NULL,							NULL}
	};
	memset(&glx, 0, sizeof(glx));

	if (driver && *driver)
		glx.gllibrary = Sys_LoadLibrary(driver, funcs);
	else
		glx.gllibrary = NULL;
#ifdef __CYGWIN__
	if (!glx.gllibrary)
		glx.gllibrary = Sys_LoadLibrary("cygGL-1.dll", funcs);
#endif
	if (!glx.gllibrary)	//I hate this.
		glx.gllibrary = Sys_LoadLibrary("libGL.so.1", funcs);
	if (!glx.gllibrary)
		glx.gllibrary = Sys_LoadLibrary("libGL", funcs);
	if (!glx.gllibrary)
		return false;

	glx.QueryExtensionsString = GLX_GetSymbol("glXQueryExtensionsString");
	glx.GetProcAddress = GLX_GetSymbol("glXGetProcAddress");
	if (!glx.GetProcAddress)
		glx.GetProcAddress = GLX_GetSymbol("glXGetProcAddressARB");

	glx.ChooseFBConfig = GLX_GetSymbol("glXChooseFBConfig");
	glx.GetFBConfigAttrib = GLX_GetSymbol("glXGetFBConfigAttrib");
	glx.GetVisualFromFBConfig = GLX_GetSymbol("glXGetVisualFromFBConfig");
	glx.CreateContextAttribs = GLX_GetSymbol("glXCreateContextAttribsARB");
	return true;
}

#ifndef GLX_RGBA_FLOAT_BIT
#define GLX_RGBA_FLOAT_BIT	0x00000004
#endif
#ifndef GLX_CONTEXT_OPENGL_NO_ERROR_ARB
#define GLX_CONTEXT_OPENGL_NO_ERROR_ARB	0x31B3
#endif

//Since GLX1.3 (equivelent to gl1.2)
static GLXFBConfig GLX_GetFBConfig(rendererstate_t *info)
{
	int attrib[32];
	int n, i;
	int numconfigs;
	GLXFBConfig *fbconfigs;

	qboolean hassrgb, hasmultisample;//, hasfloats;

	if (!glx.ChooseFBConfig || !glx.GetVisualFromFBConfig)
	{
		Con_Printf("Missing function pointer\n");
		return NULL;
	}
	if (!glx.CreateContextAttribs)
	{
		Con_Printf("Missing CreateContextAttribs\n");
		return NULL;	//don't worry about it
	}

	hassrgb = GLX_CheckExtension("GLX_ARB_framebuffer_sRGB") || GLX_CheckExtension("GLX_EXT_framebuffer_sRGB");
	hasmultisample = GLX_CheckExtension("GLX_ARB_multisample");
//	hasfloats = GLX_CheckExtension("GLX_ARB_fbconfig_float");

	//do it in a loop, mostly to disable extensions that are unlikely to be supported on various glx implementations.
	for (i = 0; i < (16<<1); i++)
	{
		n = 0;
		//attrib[n++] = GLX_LEVEL;			attrib[n++] = 0;	//overlays
		attrib[n++] = GLX_DOUBLEBUFFER;		attrib[n++] = True;
		if (!(i&1))
		{
			if (!info->stereo)
				continue;
			attrib[n++] = GLX_STEREO; attrib[n++] = True;
		}
		if (!(i&2))
		{
			if (!info->srgb || !hassrgb)
				continue;
			attrib[n++] = GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB; attrib[n++] = True;
		}
		//attrib[n++] = GLX_AUX_BUFFERS;	attrib[n++] = 0;

		
#if 0
		if (!(i&4))
		{	//unlike on windows, this is explicitly blocked except for pbuffers. ffs.
			if (info->srgb < 2 || !hasfloats)
				continue;	//skip fp16 framebuffers
			//unlike on windows, this is explicitly blocked except for pbuffers. ffs.
			attrib[n++] = GLX_RENDER_TYPE;		attrib[n++] = GLX_RGBA_FLOAT_BIT;
			attrib[n++] = GLX_RED_SIZE;			attrib[n++] = info->bpp?info->bpp/3:4;
			attrib[n++] = GLX_GREEN_SIZE;		attrib[n++] = info->bpp?info->bpp/3:4;
			attrib[n++] = GLX_BLUE_SIZE;		attrib[n++] = info->bpp?info->bpp/3:4;
		}
		else
#endif
		{
			if (info->bpp == 32)
			{	//bpp32 is an alias for 24 (we ignore the alpha channel)
				attrib[n++] = GLX_RED_SIZE;			attrib[n++] = 8;
				attrib[n++] = GLX_GREEN_SIZE;		attrib[n++] = 8;
				attrib[n++] = GLX_BLUE_SIZE;		attrib[n++] = 8;
			}
			else
			{	//clamp requested bitdepth to 8bits on the second pass, so that bpp30 doesn't fail
				attrib[n++] = GLX_RED_SIZE;			attrib[n++] = bound(1,info->bpp?info->bpp/3:4, (i&4)?8:16);
				attrib[n++] = GLX_GREEN_SIZE;		attrib[n++] = bound(1,info->bpp?info->bpp/3:4, (i&4)?8:16);
				attrib[n++] = GLX_BLUE_SIZE;		attrib[n++] = bound(1,info->bpp?info->bpp/3:4, (i&4)?8:16);
			}
		}
		//attrib[n++] = GLX_ALPHA_SIZE;		attrib[n++] = GLX_DONT_CARE;
		attrib[n++] = GLX_DEPTH_SIZE;		attrib[n++] = info->depthbits?info->depthbits:16;
		attrib[n++] = GLX_STENCIL_SIZE;		attrib[n++] = (i&16)?0:4;

		if (!(i&8))
		{
			if (!info->multisample || !hasmultisample)
				continue;
			attrib[n++] = GLX_SAMPLE_BUFFERS_ARB;	attrib[n++] = True;
			attrib[n++] = GLX_SAMPLES_ARB;			attrib[n++] = info->multisample;
		}

		//attrib[n++] = GLX_ACCUM_RED_SIZE;	attrib[n++] = 0;
		//attrib[n++] = GLX_ACCUM_GREEN_SIZE;	attrib[n++] = 0;
		//attrib[n++] = GLX_ACCUM_BLUE_SIZE;	attrib[n++] = 0;
		//attrib[n++] = GLX_ACCUM_ALPHA_SIZE;	attrib[n++] = 0;

		attrib[n++] = GLX_DRAWABLE_TYPE;	attrib[n++] = GLX_WINDOW_BIT;
		attrib[n++] = GLX_X_RENDERABLE;		attrib[n++] = True;
		//attrib[n++] = GLX_X_VISUAL_TYPE;	attrib[n++] = info->srgb?GLX_TRUE_COLOR:GLX_DIRECT_COLOR;
		//attrib[n++] = GLX_CONFIG_CAVEAT;	attrib[n++] = GLX_DONT_CARE; GLX_NONE||GLX_SLOW_CONFIG||GLX_NON_CONFORMANT_CONFIG
		//attrib[n++] = GLX_TRANSPARENT_TYPE;attrib[n++] = GLX_NONE;
		//attrib[n++] = GLX_TRANSPARENT_ALPHA_VALUE;attrib[n++] = 0;
		attrib[n++] = None;

		numconfigs = 0;
		fbconfigs = glx.ChooseFBConfig(vid_dpy, scrnum, attrib, &numconfigs);
		if (fbconfigs)
		{
			if (numconfigs)
			{
				GLXFBConfig r = fbconfigs[0];
				x11.pXFree(fbconfigs);
				return r;
			}
			x11.pXFree(fbconfigs);
		}
	}
	return NULL;
}
//for GLX<1.3
static XVisualInfo *GLX_GetVisual(rendererstate_t *info)
{
	XVisualInfo *visinfo;
	int attrib[32];
	int numattrib, i;

	//do it in a loop, mostly to disable extensions that are unlikely to be supported on various glx implementations.
	for (i = 0; i < (8<<1); i++)
	{
		numattrib = 0;
		attrib[numattrib++] = GLX_RGBA;
		attrib[numattrib++] = GLX_DOUBLEBUFFER;
		if (!(i&1))
		{
			if (!info->stereo)
				continue;
			attrib[numattrib++] = GLX_STEREO;
		}
		if (!(i&2))
		{
			if (!info->srgb)
				continue;
			attrib[numattrib++] = GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB;
		}
		if (!(i&4))
		{
			if (!info->multisample)
				continue;
			attrib[numattrib++] = GLX_SAMPLE_BUFFERS_ARB; attrib[numattrib++] = info->multisample;
		}
		if (!(i&8))
		{
			attrib[numattrib++] = GLX_STENCIL_SIZE;	attrib[numattrib++] = 4;
		}
		attrib[numattrib++] = GLX_RED_SIZE;		attrib[numattrib++] = 4;
		attrib[numattrib++] = GLX_GREEN_SIZE;	attrib[numattrib++] = 4;
		attrib[numattrib++] = GLX_BLUE_SIZE;	attrib[numattrib++] = 4;
		attrib[numattrib++] = GLX_DEPTH_SIZE;	attrib[numattrib++] = 16;
		attrib[numattrib++] = None;

		visinfo = glx.ChooseVisual(vid_dpy, scrnum, attrib);
		if (visinfo)
			return visinfo;
	}
	return NULL;
}

static qboolean GLX_Init(rendererstate_t *info, GLXFBConfig fbconfig, XVisualInfo *visinfo)
{
	extern cvar_t	vid_gl_context_version;
	extern cvar_t	vid_gl_context_forwardcompatible;
	extern cvar_t	vid_gl_context_compatibility;
	extern cvar_t	vid_gl_context_debug;
	extern cvar_t	vid_gl_context_es;
	extern cvar_t	vid_gl_context_robustness;
//	extern cvar_t	vid_gl_context_selfreset;
	extern cvar_t	vid_gl_context_noerror;

	vrsetup_t setup = {sizeof(setup)};
	setup.vrplatform = VR_X11_GLX;
	setup.x11_glx.display = vid_dpy;
	setup.x11_glx.visualid = visinfo->visualid;
	setup.x11_glx.glxfbconfig = fbconfig;
	setup.x11_glx.drawable = vid_window;

	if (info->vr && !info->vr->Prepare(&setup))
	{
		info->vr->Shutdown();
		info->vr = NULL;
	}

	if (fbconfig && glx.CreateContextAttribs)
	{
		unsigned int majorver=1, minorver=1;
		unsigned profile = 0;
		unsigned contextflags = 0;
		int attrib[16*2+1];
		int n, val;
		char *ver;

		ver = vid_gl_context_version.string;
		if (!*ver && vid_gl_context_es.ival && GLX_CheckExtension("GLX_EXT_create_context_es_profile"))
			ver = vid_gl_context_es.string;
		if (!*ver && !vid_gl_context_compatibility.ival && *vid_gl_context_compatibility.string)
			ver = "3.2";	//if the user explicitly disabled compat, then they'll want a version that actually supports doing so.

		majorver = strtoul(ver, &ver, 10);
		if (*ver == '.')
			minorver = strtoul(ver+1, &ver, 10);
		else
			minorver = 0;

		if (majorver < setup.minver.major || (majorver == setup.minver.major && minorver < setup.minver.minor))
		{	//if vr stuff requires a minimum version then try and ask for that now
			majorver = setup.minver.major;
			minorver = setup.minver.minor;
		}

		//some weirdness for you:
		//3.0 simply marked stuff as deprecated, without removing it.
		//3.1 removed it (moved to the optional GL_ARB_compatibility extension, which shouldn't be supported in a forward-compatible context).
		//3.2 added the concept of profiles.

		if (vid_gl_context_debug.ival)
			contextflags |= GLX_CONTEXT_DEBUG_BIT_ARB;
		if (vid_gl_context_forwardcompatible.ival)	//treat this as a dev/debug thing.
			contextflags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if (vid_gl_context_robustness.ival && GLX_CheckExtension("GLX_ARB_create_context_robustness"))
			contextflags |= GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB;

		if (vid_gl_context_es.ival && GLX_CheckExtension("GLX_EXT_create_context_es_profile"))
			profile = GLX_CONTEXT_ES_PROFILE_BIT_EXT;
		else if (majorver>3 || (majorver==3&&minorver>=2))
		{	//profiles only started with 3.2
			if (vid_gl_context_compatibility.ival)
				profile = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
			else
				profile = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
		}
		else
			profile = 0;

		n = 0;
		if (majorver || minorver)
		{
			attrib[n++] = GLX_CONTEXT_MAJOR_VERSION_ARB;		attrib[n++] = majorver;
			attrib[n++] = GLX_CONTEXT_MINOR_VERSION_ARB;		attrib[n++] = minorver;
		}
		if (contextflags)
		{
			attrib[n++] = GLX_CONTEXT_FLAGS_ARB;				attrib[n++] = contextflags;
		}
		if (profile)
		{
			attrib[n++] = GLX_CONTEXT_PROFILE_MASK_ARB;		attrib[n++] = profile;
		}
		if (vid_gl_context_noerror.ival &&  GLX_CheckExtension("GLX_ARB_create_context_no_error"))
		{
			attrib[n++] = GLX_CONTEXT_OPENGL_NO_ERROR_ARB;	attrib[n++] = !vid_gl_context_robustness.ival && !vid_gl_context_debug.ival;
		}
		attrib[n++] = None;

		if (glx.GetFBConfigAttrib)
		{
			if (!glx.GetFBConfigAttrib(vid_dpy, fbconfig, GLX_RENDER_TYPE, &val) && val == GLX_RGBA_FLOAT_BIT)
				vid.flags |= VID_FP16;			//other things need to be 16bit too, to avoid loss of precision.
			if (!glx.GetFBConfigAttrib(vid_dpy, fbconfig, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &val) && val)
				vid.flags |= VID_SRGB_CAPABLE;	//can use srgb properly, without faking it etc.
		}

		ctx = glx.CreateContextAttribs(vid_dpy, fbconfig, NULL, True, attrib);
	}
	else
		ctx = glx.CreateContext(vid_dpy, visinfo, NULL, True);
	if (!ctx)
	{
		Con_Printf("Failed to create GLX context.\n");
		return false;
	}

	if (!glx.MakeCurrent(vid_dpy, vid_window, ctx))
	{
		Con_Printf("glXMakeCurrent failed\n");
		return false;
	}


	setup.x11_glx.glxcontext = ctx;
	if (info->vr && !info->vr->Init(&setup, info))
	{
		info->vr->Shutdown();
		return false;
	}
	vid.vr = info->vr;

	//okay, we have a context, now init OpenGL-proper.
	return GL_Init(info, &GLX_GetSymbol);
}
#endif

static void X_ShutdownUnicode(void)
{
	if (x11.unicodecontext)
		x11.pXDestroyIC(x11.unicodecontext);
	x11.unicodecontext = NULL;
	if (x11.inputmethod)
		x11.pXCloseIM(x11.inputmethod);
	x11.inputmethod = NULL;
	x11.dounicode = false;
}
static int XIMPreEditStartCallback(XIC ic, XPointer client_data, XPointer call_data)
{
	Z_Free(vid.ime_preview);	//just in case
	vid.ime_preview = NULL;
	vid.ime_previewlen = 0;
//	Con_Printf("XIMPreEditStartCallback\n");
	return -1;	//length of string we can handle (negative for unlimited)
}
static void XIMPreEditDoneCallback(XIC ic, XPointer client_data, XPointer call_data)
{
//	Con_Printf("XIMPreEditDoneCallback\n");

	Z_Free(vid.ime_preview);
	vid.ime_preview = NULL;
	vid.ime_previewlen = 0;
}
static void XIMPreEditDrawCallback(XIM ic, XPointer client_data, XIMPreeditDrawCallbackStruct *d)
{
	//if chg_length, wipe chg_length chars @chg_first.
	//if text, insert at chg_first (with per-char feedback properties)
	//if feedback (without text) then change text char flags
	//caret should then be moved accordingly.
//	Con_Printf("XIMPreEditDrawCallback %i %i %i %i %s\n", d->caret, d->chg_first, d->chg_length, d->text?d->text->encoding_is_wchar:0, d->text?d->text->string.multi_byte:"???");
	conchar_t *part[3];
	size_t clen[3], c;
	conchar_t *n;
	unsigned int wc;
	unsigned int defaultfl = CON_WHITEMASK, fl;

	if (d->chg_length || (d->text && d->text->length))
	{
		//so inputs are in terms of chars.
		//our conchar_t struct is variable-sized (*sigh*), so we always use our longchar encoding.
		//so we end up with two conchars per wchar.
		part[0] = vid.ime_preview;
		clen[0] = bound(0, d->chg_first, vid.ime_previewlen/2)*2;
		part[1] = NULL;
		clen[1] = 0;
		part[2] = part[0]+clen[0] + bound(0, d->chg_length, vid.ime_previewlen/2)*2;
		clen[2] = 0;
		if (part[2])
			while (part[2][clen[2]])
				clen[2]+=1;
		if (d->text && d->text->encoding_is_wchar && d->text->string.wide_char)
		{
			part[1] = alloca(d->text->length * 2*sizeof(wchar_t));
			for (c = 0; c < d->text->length; c++)
			{
				wc = d->text->string.wide_char[c];
				if (!wc)
					break; //erk? nulls confuse things
				part[1][c*2+0] = CON_LONGCHAR|(wc>>16);
				part[1][c*2+1] = defaultfl|(wc&CON_CHARMASK);
			}
			clen[1] = c*2;
		}
		else if (d->text && !d->text->encoding_is_wchar && d->text->string.multi_byte)
		{
			const char *in = d->text->string.multi_byte;
			int error;
			part[1] = alloca(d->text->length * 2*sizeof(wchar_t));
			//FIXME: d->text->length is meant to be chars, but fcitx always reports 1.
			for (c = 0; c < d->text->length; c++)
			{
				//FIXME: This should use mbcstowcs, but that would require switching locales all the frikkin time, which isn't thread-safe.
				wc = utf8_decode(&error, in, &in);
				if (!wc)
					break;	//abort if there's a null...
				part[1][c*2+0] = CON_LONGCHAR|(wc>>16);
				part[1][c*2+1] = defaultfl|(wc&CON_CHARMASK);
			}
			clen[1] = c*2;
		}

		n = Z_Malloc((clen[0]+clen[1]+clen[2]+1)*sizeof(*n));
		memcpy(n,					part[0],	clen[0]*sizeof(*n));
		memcpy(n+clen[0],			part[1],	clen[1]*sizeof(*n));
		memcpy(n+clen[0]+clen[1],	part[2],	clen[2]*sizeof(*n));
		n[clen[0]+clen[1]+clen[2]] = 0;
		Z_Free(vid.ime_preview);
		vid.ime_preview = n;
		vid.ime_previewlen = clen[0]+clen[1]+clen[2];
	}

	if (d->text && d->text->feedback && d->chg_first >= 0 && d->chg_first+d->text->length <= vid.ime_previewlen/2)
	{
		for (c = 0; c < d->text->length; c++)
		{
			fl = defaultfl;
			if (d->text->feedback[c] & XIMPrimary)
				fl = COLOR_RED<<CON_FGSHIFT;
			if (d->text->feedback[c] & XIMSecondary)
				fl = COLOR_GREEN<<CON_FGSHIFT;
			if (d->text->feedback[c] & XIMTertiary)
				fl = COLOR_BLUE<<CON_FGSHIFT;
			if (d->text->feedback[c] & XIMUnderline)
				fl = COLOR_MAGENTA<<CON_FGSHIFT;
			if (d->text->feedback[c] & XIMReverse)
				fl = ((fl&CON_FGMASK) << (CON_BGSHIFT-CON_FGSHIFT)) | ((fl&CON_BGMASK) >> (CON_BGSHIFT-CON_FGSHIFT));
			if (d->text->feedback[c] & XIMHighlight)
				fl |= CON_2NDCHARSETTEXT;
			vid.ime_preview[(d->chg_first+c)*2+1] = ((vid.ime_preview[(d->chg_first+c)*2+1])&~CON_FLAGSMASK)|fl;
		}
	}

	vid.ime_caret = bound(0, d->caret, vid.ime_previewlen/2)*2;
}
static void XIMPreEditCaretCallback(XIC ic, XPointer client_data, XIMPreeditCaretCallbackStruct *d)
{
//	Con_Printf("XIMPreEditCaretCallback %i %i %i\n", d->direction, d->position, d->style);
}
static qboolean XIMSupportedStyle(XIMStyle preedit, XIMStyle status)
{
	if (!(	//preedit==XIMPreeditCallbacks||	//FIXME: this should actually work, but we still need an ime that actually supports it properly to be sure.
			//preedit==XIMPreeditPosition||
			//preedit==XIMPreeditArea||			//FIXME: assume the bottom half of the screen
			preedit==XIMPreeditNothing||
			preedit==XIMPreeditNone))
		return false;
	if (!(	//status==XIMStatusCallbacks||
			//status==XIMStatusArea||
			status==XIMStatusNothing||
			status==XIMStatusNone))
		return false;

	return true;
}
static XIMStyle XIMPreferredStyle(XIMStyle old, XIMStyle new)
{	//favour the more complicated (supported) preedit styles, *THEN* choose the preferred status style.
	XIMStyle p1 = old&0x00ff;
	XIMStyle p2 = new&0x00ff;
	XIMStyle s1 = old&0xff00;
	XIMStyle s2 = new&0xff00;
	if (!XIMSupportedStyle(p2, s2))
		return old;
	if (!XIMSupportedStyle(p1, s1))
		return new;
	if (p1 != p2)
	{	//choose based upon the preedit flags
		if ((p1^p2)&XIMPreeditCallbacks)			//FIXME: support this one properly some time.
			return (p1&XIMPreeditCallbacks)?old:new;
		if ((p1^p2)&XIMPreeditPosition)
			return (p1&XIMPreeditPosition)?old:new;
		if ((p1^p2)&XIMPreeditArea)
			return (p1&XIMPreeditArea)?old:s2;
		if ((p1^p2)&XIMPreeditNothing)
			return (p1&XIMPreeditNothing)?old:new;
		if ((p1^p2)&XIMPreeditNone)
			return (p1&XIMPreeditNone)?old:new;
	}
	else
	{	//preedit flags are equal, now pick the better
		if ((s1^s2)&XIMStatusCallbacks)
			return (s1&XIMStatusCallbacks)?old:new;
		if ((s1^s2)&XIMStatusArea)
			return (s1&XIMStatusArea)?old:new;
		if ((s1^s2)&XIMStatusNothing)
			return (s1&XIMStatusNothing)?old:new;
		if ((s1^s2)&XIMStatusNone)
			return (s1&XIMStatusNone)?old:new;
	}

	//difference not known. stick with the first
	return old;
}
#include <locale.h>
static long X_InitUnicode(void)
{
	long requiredevents = 0;
	X_ShutdownUnicode();

	//FIXME: enable by default if ubuntu's issue can ever be resolved.
	if (X11_CheckFeature("xim", true))
	{
		if (x11.pXSetLocaleModifiers && x11.pXSupportsLocale && x11.pXOpenIM && x11.pXGetIMValues && x11.pXCreateIC && x11.pXSetICFocus && x11.pXUnsetICFocus && x11.pXGetICValues && x11.pXSetICValues && x11.pXFilterEvent && (x11.pXutf8LookupString || x11.pXwcLookupString) && x11.pXDestroyIC && x11.pXCloseIM)
		{
			setlocale(LC_ALL, "");	//just in case.
			x11.pXSetLocaleModifiers("");

			if (x11.pXSupportsLocale())
			{
				x11.inputmethod = x11.pXOpenIM(vid_dpy, NULL, XI_RESOURCENAME, XI_RESOURCECLASS);
				if (x11.inputmethod)
				{
					XIMStyles *sup = NULL;
					XIMStyle st = 0;
					int i;
					x11.pXGetIMValues(x11.inputmethod, XNQueryInputStyle, &sup, NULL);
					for (i = 0; sup && i < sup->count_styles; i++)
						st = XIMPreferredStyle(st, sup->supported_styles[i]);
					x11.pXFree(sup);
					Con_DPrintf("Chosen XIM Input Style: %x\n", (unsigned)st);
//					st=XIMPreeditCallbacks|XIMStatusArea;
//					st=XIMPreeditCallbacks|XIMStatusNothing;
					if (st != 0)
					{
						XIMCallback pe_cb_start={NULL,(XIMProc)XIMPreEditStartCallback};
						XIMCallback pe_cb_done={NULL,(XIMProc)XIMPreEditDoneCallback};
						XIMCallback pe_cb_draw={NULL,(XIMProc)XIMPreEditDrawCallback};
						XIMCallback pe_cb_caret={NULL,(XIMProc)XIMPreEditCaretCallback};
						void *preedit[] = {
								//should probably add in fonts, but that's kinda messy if we don't know the language/charset very well
								XNPreeditStartCallback,	&pe_cb_start,
								XNPreeditDoneCallback,	&pe_cb_done,
								XNPreeditDrawCallback,	&pe_cb_draw,
								XNPreeditCaretCallback,	&pe_cb_caret,
								XNSpotLocation,			&x11.ime_pos,
								NULL};
						void *status[] = {
								NULL};

						x11.unicodecontext = x11.pXCreateIC(x11.inputmethod,
							XNInputStyle, st,
							XNClientWindow, vid_window,
//							XNFocusWindow, vid_window,
							XNResourceName, XI_RESOURCENAME,
							XNResourceClass, XI_RESOURCECLASS,
							XNPreeditAttributes, preedit,
							XNStatusAttributes, status,
							NULL);
						if (x11.unicodecontext)
						{
							x11.ime_shown = -1;
							x11.dounicode = true;

							x11.pXGetICValues(x11.unicodecontext, XNFilterEvents, &requiredevents, NULL);
							requiredevents |= KeyPressMask;
						}
					}
				}
			}
			setlocale(LC_ALL, "C");
		}
	}

	Con_DPrintf("Unicode support: %s\n", x11.dounicode?"available":"unavailable");

	return requiredevents;
}

#define XKEY_QUAKE_MAP()			\
	XKQM(XK_KP_Page_Up,		K_KP_PGUP)		\
	XKQM(XK_Page_Up,		K_PGUP)			\
	XKQM(XK_KP_Page_Down,	K_KP_PGDN)		\
	XKQM(XK_Page_Down,		K_PGDN)			\
	XKQM(XK_KP_Home,		K_KP_HOME)		\
	XKQM(XK_Home,			K_HOME)			\
	XKQM(XK_KP_End,			K_KP_END)		\
	XKQM(XK_End,			K_END)			\
	XKQM(XK_KP_Left,		K_KP_LEFTARROW)	\
	XKQM(XK_Left,			K_LEFTARROW)	\
	XKQM(XK_KP_Right,		K_KP_RIGHTARROW)\
	XKQM(XK_Right,			K_RIGHTARROW)	\
	XKQM(XK_KP_Down,		K_KP_DOWNARROW)	\
	XKQM(XK_Down,			K_DOWNARROW)	\
	XKQM(XK_KP_Up,			K_KP_UPARROW)	\
	XKQM(XK_Up,				K_UPARROW)		\
	XKQM(XK_Escape,			K_ESCAPE)		\
	XKQM(XK_KP_Enter,		K_KP_ENTER)		\
	XKQM(XK_Return,			K_ENTER)		\
	XKQM(XK_Num_Lock,		K_KP_NUMLOCK)	\
	XKQM(XK_Caps_Lock,		K_CAPSLOCK)		\
	XKQM(XK_Scroll_Lock,	K_SCRLCK)		\
	XKQM(XK_Print,			K_PRINTSCREEN)	\
	XKQM(XK_Super_L,		K_LWIN)			\
	XKQM(XK_Super_R,		K_RWIN)			\
	XKQM(XK_Tab,			K_TAB)			\
	XKQM(XK_F1,				K_F1)			\
	XKQM(XK_F2,				K_F2)			\
	XKQM(XK_F3,				K_F3)			\
	XKQM(XK_F4,				K_F4)			\
	XKQM(XK_F5,				K_F5)			\
	XKQM(XK_F6,				K_F6)			\
	XKQM(XK_F7,				K_F7)			\
	XKQM(XK_F8,				K_F8)			\
	XKQM(XK_F9,				K_F9)			\
	XKQM(XK_F10,			K_F10)			\
	XKQM(XK_F11,			K_F11)			\
	XKQM(XK_F12,			K_F12)			\
	XKQM(XK_F13,			K_F13)			\
	XKQM(XK_F14,			K_F14)			\
	XKQM(XK_F15,			K_F15)			\
	XKQM(XK_BackSpace,		K_BACKSPACE)	\
	XKQM(XK_KP_Delete,		K_KP_DEL)		\
	XKQM(XK_Delete,			K_DEL)			\
	XKQM(XK_Pause,			K_PAUSE)		\
	XKQM(XK_Shift_L,		K_LSHIFT)		\
	XKQM(XK_Shift_R,		K_RSHIFT)		\
	/*XKQM2(XK_Execute,		K_LCTRL)*/		\
	XKQM(XK_Control_L,		K_LCTRL)		\
	XKQM(XK_Control_R,		K_RCTRL)		\
	XKQM(XK_Alt_L,			K_LALT)			\
	/*XKQM2(XK_Meta_L,		K_LWIN)*/		\
	XKQM(XK_Alt_R,			K_RALT)			\
	/*XKQM2(XK_Meta_R,		K_RWIN)*/		\
	XKQM(XK_Menu,			K_APP)			\
	XKQM(XK_KP_Begin,		K_KP_5)			\
	XKQM(XK_KP_Insert,		K_KP_INS)		\
	XKQM(XK_Insert,			K_INS)			\
	XKQM(XK_KP_Multiply,	K_KP_STAR)		\
	XKQM(XK_KP_Add,			K_KP_PLUS)		\
	XKQM(XK_KP_Subtract,	K_KP_MINUS)		\
	XKQM(XK_KP_Divide,		K_KP_SLASH)


qboolean INS_KeyToLocalName(int qkey, char *buf, size_t bufsize)
{
	char *s;
	int xk;
	*buf = 0;
	if(vid_dpy)
	{
		switch(qkey)
		{
#define XKQM(x,q) case q: xk = x; break;
XKEY_QUAKE_MAP()
#undef XKQM
		default:
			if (qkey >= K_SPACE && qkey < 127)
				xk = qkey;	//printable ascii is identity-mapped in both
			else
				return false;	//don't know how to map this. might not be a keyboard key.
			break;
		}
		s = x11.pXKeysymToString(xk);
		if (s)
			Q_strncpyz(buf, s, bufsize);
	}
	return !!*buf;
}

static void X_KeyEvent(XKeyEvent *ev, qboolean pressed, qboolean filtered)
{
	int i;
	int key;
	KeySym keysym, shifted;
	unsigned int unichar[64];
	int unichars = 0;
	key = 0;

	keysym = x11.pXLookupKeysym(ev, 0);
	if (pressed && !filtered)
	{
		if (x11.dounicode && vid.ime_allow)
		{
			Status status = XLookupNone;
			if (x11.pXutf8LookupString)
			{
				char buf1[512] = {0};
				char *buf = buf1;
				const char *c;
				int count = x11.pXutf8LookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf1, sizeof(buf1), NULL, &status);
				if (status == XBufferOverflow)
				{
					buf = alloca(count+1);
					count = x11.pXutf8LookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, count, NULL, &status);
				}
				for (c = buf; c < &buf[count]; )
				{
					int error;
					unsigned int uc = utf8_decode(&error, c, &c);
					if (uc)
						unichar[unichars++] = uc;
				}
			}
			else
			{
				//is allowed some weird encodings...
				wchar_t buf1[512] = {0};
				wchar_t *buf = buf1;
				int count = x11.pXwcLookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, sizeof(buf1), &shifted, &status);
				if (status == XBufferOverflow)
				{
					buf = alloca(sizeof(wchar_t)*(count+1));
					count = x11.pXwcLookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, count, NULL, &status);
				}
				//if wchar_t is 16bit, then expect problems when we completely ignore surrogates. this is why we favour the utf8 route as it doesn't care whether wchar_t is defined as 16bit or 32bit.
				for (i = 0; i < count; i++)
					if (buf[i])
						unichar[unichars++] = buf[i];
			}
		}
		else
		{
			char buf[64];
			if ((keysym & 0xff000000) == 0x01000000)
				unichar[unichars++] = keysym & 0x00ffffff;
			else
			{
				int count = x11.pXLookupString(ev, buf, sizeof(buf), &shifted, 0);
				for (i = 0; i < count; i++)
					if (buf[i])
						unichar[unichars++] = (unsigned char)buf[i];
			}
		}
	}

	switch(keysym)
	{
#define XKQM(x,q) case x: key = q; break;
XKEY_QUAKE_MAP()
#undef XKQM

		default:
			key = keysym;
			if (key < 32)
				key = 0;
			else if (key > 127)
				key = 0;
			else if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
			break;
	}

	if (unichars)
	{
		//we got some text, this is fun isn't it?
		//the key value itself is sent with the last text char. this avoids multiple presses, and dead keys were already sent.
		for (i = 0; i < unichars-1; i++)
		{
			IN_KeyEvent(0, pressed, 0, unichar[i]);
		}
		IN_KeyEvent(0, pressed, key, unichar[i]);
	}
	else
	{
		//no text available, just do the keypress
		IN_KeyEvent(0, pressed, key, 0);
	}
}

static void install_grabs(void)
{
	if (!mouse_grabbed)
	{
		//XGrabPointer can cause alt+tab type shortcuts to be skipped by the window manager. This means we don't want to use it unless we have no choice.
		//the grab is purely to constrain the pointer to the window
		if (GrabSuccess != x11.pXGrabPointer(vid_dpy, DefaultRootWindow(vid_dpy),
					True,
					0,
					GrabModeAsync, GrabModeAsync,
					vid_window,
					None,
					CurrentTime))
		{
			Con_Printf("Pointer grab failed\n");
			return;
		}

		Con_DLPrintf(2, "Grabbed mouse\n");
		mouse_grabbed = true;

		if (x11_input_method == XIM_DGA)
		{
			dgam.pXF86DGADirectVideo(vid_dpy, DefaultScreen(vid_dpy), XF86DGADirectMouse);
		}
		else
		{
			x11.pXWarpPointer(vid_dpy, None, vid_window,
						 0, 0, 0, 0,
						 vid.pixelwidth / 2, vid.pixelheight / 2);
		}

//		x11.pXSync(vid_dpy, True);
	}
}

static void uninstall_grabs(void)
{
	if (mouse_grabbed && vid_dpy)
	{
		Con_DLPrintf(2, "Releasing mouse grab\n");
		mouse_grabbed = false;
		if (x11_input_method == XIM_DGA)
		{
			dgam.pXF86DGADirectVideo(vid_dpy, DefaultScreen(vid_dpy), 0);
		}

		x11.pXUngrabPointer(vid_dpy, CurrentTime);

//		x11.pXSync(vid_dpy, True);

		if (!vid.forcecursor)
		{
			vid.forcecursor = true;
			vid.forcecursorpos[0] = vid.pixelwidth/2;
			vid.forcecursorpos[1] = vid.pixelheight/2;
		}
	}

	if (vid.forcecursor)
	{
		vid.forcecursor = false;
		x11.pXWarpPointer(vid_dpy, vid_window, vid_window,
						 0, 0, vid.pixelwidth, vid.pixelheight,
						 vid.forcecursorpos[0], vid.forcecursorpos[1]);
	}
}

static void UpdateGrabs(void)
{
	qboolean wantmgrabs, allownullcursor;
	Cursor wantcursor;

	wantmgrabs = (fullscreenflags&FULLSCREEN_ACTIVE) || !!in_windowed_mouse.value;
	if (!vid.activeapp)
		wantmgrabs = false;
	allownullcursor = wantmgrabs;	//this says whether we can possibly want it. if false then we disallow the null cursor. Yes, this might break mods that do their own sw cursors. such mods are flawed in other ways too.
	if (!vrui.enabled && Key_MouseShouldBeFree())
		wantmgrabs = false;
//	if (modeswitchpending)
//		wantmgrabs = false;

	if (wantmgrabs)
		install_grabs();
	else
		uninstall_grabs();

	if (mouse_grabbed)
		wantcursor = vid_nullcursor;
	else if (vid_newcursor)
		wantcursor = vid_newcursor;
	else if (!allownullcursor)
		wantcursor = None;
	else
		wantcursor = vid_nullcursor;
	if (wantcursor != vid_activecursor)
	{
		vid_activecursor = wantcursor;
		if (vid_activecursor)
			x11.pXDefineCursor(vid_dpy, vid_window, vid_activecursor);
		else
			x11.pXUndefineCursor(vid_dpy, vid_window);
	}

	X11Xss_SuspendSaver(cls.demoplayback && vid.activeapp);
}

static void ClearAllStates (void)
{
	int		i;

// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<K_MAX ; i++)
	{
		Key_Event (0, i, 0, false);
	}

	Key_ClearStates ();
//	IN_ClearStates ();
}

static void GetEvent(void)
{
	XEvent event, rep;
	int b;
	qboolean x11violations = true;
	Window mw;
	qboolean filtered = false;

	x11.pXNextEvent(vid_dpy, &event);

	if (x11.dounicode)
	{
		if (x11.pXFilterEvent(&event, None))//vid_window))
		{
			if (vid.ime_allow)
				return;
			filtered = true;
		}
	}

	switch (event.type)
	{
	case GenericEvent:
		if (x11.pXGetEventData(vid_dpy, &event.xcookie))
		{
			if (event.xcookie.extension == xi2.opcode)
			{
				switch(event.xcookie.evtype)
				{
				case XI_RawButtonPress:
				case XI_RawButtonRelease:
					if (mouse_grabbed)
					{
						XIRawEvent *raw = event.xcookie.data;
						int *qdev = &XI2_GetDeviceInfo(raw->sourceid)->qdev;
						int button = raw->detail;	//1-based
						if (raw->sourceid != raw->deviceid)
							return;	//ignore master devices to avoid dupes.
						if (*qdev == DEVID_UNSET)
							*qdev = xi2.nextqdev++;
						switch(button)
						{
						case 1: button = K_MOUSE1; break;
						case 2: button = K_MOUSE3; break;
						case 3: button = K_MOUSE2; break;
						case 4: button = K_MWHEELUP; break;	//so much for 'raw'.
						case 5: button = K_MWHEELDOWN; break;
						case 6: button = K_MWHEELLEFT; break;	//so much for 'raw'.
						case 7: button = K_MWHEELRIGHT; break;
						case 8: button = K_MOUSE4; break;
						case 9: button = K_MOUSE5; break;
						case 10: button = K_MOUSE6; break;
						case 11: button = K_MOUSE7; break;
						case 12: button = K_MOUSE8; break;
						case 13: button = K_MOUSE9; break;
						case 14: button = K_MOUSE10; break;
						default:button = 0; break;
						}
						if (button)
							IN_KeyEvent(*qdev, (event.xcookie.evtype==XI_RawButtonPress), button, 0);
					}
					break;
				case XI_RawMotion:
					if (mouse_grabbed)
					{
						XIRawEvent *raw = event.xcookie.data;
						struct xidevinfo *dev = XI2_GetDeviceInfo(raw->sourceid);
						double *val, *raw_val;
						double axis[2] = {0, 0};
						int i;
						if (raw->sourceid != raw->deviceid)
							return;	//ignore master devices to avoid dupes (we have our own device remapping stuff which should be slightly more friendly).
						if (dev->qdev == DEVID_UNSET)
							dev->qdev = xi2.nextqdev++;
						val = raw->valuators.values;
						raw_val = raw->raw_values;
						if (dev->abs)
							axis[0] = axis[1] = FLT_MIN;
						for (i = 0; i < raw->valuators.mask_len * 8; i++)
						{
							if (XIMaskIsSet(raw->valuators.mask, i))
							{
								if (i == 0) axis[0] = *raw_val;
								if (i == 1) axis[1] = *raw_val;
								val++;
								raw_val++;
							}
						}
						if (dev->abs)
						{	//tablets use weird 16bit coords or whatever. rescale to window coords (we have grabs, so offscreen stuff shouldn't matter).
							for (i = 0; i < 2; i++)
							{
								if (axis[i] == FLT_MIN)
									axis[i] = dev->axis[i].old;
								dev->axis[i].old = axis[i];
								axis[i] = (axis[i]-dev->axis[i].min) / (dev->axis[1].max+1-dev->axis[i].min);
								axis[i] *= i?vid.pixelheight:vid.pixelwidth;
							}
						}
						IN_MouseMove(dev->qdev, dev->abs, axis[0], axis[1], 0, 0);
					}
					break;
				default:
					Con_Printf("Unknown xinput event %u!\n", event.xcookie.evtype);
					break;
				}
			}
			else
				Con_Printf("Unknown generic event!\n");
		}
		x11.pXFreeEventData(vid_dpy, &event.xcookie);
		break;
	case ResizeRequest:
#ifdef VKQUAKE
		vk.neednewswapchain = true;
#endif
		vid.pixelwidth = event.xresizerequest.width;
		vid.pixelheight = event.xresizerequest.height;
		Cvar_ForceCallback(&vid_conautoscale);
//		if (fullscreenflags & FULLSCREEN_ACTIVE)
//			x11.pXMoveWindow(vid_dpy, vid_window, 0, 0);
		break;
	case ConfigureNotify:
		if (event.xconfigurerequest.window == vid_window)
		{
#ifdef VKQUAKE
			vk.neednewswapchain = true;
#endif
			vid.pixelwidth = event.xconfigurerequest.width;
			vid.pixelheight = event.xconfigurerequest.height;
			Cvar_ForceCallback(&vid_conautoscale);
		}
		else if (event.xconfigurerequest.window == vid_decoywindow)
		{
			if (!(fullscreenflags & FULLSCREEN_ACTIVE))
				x11.pXResizeWindow(vid_dpy, vid_window, event.xconfigurerequest.width, event.xconfigurerequest.height);
		}
//		if (fullscreenflags & FULLSCREEN_ACTIVE)
//			x11.pXMoveWindow(vid_dpy, vid_window, 0, 0);
		break;
	case KeyPress:
		X_KeyEvent(&event.xkey, true, filtered);
		break;
	case KeyRelease:
		if (x11.pXPending(vid_dpy))
		{	//autorepeat is messy - if the next event is a press event for the same key then ignore the release (we still get presses doing their autorepeat thing, just not the releases)
			XEvent nextev;
			x11.pXPeekEvent(vid_dpy, &nextev);	//blocks, so needs XPending
			if (nextev.type == KeyPress && nextev.xkey.time == event.xkey.time && nextev.xkey.keycode == event.xkey.keycode)
				break;
		}
		X_KeyEvent(&event.xkey, false, filtered);
		break;

	case MotionNotify:
		if (x11_input_method == XIM_DGA && mouse_grabbed)
		{
			IN_MouseMove(x11_mouseqdev, false, event.xmotion.x_root, event.xmotion.y_root, 0, 0);
		}
		else
		{
			if (mouse_grabbed)
			{
				if (x11_input_method != XIM_XI2)
				{
					int cx = vid.pixelwidth/2, cy=vid.pixelheight/2;

					IN_MouseMove(x11_mouseqdev, false, event.xmotion.x - cx, event.xmotion.y - cy, 0, 0);

					/* move the mouse to the window center again (disabling warp first so we don't see it*/
					x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask & ~PointerMotionMask);
					x11.pXWarpPointer(vid_dpy, None, vid_window, 0, 0, 0, 0,
						cx, cy);
					x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask);
				}
			}
			else
			{
				IN_MouseMove(x11_mouseqdev, true, event.xmotion.x, event.xmotion.y, 0, 0);
			}
		}
		break;

	case ButtonPress:
		if (x11_input_method == XIM_XI2 && mouse_grabbed)
			break;	//no dupes!
		b=-1;
		if (event.xbutton.button == 1)
			b = K_MOUSE1;
		else if (event.xbutton.button == 2)
			b = K_MOUSE3;
		else if (event.xbutton.button == 3)
			b = K_MOUSE2;
		//note, the x11 protocol does not support a mousewheel
		//we only support it because we follow convention. the actual protocol specifies 4+5 as regular buttons
		else if (event.xbutton.button == 4)
			b = x11violations?K_MWHEELUP:K_MOUSE4;
		else if (event.xbutton.button == 5)
			b = x11violations?K_MWHEELDOWN:K_MOUSE5;
		//note, the x11 protocol does not support more than 5 mouse buttons
		//which is a bit of a shame, but hey.
		else if (event.xbutton.button == 6)
			b = x11violations?K_MWHEELLEFT:-1;
		else if (event.xbutton.button == 7)
			b = x11violations?K_MWHEELRIGHT:-1;
		else if (event.xbutton.button == 8)
			b = x11violations?K_MOUSE4:-1;
		else if (event.xbutton.button == 9)
			b = x11violations?K_MOUSE5:-1;
		else if (event.xbutton.button == 10)
			b = x11violations?K_MOUSE6:-1;
		else if (event.xbutton.button == 11)
			b = x11violations?K_MOUSE7:-1;
		else if (event.xbutton.button == 12)
			b = x11violations?K_MOUSE8:-1;

		if (b>=0)
			IN_KeyEvent(x11_mouseqdev, true, b, 0);

/*
		if (fullscreenflags & FULLSCREEN_LEGACY)
		if (fullscreenflags & FULLSCREEN_VMODE)
		if (!vid.activeapp)
		{	//KDE doesn't seem to like us, in that you can't alt-tab back or click to activate.
			//This allows us to steal input focus back from the window manager
			x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime);
		}
*/
		break;

	case ButtonRelease:
		b=-1;
		if (event.xbutton.button == 1)
			b = K_MOUSE1;
		else if (event.xbutton.button == 2)
			b = K_MOUSE3;
		else if (event.xbutton.button == 3)
			b = K_MOUSE2;
		//note, the x11 protocol does not support a mousewheel
		//we only support it because we follow convention. the actual protocol specifies 4+5 as regular buttons
		else if (event.xbutton.button == 4)
			b = x11violations?K_MWHEELUP:K_MOUSE4;
		else if (event.xbutton.button == 5)
			b = x11violations?K_MWHEELDOWN:K_MOUSE5;
		//note, the x11 protocol does not support more than 5 mouse buttons
		//which is a bit of a shame, but hey.
		else if (event.xbutton.button == 6)
			b = x11violations?K_MWHEELLEFT:-1;
		else if (event.xbutton.button == 7)
			b = x11violations?K_MWHEELRIGHT:-1;
		else if (event.xbutton.button == 8)
			b = x11violations?K_MOUSE4:-1;
		else if (event.xbutton.button == 9)
			b = x11violations?K_MOUSE5:-1;
		else if (event.xbutton.button == 10)
			b = x11violations?K_MOUSE6:-1;
		else if (event.xbutton.button == 11)
			b = x11violations?K_MOUSE7:-1;
		else if (event.xbutton.button == 12)
			b = x11violations?K_MOUSE8:-1;

		if (b>=0)
			IN_KeyEvent(x11_mouseqdev, false, b, 0);
		break;

	case FocusIn:
		//don't care about it if its just someone wiggling the mouse
		if (event.xfocus.detail == NotifyPointer)
			break;
		//activeapp is if the game window is focused
		vid.activeapp = true;
		ClearAllStates();	//just in case.

		//but change modes to track the desktop window
//		if (!(fullscreenflags & FULLSCREEN_ACTIVE) || event.xfocus.window != vid_decoywindow)
		{
			modeswitchpending = 1;
			modeswitchtime = Sys_Milliseconds() + 1500;	/*fairly slow, to make sure*/
			UpdateGrabs();
		}

		if (event.xfocus.window == vid_window)
			x11.ime_shown = -1;

		//we we're focusing onto the game window and we're currently fullscreen, hide the other one so alt-tab won't select that instead of a real alternate app.
//		if ((fullscreenflags & FULLSCREEN_ACTIVE) && (fullscreenflags & FULLSCREEN_LEGACY) && event.xfocus.window == vid_window)
//			x11.pXUnmapWindow(vid_dpy, vid_decoywindow);
		break;
	case FocusOut:
		//don't care about it if its just someone wiggling the mouse
		if (event.xfocus.detail == NotifyPointer)
			break;
		//if we're already active, the decoy window shouldn't be focused anyway.
		if (event.xfocus.window == vid_window)
			x11.ime_shown = -1;

		if ((fullscreenflags & FULLSCREEN_ACTIVE) && event.xfocus.window == vid_decoywindow)
		{
			break;
		}

#ifdef USE_XRANDR
		if (xrandr.origgamma)
			xrandr.pSetCrtcGamma(vid_dpy, xrandr.crtc, xrandr.origgamma);
#endif
#ifdef USE_VMODE
		if (vm.originalapplied)
			vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, vm.originalrampsize, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
#endif

		mw = vid_window;
		if ((fullscreenflags & FULLSCREEN_LEGACY) && (fullscreenflags & FULLSCREEN_ACTIVE))
			mw = vid_decoywindow;

		if (event.xfocus.window == mw || event.xfocus.window == vid_window)
		{
			vid.activeapp = false;
			UpdateGrabs();
			ClearAllStates();
		}
		modeswitchpending = -1;
		modeswitchtime = Sys_Milliseconds() + 100;	/*fairly fast, so we don't unapply stuff when switching to other progs with delays*/
		break;
	case ClientMessage:
		{
			char *name = x11.pXGetAtomName(vid_dpy, event.xclient.message_type);
			if (!strcmp(name, "WM_PROTOCOLS") && event.xclient.format == 32)
			{
				char *protname = x11.pXGetAtomName(vid_dpy, event.xclient.data.l[0]);
				if (!strcmp(protname, "WM_DELETE_WINDOW"))
				{
					x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime); //make it easier to pick an option. FIXME: bring to top is a separate thing.
					M_Window_ClosePrompt();
				}
				else if (!strcmp(protname, "_NET_WM_PING"))
				{
					event.xclient.window = vid_root;
					x11.pXSendEvent(vid_dpy, vid_root, false, SubstructureNotifyMask|SubstructureRedirectMask, &event);
				}
				else
					Con_DPrintf("Got unknown x11wm message %s\n", protname);
				x11.pXFree(protname);
			}
#if 1
			else if (!strcmp(name, "XdndEnter") && event.xclient.format == 32)
			{
				//check for text/uri-list
				int i;
				x11.dnd.type = None;
				for (i = 2; i < 2+3; i++)
				{
					if (event.xclient.data.l[i])
					{
						char *t = x11.pXGetAtomName(vid_dpy, event.xclient.data.l[i]);
#ifdef XDS
						//direct-save has no way to deal with multiple files, other than lying about it.
						//which is unfortunately what other programs do. we have no real way to tell which file(s) actually got dragged/written.
						//we would need to report an empty directory, then scan for new files, then wipe the lot recursively.
						if (!strcmp(t, "XdndDirectSave0") && !x11.dnd.type)	//single file
							x11.dnd.type = event.xclient.data.l[i];
#endif
						if (!strcmp(t, "text/uri-list"))	//file list
							x11.dnd.type = event.xclient.data.l[i];
//						else if (!strcmp(t, "application/octet-stream"))	//raw file data without a name.
//							x11.dnd.type = event.xclient.data.l[i];
						x11.pXFree(t);
					}
				}
			}
			else if (!strcmp(name, "XdndPosition") && event.xclient.format == 32)
			{
				//Send XdndStatus
				XEvent xev;
				memset(&xev, 0, sizeof(xev));
				xev.type = ClientMessage;
				xev.xclient.window = event.xclient.data.l[0];
				xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "XdndStatus", False);
				xev.xclient.format = 32;
				xev.xclient.data.l[0] = vid_window;	//so source can ignore it if stale
				xev.xclient.data.l[1] = x11.dnd.type?1:0;
				xev.xclient.data.l[2] = 0;	//(x<<16)|y (should be in root coords)
				xev.xclient.data.l[3] = 0;	//(w<<16)|h
				xev.xclient.data.l[4] = x11.pXInternAtom (vid_dpy, "XdndActionCopy", False);
				x11.pXSendEvent(vid_dpy, xev.xclient.window, False, 0, &xev);
			}
			else if (!strcmp(name, "XdndLeave") && event.xclient.format == 32)
			{
				if (x11.dnd.source == event.xclient.data.l[0])
				{
					x11.dnd.source = None;
					x11.dnd.type = None;
				}
			}
			else if (!strcmp(name, "XdndDrop") && event.xclient.format == 32)
			{
				Atom xa_XdndSelection = x11.pXInternAtom(vid_dpy, "XdndSelection", False);
				Window source = event.xclient.data.l[0];
				Time t = CurrentTime;//event.xclient.data.l[2];

#ifdef XDS
				char *droptype = x11.dnd.type?x11.pXGetAtomName(vid_dpy, x11.dnd.type):NULL;
				if (droptype && !strcmp(droptype, "XdndDirectSave0"))	//single file
				{
					unsigned char *data = NULL;
					Atom type;
					int fmt;
					unsigned long nitems;
					unsigned long bytesleft;
					if (x11.pXGetWindowProperty(vid_dpy, source, x11.dnd.type, 0, 65536, False, AnyPropertyType, &type, &fmt, &nitems, &bytesleft, &data) == Success && data)
					{
						char hostname[1024];
						if (gethostname(hostname, sizeof(hostname)) < 0)
							*hostname = 0;	//failed? o.O
						hostname[sizeof(hostname)-1] = 0;
						char *path = va("file://%s/tmp/%s", hostname, data);
						Atom proptype = x11.pXInternAtom(vid_dpy, "text/plain", false);
						x11.pXChangeProperty(vid_dpy, source, x11.dnd.type, proptype, 8, PropModeReplace, (void*)path, strlen(path));
						Con_Printf("Dropping file %s\n", data);
						x11.pXFree(data);
					}
				}
				x11.pXFree(droptype);
#endif

				x11.dnd.myprop = x11.pXInternAtom(vid_dpy, "_FTE_dnd", False);
				if (x11.pXGetSelectionOwner(vid_dpy, xa_XdndSelection) == source)
				{
					x11.pXDeleteProperty(vid_dpy, vid_window, x11.dnd.myprop);
					x11.pXConvertSelection(vid_dpy, xa_XdndSelection, x11.dnd.type, x11.dnd.myprop, vid_window, t);
				}
			}
#endif
			else
				Con_DPrintf("Got unknown x11 message %s\n", name);
			x11.pXFree(name);
		}
		break;
#if 1
	case SelectionNotify:
		//for paste
		if (X11_Clipboard_Notify(&event.xselection))
			;
		//for drag-n-drop
		else if (event.xselection.selection == x11.pXInternAtom(vid_dpy, "XdndSelection", False) && x11.dnd.myprop != None)
		{
			qboolean okay = false;
			unsigned char *data;
			Atom type;
			int fmt;
			unsigned long nitems;
			unsigned long bytesleft;
			if (x11.pXGetWindowProperty(vid_dpy, vid_window, x11.dnd.myprop, 0, 65536, False, AnyPropertyType, &type, &fmt, &nitems, &bytesleft, &data) == Success && data)
			{
				char *tname = x11.pXGetAtomName(vid_dpy, x11.dnd.type);
				if (type == x11.dnd.type && !strcmp(tname, "text/uri-list"))
				{
					char *start, *end;
					for (start = data; *start; )
					{
						for (end = start; *end && *end != '\r'; end++)
							;
						if (end != start)
							Host_RunFile(start, end-start, NULL);
						start = end;
						while (*start == '\r' || *start == '\n')
							start++;
					}
					okay = true;
				}
#ifdef XDS
				else if (type == x11.dnd.type && !strcmp(tname, "XdndDirectSave0") && nitems == 1)
				{
					switch(data[0])
					{
					case 'S':	//sender wrote the file
					case 'E':	//sender failed to generate the data or something
					case 'F':	//sender failed to write the file. we should use application/octet-stream and write it ourself.
					}
				}
#endif
				x11.pXFree(tname);
				x11.pXFree(data);
			}
			x11.pXDeleteProperty(vid_dpy, vid_window, x11.dnd.myprop);	//might be large, so don't force it to hang around.

			//Send XdndFinished now
			{
				XEvent xev;
				memset(&xev, 0, sizeof(xev));
				xev.type = ClientMessage;
				xev.xclient.window = x11.dnd.source;
				xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "XdndFinished", False);
				xev.xclient.format = 32;
				xev.xclient.data.l[0] = vid_window;	//so source can ignore it if stale
				xev.xclient.data.l[1] = (okay?1:0);
				xev.xclient.data.l[2] = x11.pXInternAtom (vid_dpy, "XdndActionCopy", False);
				x11.pXSendEvent(vid_dpy, xev.xclient.window, False, 0, &xev);
			}
		}
		break;

	case SelectionRequest:	//needed for when another program tries pasting.
		{
			Atom xa_u8string = x11.pXInternAtom(vid_dpy, "UTF8_STRING", false);	//explicitly UTF-8
			Atom xa_l1string = x11.pXInternAtom(vid_dpy, "STRING", false);	//explicitly 8859-1
			Atom xa_text = x11.pXInternAtom(vid_dpy, "TEXT", false);	//selection owner decides encoding (and we pick UTF-8)
			Atom xa_targets = x11.pXInternAtom(vid_dpy, "TARGETS", false);
			Atom xa_supportedtargets[] = {xa_u8string, xa_l1string, xa_text, xa_targets/*, xa_multiple, xa_timestamp*/};
			char *cliptext = NULL;
			memset(&rep, 0, sizeof(rep));

			if (event.xselectionrequest.selection == x11.pXInternAtom(vid_dpy, "PRIMARY", false))
				cliptext = clipboard_buffer[CBT_SELECTION];
			else if (event.xselectionrequest.selection == x11.pXInternAtom(vid_dpy, "CLIPBOARD", false))
				cliptext = clipboard_buffer[CBT_CLIPBOARD];
			if (!cliptext)	//err, nothing in the clipboard buffer... that's not meant to happen.
				cliptext = "";

			if (event.xselectionrequest.property == None)	//no property sets a property matching the target atom, as a fallback.
				event.xselectionrequest.property = event.xselectionrequest.target;
			if (event.xselectionrequest.property == None)
				event.xselectionrequest.property = x11.pXInternAtom(vid_dpy, "foobar2000", false);
			if (event.xselectionrequest.property != None && event.xselectionrequest.target == xa_targets)
			{	//TARGETS results in a list of accepted target types (atoms)
				x11.pXChangeProperty(vid_dpy, event.xselectionrequest.requestor, event.xselectionrequest.property, event.xselectionrequest.target, 32, PropModeReplace, (void*)xa_supportedtargets, countof(xa_supportedtargets));
				rep.xselection.property = event.xselectionrequest.property;
			}
			else if (event.xselectionrequest.property != None && (event.xselectionrequest.target == xa_u8string || event.xselectionrequest.target == xa_text))
			{	//UTF8_STRING or TEXT (which we choose to use utf-8 as our charset)
				x11.pXChangeProperty(vid_dpy, event.xselectionrequest.requestor, event.xselectionrequest.property, event.xselectionrequest.target, 8, PropModeReplace, (void*)cliptext, strlen(cliptext));
				rep.xselection.property = event.xselectionrequest.property;
			}
			else if (event.xselectionrequest.property != None && event.xselectionrequest.target == xa_l1string)
			{	//STRING == latin1. convert as needed.
				char *latin1 = alloca(strlen(cliptext)+1);	//may shorten
				const char *in = cliptext;
				int c = 0;
				int err;
				while (*in && c < sizeof(latin1))
				{
					int uc = utf8_decode(&err, in, &in);
					if ((uc >= 0xe000 && uc <= 0xe100) && (uc&0x7f) >= 32)
						uc = uc&0x7f;	//don't do c0/c1 glyphs. otherwise treat as ascii.
					else if (uc > 255 || err)
						uc = '?';	//unsupported char
					latin1[c++] = uc;
				}
				x11.pXChangeProperty(vid_dpy, event.xselectionrequest.requestor, event.xselectionrequest.property, event.xselectionrequest.target, 8, PropModeReplace, (void*)latin1, c);
				rep.xselection.property = event.xselectionrequest.property;
			}
			else
			{	//unsupported target. we need to let them know that we don't know what they're asking for.
				rep.xselection.property = None;
			}
			rep.xselection.type = SelectionNotify;
			rep.xselection.serial = 0;
			rep.xselection.send_event = true;
			rep.xselection.display = rep.xselection.display;
			rep.xselection.requestor = event.xselectionrequest.requestor;
			rep.xselection.selection = event.xselectionrequest.selection;
			rep.xselection.target = event.xselectionrequest.target;
			rep.xselection.time = event.xselectionrequest.time;
			x11.pXSendEvent(vid_dpy, event.xselectionrequest.requestor, 0, 0, &rep);
		}
		break;
#endif

	default:
//		Con_Printf("%x\n", event.type);
		break;
	}
}


static void GLVID_Shutdown(void)
{
	if (!vid_dpy)
		return;
	vid_activecursor = None;
	vid_newcursor = None;
	x11.pXUngrabKeyboard(vid_dpy, CurrentTime);
	uninstall_grabs();

#ifdef USE_VMODE
	if (vm.originalapplied)
		vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, vm.originalrampsize, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
#endif

	X_ShutdownUnicode();

	switch(currentpsl)
	{
#ifdef GLQUAKE
#ifdef USE_EGL
	case PSL_EGL:
		EGL_Shutdown();
		GL_ForgetPointers();
		break;
#endif
	case PSL_GLX:
		if (ctx)
		{
			glx.DestroyContext(vid_dpy, ctx);
			ctx = NULL;
		}
		GL_ForgetPointers();
		break;
#endif
#ifdef VKQUAKE
	case PSL_VULKAN:
		VK_Shutdown();
		break;
#endif
	case PSL_NONE:
		break;
	}

#ifdef USE_XRANDR
	XRandR_Shutdown();
#endif

	if (vid_window)
		x11.pXDestroyWindow(vid_dpy, vid_window);
	if (vid_decoywindow)
		x11.pXDestroyWindow(vid_dpy, vid_decoywindow);
	if (vid_nullcursor)
		x11.pXFreeCursor(vid_dpy, vid_nullcursor);
	if (vid_dpy)
	{
#ifdef USE_VMODE
		if (fullscreenflags & FULLSCREEN_VMODEACTIVE)
			vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[0]);
		fullscreenflags &= ~FULLSCREEN_VMODEACTIVE;

		if (vm.modes)
			x11.pXFree(vm.modes);
		vm.modes = NULL;
		vm.num_modes = 0;
#endif
		x11.pXCloseDisplay(vid_dpy);
	}
	vid_dpy = NULL;
	vid_window = (Window)NULL;

	switch(currentpsl)
	{
#ifdef GLQUAKE
#ifdef USE_EGL
	case PSL_EGL:
		EGL_UnloadLibrary();
		break;
#endif
#endif
	default:
		break;
	}

	currentpsl = PSL_NONE;
}

void GLVID_DeInit(void)	//FIXME:....
{
	GLVID_Shutdown();
}

static Cursor CreateNullCursor(Display *display, Window root)
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour = {0};
	Cursor cursor;

	cursormask = x11.pXCreatePixmap(display, root, 1, 1, 1/*depth*/);
	xgc.function = GXclear;
	gc =  x11.pXCreateGC(display, cursormask, GCFunction, &xgc);
	x11.pXFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	dummycolour.flags = 04;
	cursor = x11.pXCreatePixmapCursor(display, cursormask, cursormask,
		&dummycolour,&dummycolour, 0,0);
	x11.pXFreePixmap(display,cursormask);
	x11.pXFreeGC(display,gc);
	return cursor;
}

#ifndef NO_X11_CURSOR
#include <X11/Xcursor/Xcursor.h>
static struct
{
	void *lib;

	XcursorBool (*SupportsARGB) (Display *dpy);
	XcursorImage *(*ImageCreate) (int width, int height);
	Cursor (*ImageLoadCursor) (Display *dpy, const XcursorImage *image);
	void (*ImageDestroy) (XcursorImage *image);
} xcursor;
static void *X11VID_CreateCursorRGBA(const qbyte *rgbacursor, uploadfmt_t format, size_t w, size_t h, float hotx, float hoty)
{
	Cursor *cursor;
	size_t x, y;
	XcursorImage *img;
	XcursorPixel *dest;

	img = xcursor.ImageCreate(w, h);
	img->xhot = hotx;
	img->yhot = hoty;
	dest = img->pixels;

	switch (format)
	{
	case PTI_BGRA8:
	case PTI_BGRX8:
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++, rgbacursor+=4)
				*dest++ = (rgbacursor[3]<<24)|(rgbacursor[2]<<16)|(rgbacursor[1]<<8)|(rgbacursor[0]<<0);	//0xARGB
		break;	//supported...
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_LLLA8:
	case PTI_LLLX8:
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++, rgbacursor+=4)
				*dest++ = (rgbacursor[3]<<24)|(rgbacursor[0]<<16)|(rgbacursor[1]<<8)|(rgbacursor[2]<<0);	//0xARGB
		break;
	default:
		//panic... format wasn't supported. I hope we didn't spend ages resampling it...
		xcursor.ImageDestroy(img);
		return NULL;
	}

	cursor = Z_Malloc(sizeof(*cursor));
	*cursor = xcursor.ImageLoadCursor(vid_dpy, img);
	xcursor.ImageDestroy(img);
	if (*cursor)
		return (void*)cursor;
	Z_Free(cursor);
	return NULL;
}
static void *X11VID_CreateCursor(const qbyte *imagedata, int width, int height, uploadfmt_t format, float hotx, float hoty, float scale)
{
	void *r;
	if (!imagedata)
		return NULL;

	if (scale != 1)
	{
		int nw,nh;
		qbyte *nd;
		for(;;)
		{
			nw = width * scale;
			nh = height * scale;
			if (nw <= 0 || nh <= 0 || nw > 128 || nh > 128) //don't go crazy.
				return NULL;
			if (nw < 8)
				scale = 8/width;
			else if (nh < 8)
				scale = 8/height;
			else
				break;
		}
		nd = Image_ResampleTexture(format, imagedata, width, height, NULL, nw, nh);
		if (!nd)
			return NULL;	//resampling of that format didn't work for some reason...
		width = nw;
		height = nh;
		r = X11VID_CreateCursorRGBA(nd, format, width, height, hotx, hoty);
		BZ_Free(nd);
	}
	else
		r = X11VID_CreateCursorRGBA(imagedata, format, width, height, hotx, hoty);

	return r;
}
static void X11VID_DestroyCursor(void *qcursor)
{
	Cursor c = *(Cursor*)qcursor;
	if (vid_newcursor == c)
		vid_newcursor = None;
	if (vid_dpy)
		x11.pXFreeCursor(vid_dpy, c);
	Z_Free(qcursor);
}
static qboolean X11VID_SetCursor(void *qcursor)
{
	if (qcursor)
		vid_newcursor = *(Cursor*)qcursor;
	else
		vid_newcursor = None;

	if (vid_dpy)
		UpdateGrabs();
	return true;
}
static qboolean XCursor_Init(void)
{
	static dllfunction_t xcursor_functable[] =
	{
		{(void**)&xcursor.SupportsARGB,		"XcursorSupportsARGB"},
		{(void**)&xcursor.ImageCreate,		"XcursorImageCreate"},
		{(void**)&xcursor.ImageLoadCursor,	"XcursorImageLoadCursor"},
		{(void**)&xcursor.ImageDestroy,		"XcursorImageDestroy"},
		{NULL, NULL}
	};
	qboolean defaulthwcursor = true;

#ifdef GLQUAKE
	if (qrenderer == QR_OPENGL && !strcmp(gl_vendor, "Humper") && !strcmp(gl_renderer, "Chromium"))
	{	//I don't really understand the significance of these two values, but we get them when running inside VirtualBox
		//in such cases, the opengl window has a nasty habit of appearing top-most, even above mouse cursors...
		//so we NEED to disable hardware cursors by default in this case, because otherwise we won't see any
		//(the cursor should be visible with mouse integration enabled, but then XWarpPointer fails without error. unplayable both ways to some extent)
		Con_Printf("VirtualBox Detected: OpenGL obscures hardware cursors. seta x11_allow_xcursor 1 to ignore.\n");
		defaulthwcursor = false;
	}
#endif

	//in case they were previously set...
	rf->VID_CreateCursor = NULL;
	rf->VID_DestroyCursor = NULL;
	rf->VID_SetCursor = NULL;

	if (!X11_CheckFeature("xcursor", defaulthwcursor))
	{
		Con_Printf("Hardware cursors disabled.\n");
		return false;
	}

	if (!xcursor.lib)
	{
#ifdef __CYGWIN__
		if (!xcursor.lib)
			xcursor.lib = Sys_LoadLibrary("cygXcursor.dll", xcursor_functable);
#endif
		if (!xcursor.lib)
			xcursor.lib = Sys_LoadLibrary("libXcursor.so.1", xcursor_functable);
		if (!xcursor.lib)
			xcursor.lib = Sys_LoadLibrary("libXcursor.so", xcursor_functable);
		if (!xcursor.lib)
			Con_Printf("Xcursor library not available or too old.\n");
	}
	if (xcursor.lib)
	{
		if (xcursor.SupportsARGB(vid_dpy))
		{	//okay, we should be able to use argb hardware cursors, so set up our public function pointers
			rf->VID_CreateCursor = X11VID_CreateCursor;
			rf->VID_DestroyCursor = X11VID_DestroyCursor;
			rf->VID_SetCursor = X11VID_SetCursor;
			return true;
		}
	}
	Con_Printf("Hardware cursors unsupported.\n");
	return false;
}
#else
static qboolean XCursor_Init(void)
{
	return false;
}
#endif

qboolean GLVID_ApplyGammaRamps(unsigned int rampcount, unsigned short *ramps)
{
	extern qboolean gammaworks;

	if (ramps)
	{
		switch(vid_hardwaregamma.ival)
		{
		case 0:	//never use hardware/glsl gamma
		case 2:	//ALWAYS use glsl gamma
			return false;
		default:
		case 1:	//no hardware gamma when windowed
			if (!(fullscreenflags & FULLSCREEN_ACTIVE))
				return false;
			break;
		case 3:	//ALWAYS try to use hardware gamma, even when it fails...
			break;
		}
	}

#ifdef USE_XRANDR
	if (xrandr.origgamma)
	{	//we favour xrandr - xf86 gamma seems to cache old values which screws up gamma after having previously quit ezquake.
		if (ramps && rampcount == xrandr.origgamma->size)
		{
			XRRCrtcGamma g;
			g.size = rampcount;
			g.red = &ramps[0];
			g.green = &ramps[rampcount];
			g.blue = &ramps[rampcount*2];
			if (vid.activeapp)
			{
				if (gammaworks)
					xrandr.pSetCrtcGamma(vid_dpy, xrandr.crtc, &g);
				gammaworks = true;
				return gammaworks;
			}
			return false;
		}
		else if (gammaworks)
		{
			xrandr.pSetCrtcGamma(vid_dpy, xrandr.crtc, xrandr.origgamma);
			return true;
		}
	}
#endif

#ifdef USE_VMODE
	//if we don't know the original ramps yet, don't allow changing them, because we're probably invalid anyway, and even if it worked, it'll break something later.
	if (vm.originalapplied)
	{

		if (ramps && rampcount == vm.originalrampsize)
		{
			if (vid.activeapp)
			{
				if (gammaworks)
					vm.pXF86VidModeSetGammaRamp (vid_dpy, scrnum, rampcount, &ramps[0], &ramps[rampcount], &ramps[rampcount*2]);
				else
					gammaworks = !!vm.pXF86VidModeSetGammaRamp (vid_dpy, scrnum, rampcount, &ramps[0], &ramps[rampcount], &ramps[rampcount*2]);
				return gammaworks;
			}
			return false;
		}
		else if (gammaworks)
		{
			vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, vm.originalrampsize, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
			return true;
		}
	}
#endif
	return false;
}

void GLVID_SwapBuffers (void)
{
	switch(currentpsl)
	{
#ifdef GLQUAKE
#ifdef USE_EGL
	case PSL_EGL:
		EGL_SwapBuffers();
		break;
#endif
	case PSL_GLX:
		{
			int n = vid_vsync.ival;
			if (cls.timedemo && cls.demoplayback)
				n = 0;
			if (!glx.swaptear)
				n = abs(n);
			if (glx.swapint != n)
			{
				glx.swapint = n;
				if (glx.SwapIntervalEXT)
				{
					glx.SwapIntervalEXT(vid_dpy, vid_window, glx.swapint);
					Con_DPrintf("Swap interval changed to %i\n", glx.swapint);
				}
				else if (glx.SwapIntervalMESA && glx.swapint>=0)
				{
					glx.SwapIntervalMESA(glx.swapint);
					Con_DPrintf("Swap interval changed to %i\n", glx.swapint);
				}
				else if (glx.SwapIntervalSGI && glx.swapint>0)
				{
					glx.SwapIntervalSGI(glx.swapint);
					Con_DPrintf("Swap interval changed to %i\n", glx.swapint);
				}
				else
					Con_Printf("Unable to change swap interval to %i\n", glx.swapint);
			}
		}

		//we don't need to flush, XSawpBuffers does it for us.
		//chances are, it's version is more suitable anyway. At least there's the chance that it might be.
		glx.SwapBuffers(vid_dpy, vid_window);
		break;
#endif
#ifdef VKQUAKE
	case PSL_VULKAN:
		break;
#endif
	default:
	case PSL_NONE:
		break;
	}
}

#include "fte_eukara64.h"
//#include "bymorphed.h"
static void X_StoreIcon(Window wnd)
{
	int i;
	Atom propname = x11.pXInternAtom(vid_dpy, "_NET_WM_ICON", false);
	Atom proptype = x11.pXInternAtom(vid_dpy, "CARDINAL", false);

	size_t filesize = 0;
	qbyte *filedata = NULL;
#ifdef IMAGEFMT_PNG
	if (!filedata)
		filedata = FS_LoadMallocFile("icon.png", &filesize);
#endif
	if (!filedata)
		filedata = FS_LoadMallocFile("icon.tga", &filesize);
#ifdef IMAGEFMT_JPG
	if (!filedata)
		filedata = FS_LoadMallocFile("icon.jpg", &filesize);
#endif
#ifdef IMAGEFMT_BMP
	if (!filedata)
		filedata = FS_LoadMallocFile("icon.ico", &filesize);
#endif
#ifdef HAVE_LEGACY
	if (!filedata)
		filedata = FS_LoadMallocFile("darkplaces-icon.tga", &filesize);
#endif
	if (filedata)
	{
		int imagewidth, imageheight;
		unsigned long *iconblob;	//yes, long, even on 64bit machines. and even when we claim it to be 32bit. xlib legacy cruft that'll get stripped...
		uploadfmt_t format;
		qbyte *imagedata = ReadRawImageFile(filedata, filesize, &imagewidth, &imageheight, &format, true, "icon.png");
		Z_Free(filedata);

		if (imagedata)
		{
			iconblob = BZ_Malloc(sizeof(*iconblob)*(2+imagewidth*imageheight));
			iconblob[0] = imagewidth;
			iconblob[1] = imageheight;
			//needs to be 0xARGB, rather than RGBA bytes
			for (i = 0; i < imagewidth*imageheight; i++)
				iconblob[i+2] = (imagedata[i*4+3]<<24) | (imagedata[i*4+0]<<16) | (imagedata[i*4+1]<<8) | (imagedata[i*4+2]<<0);
			Z_Free(imagedata);

			x11.pXChangeProperty(vid_dpy, wnd, propname, proptype, 32, PropModeReplace, (void*)iconblob, 2+imagewidth*imageheight);
			BZ_Free(iconblob);
			return;
		}
	}

	{
		//fall back to the embedded icon.
		unsigned long data[64*64+2];
		data[0] = icon.width;
		data[1] = icon.height;

		/* GIMP exports dumps as RGBA only, so we have to convert them too - eukara */
		for (i = 0; i < data[0]*data[1]; i++)
			data[i+2] = (icon.pixel_data[i*4+3]<<24) | (icon.pixel_data[i*4+0]<<16) | (icon.pixel_data[i*4+1]<<8) | (icon.pixel_data[i*4+2]<<0);

		x11.pXChangeProperty(vid_dpy, wnd, propname, proptype, 32, PropModeReplace, (void*)data, data[0]*data[1]+2);
	}
}

static void X_StorePID(Window wnd)
{
	Atom net_wm_pid  = x11.pXInternAtom(vid_dpy, "_NET_WM_PID", false);
	Atom wm_client_machine = x11.pXInternAtom(vid_dpy, "WM_CLIENT_MACHINE", false);
	Atom cardinal = x11.pXInternAtom(vid_dpy, "CARDINAL", false);
	Atom string = x11.pXInternAtom(vid_dpy, "STRING", false);
	long pid = getpid();
	#ifndef HOST_NAME_MAX
	#define HOST_NAME_MAX 255
	#endif
	char hostname[HOST_NAME_MAX+1];
	*hostname = 0;
	if (gethostname(hostname, sizeof(hostname)) < 0)
		return;	//just give up. if the hostname isn't valid then the pid won't be useful anyway.
	hostname[countof(hostname)-1] = 0;	//make sure its null terminated... *sigh*

	x11.pXChangeProperty(vid_dpy, wnd, wm_client_machine, string, 8, PropModeReplace, hostname, strlen(hostname));
	x11.pXChangeProperty(vid_dpy, wnd, net_wm_pid, cardinal, 32, PropModeReplace, (void*)&pid, 1);
}

void X_GoFullscreen(void)
{
	XEvent xev;
	
	//for NETWM window managers
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = vid_window;
	xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;	//add
	xev.xclient.data.l[1] = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xev.xclient.data.l[2] = 0;

	//for any other window managers, and broken NETWM
	//Note that certain window managers ignore this entirely, and instead fullscreen us on whichever monitor has the mouse cursor at the time. Which is awkward.
	x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx, fullscreeny, fullscreenwidth, fullscreenheight);
//	x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx+fullscreenwidth/2, fullscreeny+fullscreenheight/2, 1, 1);
	x11.pXSync(vid_dpy, False);
	x11.pXSendEvent(vid_dpy, DefaultRootWindow(vid_dpy), False, SubstructureNotifyMask, &xev);
	x11.pXSync(vid_dpy, False);
	x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx, fullscreeny, fullscreenwidth, fullscreenheight);
	x11.pXSync(vid_dpy, False);

//Con_Printf("Gone fullscreen\n");
}
void X_GoWindowed(void)
{
	XEvent xev;
	x11.pXFlush(vid_dpy);
	x11.pXSync(vid_dpy, False);

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = vid_window;
	xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 0;	//remove
	xev.xclient.data.l[1] = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xev.xclient.data.l[2] = 0;
	x11.pXSendEvent(vid_dpy, DefaultRootWindow(vid_dpy), False, SubstructureNotifyMask, &xev);
	x11.pXSync(vid_dpy, False);

	x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, 640, 480);
//Con_Printf("Gone windowed\n");
}
static qboolean X_CheckWMFullscreenAvailable(void)
{
	//root window must have _NET_SUPPORTING_WM_CHECK which is a Window created by the WM
	//the WM's window must have _NET_WM_NAME set, which is the name of the window manager
	//if we can find those, then the window manager has not crashed.
	//if we can then find _NET_WM_STATE_FULLSCREEN in the _NET_SUPPORTED atom list on the root, then we can get fullscreen mode from the WM
	//and we'll have no alt-tab issues whatsoever.

	Atom xa_net_supporting_wm_check = x11.pXInternAtom(vid_dpy, "_NET_SUPPORTING_WM_CHECK", False);
	Atom xa_net_wm_name = x11.pXInternAtom(vid_dpy, "_NET_WM_NAME", False);
	Atom xa_net_supported = x11.pXInternAtom(vid_dpy, "_NET_SUPPORTED", False);
	Atom xa_net_wm_state_fullscreen = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	Window wmwindow;
	unsigned char *prop;
	unsigned long bytes_after, nitems;
	Atom type;
	int format;
	qboolean success = false;
	unsigned char *wmname;
	int i;

	if (!X11_CheckFeature("wmfullscreen", true))
	{
		Con_Printf("Window manager fullscreen support disabled. Will attempt to hide from it instead.\n");
		return success;
	}
	

	if (x11.pXGetWindowProperty(vid_dpy, vid_root, xa_net_supporting_wm_check, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &prop) != Success || prop == NULL)
	{
		Con_Printf("Window manager not identified\n");
		return success;
	}
	wmwindow = *(Window *)prop;
	x11.pXFree(prop);
	
	if (x11.pXGetWindowProperty(vid_dpy, wmwindow, xa_net_wm_name, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &wmname) != Success || wmname == NULL)
	{
		Con_Printf("Window manager crashed or something\n");
		return success;
	}
	else
	{
		if (x11.pXGetWindowProperty(vid_dpy, vid_root, xa_net_supported, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &prop) != Success || prop == NULL)
		{
			Con_Printf("Window manager \"%s\" supports nothing\n", wmname);
		}
		else
		{
			for (i = 0; i < nitems; i++)
			{
//				Con_Printf("supported: %s\n", x11.pXGetAtomName(vid_dpy, ((Atom*)prop)[i]));
				if (((Atom*)prop)[i] == xa_net_wm_state_fullscreen)
				{
					success = true;
					break;
				}
			}
			if (!success)
				Con_Printf("Window manager \"%s\" does not appear to support fullscreen\n", wmname);
			else if (!strcmp(wmname, "Fluxbox"))
			{
				Con_Printf("Window manager \"%s\" claims to support fullscreen, but is known to be buggy\n", wmname);
				success = false;
			}
			else
				Con_DPrintf("Window manager \"%s\" supports fullscreen. Set x11_allow_wmfullscreen 0 if it is buggy.\n", wmname);
			x11.pXFree(prop);
		}
		x11.pXFree(wmname);
	}
	return success;
}

static Window X_CreateWindow(rendererstate_t *info, qboolean override, XVisualInfo *visinfo, int x, int y, unsigned int width, unsigned int height, qboolean fullscreen)
{
	Window wnd, parent;
	XSetWindowAttributes attr;
	XSizeHints szhints;
	unsigned int mask;
	Atom prots[2];
	extern cvar_t vid_minsize;

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = x11.pXCreateColormap(vid_dpy, vid_root, visinfo->visual, AllocNone);
	attr.event_mask = vid_x_eventmask = X_MASK;
	attr.backing_store = NotUseful;
	attr.save_under = False;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWBackingStore |CWSaveUnder;

	// override redirect prevents the windowmanager from finding out about us, and thus will not apply borders to our window.
	if (override)
	{
		mask |= CWOverrideRedirect;
		attr.override_redirect = True;
	}

	memset(&szhints, 0, sizeof(szhints));
	szhints.flags = PMinSize|PPosition|PSize;
	szhints.min_width = max(vid_minsize.vec4[0], 320);
	szhints.min_height = max(vid_minsize.vec4[1], 200);

	if (!fullscreen)
	{
#ifdef USE_XRANDR
		int dx, dy, dw, dh;
		if (*info->devicename && XRandr_PickScreen(info->devicename, &dx, &dy, &dw, &dh))
		{
			x += dx + (dw-width)/2;
			y += dy + (dh-height)/2;
		}
		else
#endif
			szhints.flags &= ~PPosition;
	}

	if (sys_parentwindow && !fullscreen)
	{
		x = (sys_parentwidth - width) / 2;
		y = (sys_parentheight - height) / 2;
		parent = sys_parentwindow;
	}
	else
		parent = vid_root;

	szhints.x = x;
	szhints.y = y;
	szhints.width = width;
	szhints.height = height;

	wnd = x11.pXCreateWindow(vid_dpy, parent, x, y, width, height,
						0, visinfo->depth, InputOutput,
						visinfo->visual, mask, &attr);
	/*ask the window manager to stop triggering bugs in Xlib*/
	prots[0] = x11.pXInternAtom(vid_dpy, "WM_DELETE_WINDOW", False);
	prots[1] = x11.pXInternAtom(vid_dpy, "_NET_WM_PING", False);
	x11.pXSetWMProtocols(vid_dpy, wnd, prots, countof(prots));
	x11.pXSetWMNormalHints(vid_dpy, wnd, &szhints);
	/*set caption*/
	x11.pXStoreName(vid_dpy, wnd, "FTE QuakeWorld");
	x11.pXSetIconName(vid_dpy, wnd, "FTEQW");
	X_StoreIcon(wnd);
	X_StorePID(wnd);
	/*make it visible*/
	x11.pXMapWindow(vid_dpy, wnd);

	//advertise support as a drag+drop target
	prots[0] = 5;	//version 5 is the most recent.
	x11.pXChangeProperty(vid_dpy, wnd, x11.pXInternAtom(vid_dpy, "XdndAware", False), XA_ATOM, 32, PropModeReplace, (void*)prots, 1);

	return wnd;
}

static qboolean X11VID_Init (rendererstate_t *info, unsigned char *palette, int psl)
{
	int x = 0;
	int y = 0;
	int width = info->width;	//can override these if vmode isn't available
	int height = info->height;
	int rate = info->rate;
#if defined(USE_EGL)
	EGLConfig eglcfg = 0;
#endif
#if defined(USE_EGL) || defined(VKQUAKE)
	XVisualInfo vinfodef;
#endif
	XVisualInfo *visinfo;
#ifdef GLQUAKE
	GLXFBConfig fbconfig = NULL;
#endif
	qboolean fullscreen = false;

	if (!x11_initlib())
		return false;

	if (info->fullscreen)
		fullscreen = true;

	currentpsl = psl;

	switch(currentpsl)
	{
#ifdef GLQUAKE
#ifdef USE_EGL
	case PSL_EGL:
		if (!EGL_LoadLibrary(info->subrenderer))
		{
			Con_Printf("couldn't load EGL library\n");
			return false;
		}
		break;
#endif
	case PSL_GLX:
		if (!GLX_InitLibrary(info->subrenderer))
		{
			Con_Printf("Couldn't intialise GLX\nEither your drivers are not installed or you need to specify the library name with the gl_driver cvar\n");
			return false;
		}
		break;
#endif
#ifdef VKQUAKE
	case PSL_VULKAN:
		{
			dllfunction_t func[] =
			{
				{(void*)&vkGetInstanceProcAddr,		"vkGetInstanceProcAddr"},
				{NULL,							NULL}
			};

			if (!Sys_LoadLibrary("libvulkan.so.1", func))
			{
				if (!Sys_LoadLibrary("libvulkan.so", func))
				{
					Con_Printf(CON_ERROR"Couldn't load libvulkan.so\nvulkan loader is not installed\n");
					return false;
				}
			}

			//"Some implementations may require threads to implement some presentation modes so applications must call XInitThreads() before calling any other Xlib functions."
			x11.pXInitThreads();
		}
		break;
#endif
	case PSL_NONE:
		return false;
	}

	if (!vid_dpy)
		vid_dpy = x11.pXOpenDisplay(NULL);
	if (!vid_dpy)
	{
		Con_Printf(CON_ERROR "Error: couldn't open the X display\n");
		return false;
	}

	scrnum = DefaultScreen(vid_dpy);
	vid_root = RootWindow(vid_dpy, scrnum);

#define MILLIMETRESPERINCH 25.4	//sigh, why did we go for dpi?
	vid.dpi_x = DisplayWidth(vid_dpy, scrnum) * (MILLIMETRESPERINCH / DisplayWidthMM(vid_dpy, scrnum));
	vid.dpi_y = DisplayHeight(vid_dpy, scrnum) * (MILLIMETRESPERINCH / DisplayHeightMM(vid_dpy, scrnum));

	fullscreenflags = 0;

	if (info->fullscreen == 2)
		fullscreenflags |= FULLSCREEN_DESKTOP;
#ifdef USE_XRANDR
	XRandR_Init();
	XRandR_FindOutput(info->devicename);
	if (fullscreen && !(fullscreenflags & FULLSCREEN_ANYMODE))
		XRandR_SelectMode(info->devicename, &x, &y, &width, &height, rate);
#endif

#ifdef USE_VMODE
	VMODE_Init();
	if (fullscreen && !(fullscreenflags & FULLSCREEN_ANYMODE))
		VMODE_SelectMode(&width, &height, rate);
#endif

	if (fullscreen)
	{
		if (!(fullscreenflags & (FULLSCREEN_ANYMODE&~FULLSCREEN_DESKTOP)))
		{
			//if we arn't using any mode switching extension then our fullscreen has to be the size of the root window
			//FIXME: with xrandr (even when not fullscreen), we should pick an arbitrary display to size the game appropriately.
			XWindowAttributes xwa;
			x11.pXGetWindowAttributes(vid_dpy, DefaultRootWindow(vid_dpy), &xwa);
			width = xwa.width;
			height = xwa.height;
			x = 0;
			y = 0;
#ifdef USE_XRANDR
			XRandr_PickScreen(info->devicename, &x, &y, &width, &height);
#endif
		}

		//window managers fuck up too much if we change the video mode and request the windowmanager make us fullscreen.
		//we assume that window manages will understand xrandr, as that actually provides notifications that things have changed.
		if (!(fullscreenflags & (FULLSCREEN_VMODE|FULLSCREEN_XRANDR)) && X_CheckWMFullscreenAvailable())
			fullscreenflags |= FULLSCREEN_WM;
		else
			fullscreenflags |= FULLSCREEN_LEGACY;
	}
	else if (sys_parentwindow)
	{
		if (width < 64 || width > sys_parentwidth)
			width = sys_parentwidth;
		if (height < 64 || height > sys_parentheight)
			height = sys_parentheight;
	}

	switch(currentpsl)
	{
#ifdef GLQUAKE
#ifdef USE_EGL
	case PSL_EGL:
		if (!EGL_InitDisplay(info, EGL_PLATFORM_X11_KHR, vid_dpy, (EGLNativeDisplayType)vid_dpy, &eglcfg))
		{
			Con_Printf("X11VID_Init: Unable to find suitable EGL config\n");
			GLVID_Shutdown();
			return false;
		}
		{
			int num_visuals;
			EGLint id;
			if (!qeglGetConfigAttrib(egldpy, eglcfg, EGL_NATIVE_VISUAL_ID, &id))
				Sys_Error("Couldn't choose visual for EGL\n");
			vinfodef.visualid = id;
			visinfo = x11.pXGetVisualInfo(vid_dpy, VisualIDMask, &vinfodef, &num_visuals);
			if (!visinfo)
				Sys_Error("Couldn't get visual info for EGL\n");
		}
		break;
#endif
	case PSL_GLX:
		if (glx.QueryExtensionsString)
			glx.glxextensions = glx.QueryExtensionsString(vid_dpy, scrnum);
		fbconfig = GLX_GetFBConfig(info);
		if (fbconfig)
			visinfo = glx.GetVisualFromFBConfig(vid_dpy, fbconfig);
		else
			visinfo = GLX_GetVisual(info);
		if (!visinfo)
		{
			Con_Printf("X11VID_Init: Error couldn't get an RGB, Double-buffered, Depth visual\n");
			GLVID_Shutdown();
			return false;
		}
		break;
#endif
#ifdef VKQUAKE
	case PSL_VULKAN:
		visinfo = &vinfodef;
		//the idea of a x11 server supporting vulkan but greyscale/palette video is just comedic. there will be a truecolor visual option.
		//WARNING: nvidia's vulkan drivers might fall back to 8bpc/24.8bpp here, then only allow a 30bit swapchain for that 8bpc surface. this will cause colour screwups. I'm going to call that a driver screwup if its choosing surface formats based on padded depth of the surface's window instead of x11's visuals or unpadded depth.
		if (info->bpp>0 && !x11.pXMatchVisualInfo(vid_dpy, scrnum, info->bpp, TrueColor, visinfo))
		{
			int defdepth = DefaultDepth(vid_dpy, scrnum);
			if (info->bpp!=defdepth && !x11.pXMatchVisualInfo(vid_dpy, scrnum, defdepth, TrueColor, visinfo))
			{
				if (defdepth!=24 && info->bpp!=24 && !x11.pXMatchVisualInfo(vid_dpy, scrnum, 24, TrueColor, visinfo))
				{
					Con_Printf("Couldn't choose visual for vulkan (depth %i), try changing vid_bpp\n", info->bpp?info->bpp:DefaultDepth(vid_dpy, scrnum));
					GLVID_Shutdown();
					return false;
				}
			}
		}
		break;
#endif
	default:
	case PSL_NONE:
		visinfo = NULL;
		break;	//erm
	}

	vid.activeapp = false;
	if (fullscreenflags & FULLSCREEN_LEGACY)
	{
		vid_decoywindow = X_CreateWindow(info, false, visinfo, x, y, 320, 200, false);
		vid_window = X_CreateWindow(info, true, visinfo, x, y, width, height, fullscreen);
	}
	else
		vid_window = X_CreateWindow(info, false, visinfo, x, y, width, height, fullscreen);

	vid_x_eventmask |= X_InitUnicode();
	x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask);

	CL_UpdateWindowTitle();
	/*make it visible*/

#ifdef USE_VMODE
	if (fullscreenflags & FULLSCREEN_VMODE)
	{
		x11.pXRaiseWindow(vid_dpy, vid_window);
		x11.pXWarpPointer(vid_dpy, None, vid_window, 0, 0, 0, 0, 0, 0);
		x11.pXFlush(vid_dpy);
		// Move the viewport to top left, in case its not already.
		vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
	}
#endif

	vid_nullcursor = CreateNullCursor(vid_dpy, vid_window);
	vid_newcursor = vid_activecursor = None;	//at this point, the cursor is undefined (aka: inherited from parent)

	x11.pXFlush(vid_dpy);

#ifdef USE_XRANDR
	if (xrandr.origgamma)
		vid.gammarampsize = xrandr.origgamma->size;
	else
#endif
#ifdef USE_VMODE
	if (vm.pXF86VidModeGetGammaRampSize)
	{
		int rampsize = 256;
		vm.pXF86VidModeGetGammaRampSize(vid_dpy, scrnum, &rampsize);
		if (rampsize > countof(vm.originalramps[0]))
		{
			vm.originalapplied = false;
			Con_Printf("Gamma ramps have more than %zi entries (%i).\n", countof(vm.originalramps[0]), rampsize);
		}
		else
		{
			vm.originalrampsize = vid.gammarampsize = rampsize;
			vm.originalapplied = vm.pXF86VidModeGetGammaRamp(vid_dpy, scrnum, vm.originalrampsize, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
		}
	}
	else
#endif
		vid.gammarampsize = 256;

	switch(currentpsl)
	{
#ifdef GLQUAKE
	case PSL_GLX:
		if (!GLX_Init(info, fbconfig, visinfo))
		{
#if defined(USE_EGL) || defined(VKQUAKE)
			if (visinfo != &vinfodef)
#endif
				x11.pXFree(visinfo);
			GLVID_Shutdown();
			return false;
		}
#if defined(USE_EGL) || defined(VKQUAKE)
		if (visinfo != &vinfodef)
#endif
			x11.pXFree(visinfo);

		glx.SwapIntervalEXT = GLX_CheckExtension("GLX_EXT_swap_control")?GLX_GetSymbol("glXSwapIntervalEXT"):NULL;
		glx.swaptear = glx.SwapIntervalEXT&&GLX_CheckExtension("GLX_EXT_swap_control_tear");
//		if (glx.swaptear)
//			glx.QueryDrawable(vid_dpy, vid_window, 0x20F3, &glx.swaptear);
		if (!glx.SwapIntervalEXT)
			glx.SwapIntervalMESA = GLX_CheckExtension("GLX_MESA_swap_control")?GLX_GetSymbol("glXSwapIntervalMESA"):NULL;
		if (!glx.SwapIntervalEXT && !glx.SwapIntervalMESA)
			glx.SwapIntervalSGI = GLX_CheckExtension("GLX_SGI_swap_control")?GLX_GetSymbol("glXSwapIntervalSGI"):NULL;
		glx.swapint = vid_vsync.ival;
		if (!glx.swaptear)
			glx.swapint	= abs(glx.swapint);
		if (*vid_vsync.string)
		{
			if (glx.SwapIntervalEXT /*&& (glx.swapint>=0 || swap_tear)*/)
			{
				glx.SwapIntervalEXT(vid_dpy, vid_window, glx.swapint);
				Con_DPrintf("Swap interval %i\n", glx.swapint);
			}
			else if (glx.SwapIntervalMESA && glx.swapint>=0)
			{
				glx.SwapIntervalMESA(abs(glx.swapint));
				Con_DPrintf("Swap interval %i\n", glx.swapint);
			}
			else if (glx.SwapIntervalSGI && glx.swapint>0)
			{
				glx.SwapIntervalSGI(glx.swapint);
				Con_DPrintf("Swap interval %i\n", glx.swapint);
			}
			else
				Con_Printf("Unable to explicitly %s vsync\n", glx.swapint?"configure":"disable");
		}
		break;
#ifdef USE_EGL
	case PSL_EGL:
		if (!EGL_InitWindow(info, EGL_PLATFORM_X11_KHR, &vid_window, (EGLNativeWindowType)vid_window, eglcfg))
		{
			Con_Printf("Failed to create EGL context.\n");
			GLVID_Shutdown();
			return false;
		}
		if (!GL_Init(info, &EGL_Proc))
			return false;
		break;
#endif
#endif
#ifdef VKQUAKE
	case PSL_VULKAN:
#ifdef VK_USE_PLATFORM_XLIB_KHR
		{
			const char *extnames[] = {VK_KHR_XLIB_SURFACE_EXTENSION_NAME, NULL};
			if (VK_Init(info, extnames, XVK_SetupSurface_XLib, NULL))
				break;
			VK_Shutdown();
		}
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
		{
			const char *extnames[] = {VK_KHR_XCB_SURFACE_EXTENSION_NAME, NULL};
			if (x11xcb_initlib() && VK_Init(info, extnames, XVK_SetupSurface_XCB, NULL))
				break;
			VK_Shutdown();
		}
#endif
		//Con_Printf(CON_ERROR "Failed to create a vulkan context.\n");
		GLVID_Shutdown();
		return false;
#endif
	case PSL_NONE:
		break;
	}

	//probably going to be resized in the event handler
	vid.pixelwidth = fullscreenwidth = width;
	vid.pixelheight = fullscreenheight = height;
	fullscreenx = x;
	fullscreeny = y;

	vid.numpages = 2;

//	Con_SafePrintf ("Video mode %dx%d+%d,%d initialized.\n", width, height, x, y);
	x11.pXRaiseWindow(vid_dpy, vid_window);
	if (fullscreenflags & FULLSCREEN_WM)
		X_GoFullscreen();
	if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx, fullscreeny, fullscreenwidth, fullscreenheight);
	if (fullscreenflags)
		fullscreenflags |= FULLSCREEN_ACTIVE;
	vid_isfullscreen = !!(fullscreenflags &FULLSCREEN_ACTIVE);

	if (X11_CheckFeature("xi2", true) && XI2_Init())
	{
		x11_input_method = XIM_XI2;
		Con_DPrintf("Using XInput2\n");
	}
	else if (X11_CheckFeature("dga", true) && DGAM_Init())
	{
		x11_input_method = XIM_DGA;
		Con_DPrintf("Using DGA mouse\n");
	}
	else
	{
		x11_input_method = XIM_ORIG;
		Con_DPrintf("Using X11 mouse\n");
	}
	XCursor_Init();
	X11Xss_Init();

	if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx, fullscreeny, fullscreenwidth, fullscreenheight);
	if (Cvar_Get("vidx_grabkeyboard", "0", 0, "Additional video options")->value)
		x11.pXGrabKeyboard(vid_dpy, vid_window,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);
	else if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXSetInputFocus(vid_dpy, vid_window, RevertToNone, CurrentTime);

	return true;
}
#ifdef GLQUAKE
qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return X11VID_Init(info, palette, PSL_GLX);
}
#ifdef USE_EGL
static qboolean EGLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return X11VID_Init(info, palette, PSL_EGL);
}
#endif
#endif
#ifdef VKQUAKE
static qboolean VKVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return X11VID_Init(info, palette, PSL_VULKAN);
}
#endif


void GLVID_SetCaption(const char *text)
{
	x11.pXStoreName(vid_dpy, vid_window, (char*)text);
}

#ifdef USE_EGL
#include "shader.h"
#include "gl_draw.h"
rendererinfo_t eglrendererinfo =
{
	"OpenGL (X11 EGL)",
	{
		"egl"
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

	EGLVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,

	NULL,
	NULL,
	NULL,
	GLVID_SetCaption,       //setcaption
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

	""
};
#endif

#ifdef VKQUAKE
#ifdef VK_USE_PLATFORM_XLIB_KHR
static qboolean XVK_SetupSurface_XLib(void)
{
	VkXlibSurfaceCreateInfoKHR inf = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
	inf.flags = 0;
	inf.dpy = vid_dpy;
	inf.window = vid_window;

	if (VK_SUCCESS == vkCreateXlibSurfaceKHR(vk.instance, &inf, vkallocationcb, &vk.surface))
		return true;
	return false;
}
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
static qboolean XVK_SetupSurface_XCB(void)
{
	VkXcbSurfaceCreateInfoKHR inf = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
	inf.flags = 0;
	inf.connection = x11xcb.pXGetXCBConnection(vid_dpy);
	inf.window = vid_window;

	if (VK_SUCCESS == vkCreateXcbSurfaceKHR(vk.instance, &inf, vkallocationcb, &vk.surface))
		return true;
	return false;
}
#endif
static qboolean XVK_EnumerateDevices(void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc))
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
	ret = VK_EnumerateDevices(usercontext, callback, "Vulkan-X11-", vkGetInstanceProcAddr);
#ifdef VK_NO_PROTOTYPES
	Sys_CloseLibrary(lib);
#endif
	return ret;
}
rendererinfo_t vkrendererinfo =
{
	"Vulkan (X11)",
	{
		"xvk",
		"vk",
		"vulkan"
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

	VKVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,

	NULL,
	NULL,
	NULL,
	GLVID_SetCaption,       //setcaption
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

	NULL,//int	(*VID_GetPriority)	(void);	//so that eg x11 or wayland can be prioritised depending on environment settings. assumed to be 1.
	NULL,//void	(*VID_EnumerateVideoModes) (const char *driver, const char *output, void (*cb) (int w, int h));
	XVK_EnumerateDevices,//qboolean	(*VID_EnumerateDevices) (void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc));
};
#endif

#if 1
static void (*paste_callback)(void *cb, const char *utf8);
static void *pastectx;
static struct {
	Atom clipboard;
	Atom prop;
	Atom owner;
} x11paste;
void Sys_Clipboard_PasteText(clipboardtype_t clipboardtype, void (*callback)(void *ctx, const char *utf8), void *ctx)
{
	//if there's a paste already pending, cancel the callback to ensure it always gets called.
	if (paste_callback)
		paste_callback(pastectx, NULL);

	paste_callback = NULL;
	pastectx = NULL;

	if(vid_dpy)
	{
		Window clipboardowner;
		Atom xa_string = x11.pXInternAtom(vid_dpy, "UTF8_STRING", false);
		//FIXME: we should query it using TARGETS first to see if UTF8_STRING etc is actually valid.
		if (clipboardtype == CBT_SELECTION)
			x11paste.clipboard = x11.pXInternAtom(vid_dpy, "PRIMARY", false);
		else
			x11paste.clipboard = x11.pXInternAtom(vid_dpy, "CLIPBOARD", false);
		x11paste.prop = x11.pXInternAtom(vid_dpy, "_FTE_PASTE", false);
		clipboardowner = x11.pXGetSelectionOwner(vid_dpy, x11paste.clipboard);
		if (clipboardowner == vid_window)
			callback(ctx, clipboard_buffer[clipboardtype]);	//we own it? no point doing round-robin stuff.
		else if (clipboardowner != None && clipboardowner != vid_window)
		{
			x11.pXConvertSelection(vid_dpy, x11paste.clipboard, xa_string, x11paste.prop, vid_window, CurrentTime);

			paste_callback = callback;
			pastectx = ctx;
		}
		else
			callback(ctx, NULL);	//nothing to paste (the window that owned it sucks
	}
	else
		callback(ctx, clipboard_buffer[clipboardtype]);	//if we're not using x11 then just instantly call the callback
}
static qboolean X11_Clipboard_Notify(XSelectionEvent *xselection)
{	//called once XConvertSelection completes
	if (xselection->display == vid_dpy && xselection->selection == x11paste.clipboard && xselection->requestor == vid_window && xselection->property == x11paste.prop)
	{
		int fmt;
		Atom type;
		unsigned long nitems, bytesleft;
		unsigned char *data = NULL;
		x11.pXGetWindowProperty(vid_dpy, vid_window, x11paste.prop, 0, 65536, False, AnyPropertyType, &type, &fmt, &nitems, &bytesleft, &data);
		if (paste_callback)
			paste_callback(pastectx, data);
		x11.pXFree(data);
		paste_callback = NULL;
		pastectx = NULL;
		return true;
	}
	return false;
}

/*char *Sys_GetClipboard(void)
{
	if(vid_dpy)
	{
		//FIXME: we should query it using TARGETS first to see if UTF8_STRING etc is actually valid.
		Atom xa_clipboard = x11.pXInternAtom(vid_dpy, "PRIMARY", false);
		Atom xa_string = x11.pXInternAtom(vid_dpy, "UTF8_STRING", false);
		Window clipboardowner = x11.pXGetSelectionOwner(vid_dpy, xa_clipboard);
		if (clipboardowner != None && clipboardowner != vid_window)
		{
			int fmt;
			Atom type;
			unsigned long nitems, bytesleft;
			unsigned char *data;
			x11.pXConvertSelection(vid_dpy, xa_clipboard, xa_string, None, vid_window, CurrentTime);

			//FIXME: we should rewrite the clipboard pasting to invoke a callback once its available.
			x11.pXFlush(vid_dpy);
			x11.pXSync(vid_dpy, False);
			Sys_Sleep(0.3);
			x11.pXSync(vid_dpy, False);

			//and now we can actually read the data.
			x11.pXGetWindowProperty(vid_dpy, vid_window, xa_string, 0, 65536, False, AnyPropertyType, &type, &fmt, &nitems, &bytesleft, &data);
			
			return data;
		}
	}
	return clipboard_buffer;
}
*/

void Sys_SaveClipboard(clipboardtype_t clipboardtype, const char *text)
{
	free(clipboard_buffer[clipboardtype]);
	clipboard_buffer[clipboardtype] = strdup(text);
	if(vid_dpy)
	{
		Atom xa_clipboard;
		switch(clipboardtype)
		{
		case CBT_SELECTION:
			xa_clipboard = x11.pXInternAtom(vid_dpy, "PRIMARY", false);
			break;
		case CBT_CLIPBOARD:
			xa_clipboard = x11.pXInternAtom(vid_dpy, "CLIPBOARD", false);
			break;
		default:
			return;
		}
		x11.pXSetSelectionOwner(vid_dpy, xa_clipboard, vid_window, CurrentTime);
	}
}
#endif

qboolean X11_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	Display *xtemp;
	int scr;

	if (!x11_initlib())
		return false;

	xtemp = x11.pXOpenDisplay(NULL);

	if (!xtemp)
		return false;

	scr = DefaultScreen(xtemp);

	*width = 0;//DisplayWidth(xtemp, scr);
	*height = 0;//DisplayHeight(xtemp, scr);
	*bpp = DefaultDepth(xtemp, scr);
	*refreshrate = 0;

	x11.pXCloseDisplay(xtemp);

	return true;
}
#endif




void Sys_SendKeyEvents(void)
{
#ifndef CLIENTONLY
	//this is stupid
	SV_GetConsoleCommands();
#endif

#ifndef NO_X11
	if (sys_gracefulexit)
	{
		Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
		sys_gracefulexit = false;
	}
	if (vid_dpy && vid_window)
	{
		if (x11.unicodecontext)
		{
			qboolean want = vid.ime_allow && vid.activeapp;
			XPoint pos;
			if (want != x11.ime_shown)
			{
				x11.ime_shown = want;
				if (x11.ime_shown)
					x11.pXSetICFocus(x11.unicodecontext);
				else
					x11.pXUnsetICFocus(x11.unicodecontext);
			}
			pos.x = (vid.ime_position[0] * vid.pixelwidth)/vid.width;
			pos.y = (vid.ime_position[1] * vid.pixelheight)/vid.height;
			if (/*x11.ime_shown &&*/ (x11.ime_pos.x != pos.x || x11.ime_pos.y != pos.y))
			{
				void *attr[] = {XNSpotLocation, &x11.ime_pos, NULL};
				x11.ime_pos = pos;
				x11.pXSetICValues(x11.unicodecontext, XNPreeditAttributes, attr, NULL);
			}
		}

		while (x11.pXPending(vid_dpy))
			GetEvent();

		if (modeswitchpending && modeswitchtime < Sys_Milliseconds())
		{
			UpdateGrabs();
			if (modeswitchpending > 0 && !(fullscreenflags & FULLSCREEN_ACTIVE))
			{
				//entering fullscreen mode
#ifdef USE_VMODE
				if (fullscreenflags & FULLSCREEN_VMODE)
				{
					if (!(fullscreenflags & FULLSCREEN_VMODEACTIVE))
					{
						// change to the mode
						vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[vm.usemode]);
						fullscreenflags |= FULLSCREEN_VMODEACTIVE;
						// Move the viewport to top left
					}
					vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
				}
#endif
#ifdef USE_XRANDR
				if (fullscreenflags & FULLSCREEN_XRANDR)
					XRandR_ApplyMode();
#endif
				Cvar_ForceCallback(&v_gamma);

				/*release the mouse now, because we're paranoid about clip regions*/
				if (fullscreenflags & FULLSCREEN_WM)
					X_GoFullscreen();
				if (fullscreenflags & FULLSCREEN_LEGACY)
				{
					x11.pXReparentWindow(vid_dpy, vid_window, vid_root, fullscreenx, fullscreeny);
				//	if (vid_decoywindow)
				//		x11.pXMoveWindow(vid_dpy, vid_decoywindow, fullscreenx, fullscreeny);
					//x11.pXUnmapWindow(vid_dpy, vid_decoywindow);
					//make sure we have it
					x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime);
					x11.pXRaiseWindow(vid_dpy, vid_window);
					x11.pXMoveResizeWindow(vid_dpy, vid_window, fullscreenx, fullscreeny, fullscreenwidth, fullscreenheight);
				}
				if (fullscreenflags)
					fullscreenflags |= FULLSCREEN_ACTIVE;
				vid_isfullscreen = !!(fullscreenflags &FULLSCREEN_ACTIVE);
			}
			if (modeswitchpending < 0)
			{
				//leave fullscreen mode
		 		if (!COM_CheckParm("-stayactive"))
 				{	//a parameter that leaves the program fullscreen if you taskswitch.
 					//sounds pointless, works great with two moniters. :D
#ifdef USE_VMODE
					if (fullscreenflags & FULLSCREEN_VMODE)
					{
	 					if (vm.originalapplied)
							vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, vm.originalrampsize, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
						if (fullscreenflags & FULLSCREEN_VMODEACTIVE)
						{
							vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[0]);
							fullscreenflags &= ~FULLSCREEN_VMODEACTIVE;
						}
					}
#endif
#ifdef USE_XRANDR
					if (fullscreenflags & FULLSCREEN_XRANDR)
						XRandR_RevertMode();
#endif
					if (fullscreenflags & FULLSCREEN_WM)
						X_GoWindowed();
					if (fullscreenflags & FULLSCREEN_LEGACY)
					{
						x11.pXReparentWindow(vid_dpy, vid_window, vid_decoywindow, 0, 0);
//						x11.pXMoveResizeWindow(vid_dpy, vid_decoywindow, fullscreenx + (fullscreenwidth-640)/2, fullscreeny + (fullscreenheight-480)/2, 640, 480);
						x11.pXMapWindow(vid_dpy, vid_decoywindow);
					}
					fullscreenflags &= ~FULLSCREEN_ACTIVE;
					vid_isfullscreen = !!(fullscreenflags &FULLSCREEN_ACTIVE);
				}
			}
			modeswitchpending = 0;
		}

		if (modeswitchpending)
			return;

		UpdateGrabs();
	}
#endif
}

/*static void Force_CenterView_f (void)
{
	cl.playerview[0].viewangles[PITCH] = 0;
}*/


//these are done from the x11 event handler. we don't support evdev.
void INS_Move(void)
{
}
void INS_Commands(void)
{
}
void INS_Init(void)
{
}
void INS_ReInit(void)
{
}
void INS_Shutdown(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
#ifndef NO_X11
	callback(ctx, "keyboard", "x11", NULL);
	switch(x11_input_method)
	{
	case XIM_ORIG:
		callback(ctx, "mouse", "x11", &x11_mouseqdev);
		break;
	case XIM_DGA:
		callback(ctx, "mouse", "dga", &x11_mouseqdev);
		break;
	case XIM_XI2:
		{
			int i, devs;
			XIDeviceInfo *dev = xi2.pXIQueryDevice(vid_dpy, xi2.devicegroup, &devs);
			if (dev)
			{
				for (i = 0; i < devs; i++)
				{
					if (!dev[i].enabled)
						continue;
					if (/*dev[i].use == XIMasterPointer ||*/ dev[i].use == XISlavePointer)
					{
						struct xidevinfo *devi = XI2_GetDeviceInfo(dev[i].deviceid);
						callback(ctx, devi->abs?"tablet":"mouse", dev[i].name, &devi->qdev);
					}
//					else if (dev[i].use == XIMasterKeyboard || dev[i].use == XISlaveKeyboard)
//					{
//						int qdev = dev[i].deviceid;
//						callback(ctx, "xi2kb", dev[i].name, &qdev);
//					}
				}
				xi2.pXIFreeDeviceInfo(dev);
			}
		}
		break;
	}
#endif
}

enum controllertype_e INS_GetControllerType(int id)
{
	return CONTROLLER_NONE;
}
/* doubt this will ever happen to begin with */
void INS_Rumble(int id, quint16_t amp_low, quint16_t amp_high, quint32_t duration)
{
	//Con_DPrintf(CON_WARNING "Rumble is unavailable on this platform\n");
}

void INS_RumbleTriggers(int id, quint16_t left, quint16_t right, quint32_t duration)
{
	//Con_DPrintf(CON_WARNING "Trigger rumble is unavailable on this platform\n");
}

void INS_SetLEDColor(int id, vec3_t color)
{
	//Con_DPrintf(CON_WARNING "Game-Pad LED colors are unavailable on this platform\n");
}

void INS_SetTriggerFX(int id, const void *data, size_t size)
{
	//Con_DPrintf(CON_WARNING "Trigger FX are unavailable on this platform\n");
}
