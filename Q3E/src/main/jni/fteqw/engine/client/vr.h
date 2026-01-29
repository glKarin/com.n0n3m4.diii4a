#include "merged.h"
typedef struct vrsetup_s
{
	//engine-set
	size_t structsize;
	enum
	{
		VR_HEADLESS,	//not to be confused with decapitation
		VR_EGL,
		VR_X11_GLX,
//		VR_ANDROID_EGL,
		VR_WIN_WGL,
		VR_VULKAN,		//vulkan has no platform variation
		VR_D3D11,		//d3d11 only works on windows, so no platform variation
	} vrplatform;		//the type of renderer/args getting inited. abort if unknown.
	void *userctx;		//for use in callbacks.
	qboolean (*createinstance)(struct vrsetup_s *, char *instanceextensions, void *result);				//used by vulkan, can be null for other renderers

	//vr-set (by preinit)
	struct
	{
		int major, minor;
	} minver, maxver;
	unsigned int deviceid[2];
	char *deviceextensions;


	//engine-set (for full init)
	//this stuff is intentionally at the end
	union {
		struct
		{
			void *display;
			int visualid;
			void *glxfbconfig;
			unsigned long drawable;	//really int32
			void *glxcontext;
		} x11_glx;

		struct
		{
			void *(*getprocaddr)(const char *name);
			void *egldisplay;
			void *eglconfig;
			void *eglcontext;
		} egl;

		struct
		{
			void *hdc;
			void *hglrc;
		} wgl;

		struct
		{
			void *device;
		} d3d;

		struct
		{	//these are ALWAYS pointers in vulkan (annoyingly unlike many of its typedefs).
			void *instance;
			void *physicaldevice;
			void *device;
			unsigned int queuefamily;
			unsigned int queueindex;
		} vk;
	};
} vrsetup_t;

#define VRF_OVERRIDEFRAMETIME	1	//the vr interface is responsible for determining frame intervals instead of using regular clocks (so they can fiddle with prediction etc)
#define VRF_UIACTIVE			2	//we're actually rendering through a headset. use 3d rendering exclusively, with VR UI and stuff.

//interface registered by plugins for VR stuff.
typedef struct plugvrfuncs_s
{
	const char	*description;
	qboolean	(*Prepare)	(vrsetup_t *setupinfo);	//called before graphics context init
	qboolean	(*Init)		(vrsetup_t *setupinfo, rendererstate_t *info);	//called after graphics context init
	unsigned int	(*SyncFrame)(double *frametime);	//called in the client's main loop, to block/tweak frame times. True means the game should render as fast as possible.
	qboolean	(*Render)	(void(*rendereye)(texid_t tex, const pxrect_t *viewport, const vec4_t fovoverride, const float projmatrix[16], const float eyematrix[12]));
	void		(*Shutdown)	(void);
#define plugvrfuncs_name "VR"
} plugvrfuncs_t;
