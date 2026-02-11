#include "plugin.h"
static plugfsfuncs_t *fsfuncs;
static pluginputfuncs_t *inputfuncs;

#include "../engine/client/vr.h"

//#define XR_NO_PROTOTYPES

//figure out which platforms(read: windowing apis) we need...
#if defined(_WIN32)
	#define XR_USE_PLATFORM_WIN32
//#elif defined(ANDROID)
//	#define XR_USE_PLATFORM_ANDROID
#else
	#ifndef NO_X11
		#define XR_USE_PLATFORM_XLIB
	#endif
	#if defined(GLQUAKE) && defined(USE_EGL)
		//wayland, android, and x11-egl can all just use the EGL path...
		//at least once the openxr spec gets fixed (the wayland extension is apparently basically unusable)
		//note: XR_MND_egl_enable is a vendor extension to work around openxr stupidly trying to ignore it entirely.
		#define XR_USE_PLATFORM_EGL
	#endif
#endif

//figure out which graphics apis we need...
#ifdef GLQUAKE
	#define XR_USE_GRAPHICS_API_OPENGL
#endif
#ifdef VKQUAKE
	#define XR_USE_GRAPHICS_API_VULKAN
#endif
#ifdef D3D11QUAKE
	#ifdef _WIN32
		#define XR_USE_GRAPHICS_API_D3D11
	#endif
#endif

//include any headers we need for things to make sense.
#ifdef XR_USE_GRAPHICS_API_OPENGL
	#include "glquake.h"
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	#include "../engine/vk/vkrenderer.h"
#endif
#ifdef XR_USE_GRAPHICS_API_D3D11
	#include <d3d11.h>
#endif
#ifdef XR_USE_PLATFORM_EGL
	#include "gl_videgl.h"
#endif
#ifdef XR_USE_PLATFORM_XLIB
	#include <GL/glx.h>
#endif

//and finally include openxr stuff now that its hopefully not going to fail about missing typedefs.
#include <openxr/openxr_platform.h>
#if XR_CURRENT_API_VERSION < XR_MAKE_VERSION(1, 0, 16)
#define XR_ERROR_RUNTIME_UNAVAILABLE -51	//available starting 1.0.16
#endif

#ifdef XR_NO_PROTOTYPES
#define XRFUNCS		\
		XRFUNC(xrGetInstanceProcAddr)	\
		XRFUNC(xrResultToString)	\
		XRFUNC(xrEnumerateApiLayerProperties) \
		XRFUNC(xrEnumerateInstanceExtensionProperties)	\
		XRFUNC(xrCreateInstance)	\
		XRFUNC(xrGetInstanceProperties)	\
		XRFUNC(xrGetSystem)	\
		XRFUNC(xrGetSystemProperties)	\
		XRFUNC(xrEnumerateViewConfigurations) \
		XRFUNC(xrEnumerateViewConfigurationViews)	\
		XRFUNC(xrCreateSession)	\
		XRFUNC(xrCreateReferenceSpace)	\
		XRFUNC(xrCreateActionSet)	\
		XRFUNC(xrStringToPath)	\
		XRFUNC(xrCreateAction)	\
		XRFUNC(xrSuggestInteractionProfileBindings)	\
		XRFUNC(xrCreateActionSpace)	\
		XRFUNC(xrAttachSessionActionSets)	\
		XRFUNC(xrSyncActions)	\
		XRFUNC(xrGetActionStatePose)	\
		XRFUNC(xrApplyHapticFeedback)	\
		XRFUNC(xrLocateSpace)	\
		XRFUNC(xrGetActionStateBoolean)	\
		XRFUNC(xrGetActionStateFloat)	\
		XRFUNC(xrGetActionStateVector2f)	\
		XRFUNC(xrGetCurrentInteractionProfile)	\
		XRFUNC(xrEnumerateBoundSourcesForAction)	\
		XRFUNC(xrGetInputSourceLocalizedName)	\
		XRFUNC(xrPathToString)	\
		XRFUNC(xrCreateSwapchain)	\
		XRFUNC(xrEnumerateSwapchainFormats)	\
		XRFUNC(xrEnumerateSwapchainImages)	\
		XRFUNC(xrPollEvent)	\
		XRFUNC(xrBeginSession)	\
		XRFUNC(xrWaitFrame)	\
		XRFUNC(xrBeginFrame)	\
		XRFUNC(xrLocateViews)	\
		XRFUNC(xrAcquireSwapchainImage)	\
		XRFUNC(xrWaitSwapchainImage)	\
		XRFUNC(xrReleaseSwapchainImage)	\
		XRFUNC(xrEndFrame)	\
		XRFUNC(xrRequestExitSession)	\
		XRFUNC(xrEndSession)	\
		XRFUNC(xrDestroySwapchain)	\
		XRFUNC(xrDestroySpace)	\
		XRFUNC(xrDestroySession)	\
		XRFUNC(xrDestroyInstance)
#define XRFUNC(n) static PFN_##n n;
XRFUNCS
#undef XRFUNC
#endif

#ifdef XR_EXT_hand_tracking
static PFN_xrCreateHandTrackerEXT	xrCreateHandTrackerEXT;
static PFN_xrLocateHandJointsEXT	xrLocateHandJointsEXT;
static PFN_xrDestroyHandTrackerEXT	xrDestroyHandTrackerEXT;
#endif


#ifdef SVNREVISION
	#define APPLICATIONVERSION atoi(STRINGIFY(SVNREVISION))
	#define ENGINEVERSION atoi(STRINGIFY(SVNREVISION))
#else
	#define APPLICATIONVERSION 0
	#define ENGINEVERSION 0
#endif

static cvar_t *xr_enable;
static cvar_t *xr_debug;
static cvar_t *xr_formfactor;
static cvar_t *xr_viewconfig;
static cvar_t *xr_metresize;
static cvar_t *xr_skipregularview;
static cvar_t *xr_fingertracking;

static void XR_SetupInputs_Instance(void);

#define METRES_TO_QUAKE(x) ((x)*xr_metresize->value)
#define QUAKE_TO_METRES(x) ((x)/xr_metresize->value)

#define SECONDS_TO_NANOSECONDS(x) ((x)*1000000000)
#define NANOSECONDS_TO_SECONDS(x) ((x)/1000000000.0)

static void XR_PoseToAngOrg(const XrPosef *pose, vec3_t ang, vec3_t org)
{
	XrQuaternionf q = pose->orientation;
    const float sqw = q.w * q.w;
    const float sqx = q.x * q.x;
    const float sqy = q.y * q.y;
    const float sqz = q.z * q.z;

    ang[PITCH] = -asin(-2 * (q.y * q.z - q.w * q.x)) * (180/M_PI);
    ang[YAW] = atan2(2 * (q.x * q.z + q.w * q.y), sqw - sqx - sqy + sqz) * (180/M_PI);
    ang[ROLL] = -atan2(2 * (q.x * q.y + q.w * q.z), sqw - sqx + sqy - sqz) * (180/M_PI);

#if 1
	org[0]  =     METRES_TO_QUAKE(-pose->position.z);
	org[1]  =     METRES_TO_QUAKE(-pose->position.x);
	org[2]  =     METRES_TO_QUAKE(pose->position.y);
#else
	org[0]  =     METRES_TO_QUAKE(pose->position.x);
	org[1]  =     METRES_TO_QUAKE(pose->position.y);
	org[2]  =     METRES_TO_QUAKE(pose->position.z);
#endif
}
static void XR_PoseToMat12(const XrPosef *pose, float *out)
{
	vec3_t angles, origin;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	XR_PoseToAngOrg(pose, angles, origin);

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	out[0] = cp*cy;
	out[1] = (sr*sp*cy+cr*-sy);
	out[2] = (cr*sp*cy+-sr*-sy);
	out[3] = origin[0];

	out[4] = cp*sy;
	out[5] = (sr*sp*sy+cr*cy);
	out[6] = (cr*sp*sy+-sr*cy);
	out[7] = origin[1];

	out[8] = -sp;
	out[9] = sr*cp;
	out[10] = cr*cp;
	out[11] = origin[2];
}

static void XR_Matrix3x4_RM_FromAngles(const vec3_t angles, const vec3_t origin, float *out)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	out[0] = cp*cy;
	out[1] = (sr*sp*cy+cr*-sy);
	out[2] = (cr*sp*cy+-sr*-sy);
	out[3] = origin[0];

	out[4] = cp*sy;
	out[5] = (sr*sp*sy+cr*cy);
	out[6] = (cr*sp*sy+-sr*cy);
	out[7] = origin[1];

	out[8] = -sp;
	out[9] = sr*cp;
	out[10] = cr*cp;
	out[11] = origin[2];
}

enum actset_e
{
	AS_COMMON,
	AS_MENU,
	AS_GAME,
	MAX_ACTIONSETS
};

static struct
{
//instance state (in case we want to start up)
	XrInstance instance;	//loader context
	XrSystemId systemid;	//device type thingie we're going for
#define MAX_VIEW_COUNT 12	//kinda abusive, but that's VR for you.
	unsigned int viewcount;
	XrViewConfigurationView *views;
	XrViewConfigurationType viewtype;

//engine context info (for restarting sessions)
	int renderer;			//rendering api we're using
	void *bindinginfo;		//appropriate XrGraphicsBinding*KHR struct so we can restart sessions.

//session state
	qboolean fake;
	XrSession session;		//driver context
	XrSessionState state;
	qboolean beginning;
	qboolean ending;
	XrSpace space;
	struct
	{	//basically just swapchain state.
		XrSwapchain swapchain;
		unsigned int numswapimages;
		XrSwapchainSubImage subimage;
		image_t *swapimages;
	} eye[MAX_VIEW_COUNT];	//note that eye is a vauge term.

	XrActiveActionSet actionset[MAX_ACTIONSETS];

	qboolean timeknown;
	XrTime time;
	XrFrameState framestate;
	qboolean needrender;	//we MUST call xrBegin before the next xrWait
	int srgb;	//<0 = gamma-only. 0 = no srgb at all, >0 full srgb, including textures and stuff
	int colourformat;

	unsigned int numactions;
	unsigned int maxactions;
	struct
	{
		enum actset_e set;
		XrActionType acttype;
		const char *actname;		//doubles up as command names for buttons
		const char *actdescription;	//user-visible string (exposed via openxr runtime somehow)
		const char *subactionpath;	//somethingblahblah

		XrAction	action;	//for querying.
		XrPath		path;	//for querying.
		XrSpace		space;	//for poses.
		qboolean	held;	//for buttons.
	} *actions;
	qboolean inputsdirty;	//mostly for printing them.

#ifdef XR_EXT_hand_tracking
	struct
	{
		XrHandTrackerEXT handle;
		qboolean active;
		XrHandJointLocationEXT jointloc[XR_HAND_JOINT_COUNT_EXT];
		XrHandJointVelocityEXT jointvel[XR_HAND_JOINT_COUNT_EXT];
	} hand[2];
#endif
} xr;

static qboolean QDECL XR_PluginMayUnload(void)
{
	if (xr.instance)
		return false;	//something is still using us... don't let our code go away.
	return true;
}
static void XR_SessionEnded(void)
{
	size_t u;
	if (xr.space)
	{
		xrDestroySpace(xr.space);
		xr.space = XR_NULL_HANDLE;
	}

	for (u = 0; u < countof(xr.eye); u++)
	{
		free(xr.eye[u].swapimages);
		xr.eye[u].swapimages = NULL;
		xr.eye[u].numswapimages = 0;
		if (xr.eye[u].swapchain)
		{
			xrDestroySwapchain(xr.eye[u].swapchain);
			xr.eye[u].swapchain = XR_NULL_HANDLE;
		}
	}

#ifdef XR_EXT_hand_tracking
	for (u = 0; u < countof(xr.hand); u++)
	{
		if (xr.hand[u].handle)
			xrDestroyHandTrackerEXT(xr.hand[u].handle);
		xr.hand[u].handle = XR_NULL_HANDLE;
		xr.hand[u].active = false;
	}
#endif

	if (xr.session)
	{
		xrDestroySession(xr.session);
		xr.session = XR_NULL_HANDLE;
	}
	xr.state = XR_SESSION_STATE_UNKNOWN;
	xr.beginning = false;
}
static void XR_Shutdown(void)
{	//called on any kind of failure
	XR_SessionEnded();

	free(xr.bindinginfo);
	free(xr.views);
	if (xr.instance)
		xrDestroyInstance(xr.instance);

	memset(&xr, 0, sizeof(xr));
}

static const char *XR_StringForResult(XrResult res)
{
#if 0
	//this is a bit of a joke really. xrResultToString requires a valid instance so is unusable for printing out the various reasons why we might fail to create an instance.
	static char buffer[XR_MAX_RESULT_STRING_SIZE];
	if (XR_SUCCEEDED(res=xrResultToString(xr.instance, res, buffer)))
		return buffer;
	return va("XrResult %i", res);
#else
	switch(res)
	{
	case XR_SUCCESS: return "XR_SUCCESS";
	case XR_TIMEOUT_EXPIRED: return "XR_TIMEOUT_EXPIRED";
	case XR_SESSION_LOSS_PENDING: return "XR_SESSION_LOSS_PENDING";
	case XR_EVENT_UNAVAILABLE: return "XR_EVENT_UNAVAILABLE";
	case XR_SPACE_BOUNDS_UNAVAILABLE: return "XR_SPACE_BOUNDS_UNAVAILABLE";
	case XR_SESSION_NOT_FOCUSED: return "XR_SESSION_NOT_FOCUSED";
	case XR_FRAME_DISCARDED: return "XR_FRAME_DISCARDED";
	case XR_ERROR_VALIDATION_FAILURE: return "XR_ERROR_VALIDATION_FAILURE";
	case XR_ERROR_RUNTIME_FAILURE: return "XR_ERROR_RUNTIME_FAILURE";
	case XR_ERROR_OUT_OF_MEMORY: return "XR_ERROR_OUT_OF_MEMORY";
	case XR_ERROR_API_VERSION_UNSUPPORTED: return "XR_ERROR_API_VERSION_UNSUPPORTED";
	case XR_ERROR_INITIALIZATION_FAILED: return "XR_ERROR_INITIALIZATION_FAILED";
	case XR_ERROR_FUNCTION_UNSUPPORTED: return "XR_ERROR_FUNCTION_UNSUPPORTED";
	case XR_ERROR_FEATURE_UNSUPPORTED: return "XR_ERROR_FEATURE_UNSUPPORTED";
	case XR_ERROR_EXTENSION_NOT_PRESENT: return "XR_ERROR_EXTENSION_NOT_PRESENT";
	case XR_ERROR_LIMIT_REACHED: return "XR_ERROR_LIMIT_REACHED";
	case XR_ERROR_SIZE_INSUFFICIENT: return "XR_ERROR_SIZE_INSUFFICIENT";
	case XR_ERROR_HANDLE_INVALID: return "XR_ERROR_HANDLE_INVALID";
	case XR_ERROR_INSTANCE_LOST: return "XR_ERROR_INSTANCE_LOST";
	case XR_ERROR_SESSION_RUNNING: return "XR_ERROR_SESSION_RUNNING";
	case XR_ERROR_SESSION_NOT_RUNNING: return "XR_ERROR_SESSION_NOT_RUNNING";
	case XR_ERROR_SESSION_LOST: return "XR_ERROR_SESSION_LOST";
	case XR_ERROR_SYSTEM_INVALID: return "XR_ERROR_SYSTEM_INVALID";
	case XR_ERROR_PATH_INVALID: return "XR_ERROR_PATH_INVALID";
	case XR_ERROR_PATH_COUNT_EXCEEDED: return "XR_ERROR_PATH_COUNT_EXCEEDED";
	case XR_ERROR_PATH_FORMAT_INVALID: return "XR_ERROR_PATH_FORMAT_INVALID";
	case XR_ERROR_PATH_UNSUPPORTED: return "XR_ERROR_PATH_UNSUPPORTED";
	case XR_ERROR_LAYER_INVALID: return "XR_ERROR_LAYER_INVALID";
	case XR_ERROR_LAYER_LIMIT_EXCEEDED: return "XR_ERROR_LAYER_LIMIT_EXCEEDED";
	case XR_ERROR_SWAPCHAIN_RECT_INVALID: return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
	case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED: return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";
	case XR_ERROR_ACTION_TYPE_MISMATCH: return "XR_ERROR_ACTION_TYPE_MISMATCH";
	case XR_ERROR_SESSION_NOT_READY: return "XR_ERROR_SESSION_NOT_READY";
	case XR_ERROR_SESSION_NOT_STOPPING: return "XR_ERROR_SESSION_NOT_STOPPING";
	case XR_ERROR_TIME_INVALID: return "XR_ERROR_TIME_INVALID";
	case XR_ERROR_REFERENCE_SPACE_UNSUPPORTED: return "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED";
	case XR_ERROR_FILE_ACCESS_ERROR: return "XR_ERROR_FILE_ACCESS_ERROR";
	case XR_ERROR_FILE_CONTENTS_INVALID: return "XR_ERROR_FILE_CONTENTS_INVALID";
	case XR_ERROR_FORM_FACTOR_UNSUPPORTED: return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
	case XR_ERROR_FORM_FACTOR_UNAVAILABLE: return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
	case XR_ERROR_API_LAYER_NOT_PRESENT: return "XR_ERROR_API_LAYER_NOT_PRESENT";
	case XR_ERROR_CALL_ORDER_INVALID: return "XR_ERROR_CALL_ORDER_INVALID";
	case XR_ERROR_GRAPHICS_DEVICE_INVALID: return "XR_ERROR_GRAPHICS_DEVICE_INVALID";
	case XR_ERROR_POSE_INVALID: return "XR_ERROR_POSE_INVALID";
	case XR_ERROR_INDEX_OUT_OF_RANGE: return "XR_ERROR_INDEX_OUT_OF_RANGE";
	case XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED: return "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED";
	case XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED: return "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED";
	case XR_ERROR_NAME_DUPLICATED: return "XR_ERROR_NAME_DUPLICATED";
	case XR_ERROR_NAME_INVALID: return "XR_ERROR_NAME_INVALID";
	case XR_ERROR_ACTIONSET_NOT_ATTACHED: return "XR_ERROR_ACTIONSET_NOT_ATTACHED";
	case XR_ERROR_ACTIONSETS_ALREADY_ATTACHED: return "XR_ERROR_ACTIONSETS_ALREADY_ATTACHED";
	case XR_ERROR_LOCALIZED_NAME_DUPLICATED: return "XR_ERROR_LOCALIZED_NAME_DUPLICATED";
	case XR_ERROR_LOCALIZED_NAME_INVALID: return "XR_ERROR_LOCALIZED_NAME_INVALID";
#if XR_CURRENT_API_VERSION >= XR_MAKE_VERSION(1, 0, 16)
	case XR_ERROR_RUNTIME_UNAVAILABLE: return "XR_ERROR_RUNTIME_UNAVAILABLE";
#endif
	default:
		return va("XrResult %i", res);
	}
#endif
}


static XrBool32 XRAPI_CALL XR_DebugPrint(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes, const XrDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData)
{
	char *sev;
	switch(messageSeverity)
	{
	case 1/*XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT*/:		sev = "^8";			break;
	case 16/*XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT*/:		sev = "";			break;
	case 256/*XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT*/:	sev = CON_WARNING;	break;
	default:
	case 4096/*XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT*/:		sev = CON_ERROR;	break;
	}

	Con_Printf("%s%s: %s\n", sev, callbackData->functionName, callbackData->message);
	return XR_FALSE;
}

static qboolean XR_PreInit(vrsetup_t *qreqs)
{
	XrResult res;
	const char *xrexts[8], *ext;
	const char *xrlayers[8];
	uint32_t numext = 0, numlayers = 0;
	qboolean havedebugutils = false;
#ifdef XR_EXT_hand_tracking
	qboolean havehandtrack = false;
#endif

	XR_Shutdown();	//just in case...

	if (qreqs->structsize != sizeof(*qreqs) || xr_enable->ival < 0)
		return false;	//nope, get lost.
	if (!strncasecmp(xr_formfactor->string, "none", 4))
		qreqs->vrplatform = VR_HEADLESS;
	if (!strncasecmp(xr_formfactor->string, "fake", 4))
	{
		xr.fake = true;
		return true;
	}
	switch(qreqs->vrplatform)
	{
#ifdef XR_MND_HEADLESS_EXTENSION_NAME
	case VR_HEADLESS:
		ext = XR_MND_HEADLESS_EXTENSION_NAME;
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	case VR_VULKAN:
		ext = XR_KHR_VULKAN_ENABLE_EXTENSION_NAME;
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
#ifdef XR_MND_EGL_ENABLE_EXTENSION_NAME
	case VR_EGL:
		ext = XR_MND_EGL_ENABLE_EXTENSION_NAME;
		break;
#elif defined(XR_MNDX_EGL_ENABLE_EXTENSION_NAME)
	case VR_EGL:
		ext = XR_MNDX_EGL_ENABLE_EXTENSION_NAME;
		break;
#endif
#ifdef XR_USE_PLATFORM_XLIB
	case VR_X11_GLX:
#endif
#ifdef XR_USE_PLATFORM_WIN32
	case VR_WIN_WGL:
#endif
		ext = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_D3D11
	case VR_D3D11:
		ext = XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
		break;
#endif
	default:
		Con_Printf(CON_ERROR"OpenXR: windowing-api or rendering-api not supported\n");
		return false;
	}

	xr.instance = XR_NULL_HANDLE;

	if (xr_debug->ival)
	{
		uint32_t count, u;
		struct XrApiLayerProperties *props;
		if (XR_SUCCEEDED(xrEnumerateApiLayerProperties(0, &count, NULL)))
		{
			props = calloc(count, sizeof(*props));
			for (u = 0; u < count; u++)
				props[u].type = XR_TYPE_API_LAYER_PROPERTIES;
			xrEnumerateApiLayerProperties(count, &count, props);
			for (u = 0; u < count; u++)
				Con_Printf("OpenXR Layer %s: %s\n", props[u].layerName, props[u].description);

			if (xr_debug->ival>1)
			{
				for (u = 0; u < count; u++)
					if (!strcmp(props[u].layerName, "XR_APILAYER_LUNARG_core_validation"))
					{
						xrlayers[numlayers++] = "XR_APILAYER_LUNARG_core_validation";
						break;
					}
				if (u==count)
					Con_Printf("OpenXR: Validation layers not found\n");
			}
			free(props);
		}
	}

	{
		unsigned int exts = 0, u=0;
		XrExtensionProperties *extlist;
		res = xrEnumerateInstanceExtensionProperties(NULL, 0, &exts, NULL);
		if (res == XR_ERROR_RUNTIME_UNAVAILABLE || res == XR_ERROR_INSTANCE_LOST)
		{
			Con_Printf(CON_WARNING"OpenXR: no runtime installed\n");
			return false;
		}
		else if (XR_SUCCEEDED(res))
		{
			extlist = calloc(exts, sizeof(*extlist));
			for (u = 0; u < exts; u++)
				extlist[u].type = XR_TYPE_EXTENSION_PROPERTIES;
			xrEnumerateInstanceExtensionProperties(NULL, exts, &exts, extlist);

			//print a list of them all, if we can.
			if (xr_debug->ival)
			{
				Con_DPrintf("OpenXR:\n");
				for (u = 0; u < exts; u++)
					Con_DPrintf("\t%s\n", extlist[u].extensionName);
			}

			//make sure we have an appropriate extension for the API we're using.
			if (ext)
			{
				for (u = 0; u < exts; u++)
					if (!strcmp(extlist[u].extensionName, ext))
						break;
				if (u == exts)
				{
					Con_Printf(CON_ERROR"OpenXR: instance driver does not support required %s\n", ext);
					free(extlist);
					return false;	//would just give an error on xrCreateInstance anyway.
				}

				xrexts[numext++] = ext;
			}

			//look for some interesting extensions
			for (u = 0; u < exts; u++)
			{
				if (!strcmp(extlist[u].extensionName, XR_EXT_DEBUG_UTILS_EXTENSION_NAME) && !havedebugutils && xr_debug->ival)
					havedebugutils = true, xrexts[numext++] = XR_EXT_DEBUG_UTILS_EXTENSION_NAME;

#ifdef XR_EXT_hand_tracking
				if (!strcmp(extlist[u].extensionName, XR_EXT_HAND_TRACKING_EXTENSION_NAME) && !havehandtrack && xr_fingertracking->ival)
					havehandtrack = true, xrexts[numext++] = XR_EXT_HAND_TRACKING_EXTENSION_NAME;
#endif
			}

			free(extlist);
		}
		else
		{
			Con_Printf(CON_ERROR"OpenXR: xrEnumerateInstanceExtensionProperties failed (%s)\n", XR_StringForResult(res));
			return false;
		}
	}

	//create our instance
	{
		XrInstanceCreateInfo createinfo = {XR_TYPE_INSTANCE_CREATE_INFO};
		createinfo.createFlags = 0;
		Q_strlcpy(createinfo.applicationInfo.applicationName, FULLENGINENAME, sizeof(createinfo.applicationInfo.applicationName));
		createinfo.applicationInfo.applicationVersion = APPLICATIONVERSION;
		Q_strlcpy(createinfo.applicationInfo.engineName, "FTEQW", sizeof(createinfo.applicationInfo.engineName));
		createinfo.applicationInfo.engineVersion = ENGINEVERSION;
		createinfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
		createinfo.enabledApiLayerCount = numlayers;
		createinfo.enabledApiLayerNames = xrlayers;
		createinfo.enabledExtensionCount = numext;
		createinfo.enabledExtensionNames = xrexts;
		res = xrCreateInstance(&createinfo, &xr.instance);
	}
	if (res == XR_ERROR_RUNTIME_UNAVAILABLE || res == XR_ERROR_INSTANCE_LOST)
	{
		Con_Printf(CON_WARNING"OpenXR: no runtime installed\n");
		return false;
	}
	if (XR_FAILED(res) || !xr.instance)
	{
		Con_Printf(CON_ERROR"OpenXR Runtime: xrCreateInstance failed (%s)\n", XR_StringForResult(res));
		return false;
	}

	if (havedebugutils)
	{
		XrDebugUtilsMessengerEXT messenger1 = XR_NULL_HANDLE;
		XrDebugUtilsMessengerCreateInfoEXT cb = {XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
		cb.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		cb.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |  XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
		cb.userCallback = XR_DebugPrint;
		cb.userData = NULL;

		if (!XR_FAILED(xrGetInstanceProcAddr(xr.instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*) &xrCreateDebugUtilsMessengerEXT)))
			xrCreateDebugUtilsMessengerEXT(xr.instance, &cb, &messenger1);
	}

	if (xr_debug->ival)
	{
		XrInstanceProperties props = {XR_TYPE_INSTANCE_PROPERTIES};
		if (!XR_FAILED(xrGetInstanceProperties(xr.instance, &props)))
			Con_Printf("OpenXR Runtime: %s    %u.%u.%u\n", props.runtimeName, XR_VERSION_MAJOR(props.runtimeVersion), XR_VERSION_MINOR(props.runtimeVersion), XR_VERSION_PATCH(props.runtimeVersion));
		else
			Con_Printf("OpenXR Runtime: Unable to determine runtime version (%s)\n", XR_StringForResult(res));
	}

	{
		XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
		if (qreqs->vrplatform == VR_HEADLESS)
			systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;	//err... woteva
		else if (!strncasecmp(xr_formfactor->string, "hand", 4))
			systemInfo.formFactor = XR_FORM_FACTOR_HANDHELD_DISPLAY;
		else if (!strncasecmp(xr_formfactor->string, "head",4))
			systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		else
		{
			if (*xr_formfactor->string)
				Con_Printf("\"%s\" is not a recognised value for xr_formfactor\n", xr_formfactor->string);
			else
				Con_Printf("xr_formfactor not set, assuming headmounted\n");
			systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		}
		res = xrGetSystem(xr.instance, &systemInfo, &xr.systemid);
		if (XR_FAILED(res) || !xr.systemid)
			return false;
	}

	{
		XrSystemProperties props = {XR_TYPE_SYSTEM_PROPERTIES};
#ifdef XR_EXT_hand_tracking
		XrSystemHandTrackingPropertiesEXT handtrackprops = {XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
		if (havehandtrack)
		{	//instance might support it, but the specific hardware we're trying to use might not, in which case don't try using it after all.
			handtrackprops.next = props.next;
			props.next = &handtrackprops;
		}
#endif
		if (qreqs->vrplatform != VR_HEADLESS)
			if (XR_SUCCEEDED(xrGetSystemProperties(xr.instance, xr.systemid, &props)))
			{
				if (xr_debug->ival)
					Con_Printf("OpenXR System: %s\n", props.systemName);
			}

#ifdef XR_EXT_hand_tracking
		havehandtrack = handtrackprops.supportsHandTracking;
#endif
	}

#ifdef XR_EXT_hand_tracking
	if (havehandtrack)
	{
		xrGetInstanceProcAddr(xr.instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*) &xrCreateHandTrackerEXT);
		xrGetInstanceProcAddr(xr.instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*) &xrLocateHandJointsEXT);
		xrGetInstanceProcAddr(xr.instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*) &xrDestroyHandTrackerEXT);
	}
	else
	{
		xrCreateHandTrackerEXT = NULL;
		xrLocateHandJointsEXT = NULL;
		xrDestroyHandTrackerEXT = NULL;
	}
#endif

	switch(qreqs->vrplatform)
	{
	default:
		XR_Shutdown();
		return false;
	case VR_HEADLESS:
		break;

#ifdef XR_USE_GRAPHICS_API_VULKAN
	case VR_VULKAN:
		{
			XrGraphicsRequirementsVulkanKHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
			VkInstance inst = VK_NULL_HANDLE;
			VkPhysicalDevice physdev;
			uint32_t extlen;
			char *extstr;	//space-delimited list, for some reason. writable though.

			PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR;
			PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR;
			PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR;
			PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR;
			if (XR_FAILED(xrGetInstanceProcAddr(xr.instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsRequirementsKHR)) ||
				XR_FAILED(xrGetInstanceProcAddr(xr.instance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanInstanceExtensionsKHR)) ||
				XR_FAILED(xrGetInstanceProcAddr(xr.instance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanDeviceExtensionsKHR)) ||
				XR_FAILED(xrGetInstanceProcAddr(xr.instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsDeviceKHR)))
				return false;
			xrGetVulkanGraphicsRequirementsKHR(xr.instance, xr.systemid, &reqs);
			qreqs->maxver.major = XR_VERSION_MAJOR(reqs.maxApiVersionSupported);
			qreqs->maxver.minor = XR_VERSION_MINOR(reqs.maxApiVersionSupported);
			qreqs->minver.major = XR_VERSION_MAJOR(reqs.minApiVersionSupported);
			qreqs->minver.minor = XR_VERSION_MINOR(reqs.minApiVersionSupported);

			xrGetVulkanInstanceExtensionsKHR(xr.instance, xr.systemid, 0, &extlen, NULL);
			extstr = malloc(extlen);
			xrGetVulkanInstanceExtensionsKHR(xr.instance, xr.systemid, extlen, &extlen, extstr);

			//create vulkan instance now...
			qreqs->createinstance(qreqs, extstr, &inst);
			free(extstr);

			xrGetVulkanDeviceExtensionsKHR(xr.instance, xr.systemid, 0, &extlen, NULL);
			extstr = malloc(extlen);
			xrGetVulkanDeviceExtensionsKHR(xr.instance, xr.systemid, extlen, &extlen, extstr);

			res = xrGetVulkanGraphicsDeviceKHR(xr.instance, xr.systemid, inst, &physdev);

			qreqs->deviceextensions = extstr;
			qreqs->vk.physicaldevice = physdev;
		}
		break;
#endif

#ifdef XR_USE_GRAPHICS_API_OPENGL
	case VR_X11_GLX:
	case VR_EGL:
	case VR_WIN_WGL:
		{
			XrGraphicsRequirementsOpenGLKHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
			PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR;
			if (XR_SUCCEEDED(xrGetInstanceProcAddr(xr.instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetOpenGLGraphicsRequirementsKHR)))
				xrGetOpenGLGraphicsRequirementsKHR(xr.instance, xr.systemid, &reqs);

			qreqs->maxver.major = XR_VERSION_MAJOR(reqs.maxApiVersionSupported);
			qreqs->maxver.minor = XR_VERSION_MINOR(reqs.maxApiVersionSupported);
			qreqs->minver.major = XR_VERSION_MAJOR(reqs.minApiVersionSupported);
			qreqs->minver.minor = XR_VERSION_MINOR(reqs.minApiVersionSupported);
			//caller must validate when creating its context.
		}
		break;
#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
	case VR_D3D11:
		{
			XrGraphicsRequirementsD3D11KHR reqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
			PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR;
			if (XR_SUCCEEDED(xrGetInstanceProcAddr(xr.instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetD3D11GraphicsRequirementsKHR)))
				xrGetD3D11GraphicsRequirementsKHR(xr.instance, xr.systemid, &reqs);

			qreqs->minver.major = reqs.minFeatureLevel;
			qreqs->deviceid[0] = reqs.adapterLuid.LowPart;
			qreqs->deviceid[1] = reqs.adapterLuid.HighPart;
		}
		break;
#endif
	}

	{
		XrViewConfigurationType *viewtype;
		uint32_t viewtypes, u;
		res = xrEnumerateViewConfigurations(xr.instance, xr.systemid, 0, &viewtypes, NULL);
		viewtype = alloca(viewtypes*sizeof(viewtype));
		res = xrEnumerateViewConfigurations(xr.instance, xr.systemid, viewtypes, &viewtypes, viewtype);
		xr.viewtype = (XrViewConfigurationType)0;
		for (u = 0; u < viewtypes; u++)
		{
			switch(viewtype[u])
			{
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
				if (!strcasecmp(xr_viewconfig->string, "mono"))
					xr.viewtype = viewtype[u];
				break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
				if (!strcasecmp(xr_viewconfig->string, "stereo"))
					xr.viewtype = viewtype[u];
				break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
				if (!strcasecmp(xr_viewconfig->string, "quad"))
					xr.viewtype = viewtype[u];
				break;
			default:
				break;
			}
		}
		if (!xr.viewtype)
		{
			if (viewtypes)
				xr.viewtype = viewtype[0];

			if (*xr_viewconfig->string)
			{
				Con_Printf("OpenXR: Viewtype %s unavailable, using ", xr_viewconfig->string);
				switch(xr.viewtype)
				{
				case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:		Con_Printf("mono\n"); break;
				case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:		Con_Printf("stereo\n"); break;
				case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:	Con_Printf("quad\n"); break;
				default:
					Con_Printf("unknown (%i)\n", xr.viewtype);
					break;
				}
			}
		}
	}

	if (qreqs->vrplatform == VR_HEADLESS)
		return true;

	res = xrEnumerateViewConfigurationViews(xr.instance, xr.systemid, xr.viewtype, 0, &xr.viewcount, NULL);
	if (xr.viewcount > MAX_VIEW_COUNT)
		xr.viewcount = MAX_VIEW_COUNT;	//oh noes! evile!
	xr.views = calloc(1,sizeof(*xr.views)*xr.viewcount);
	{
		uint32_t u;
		for (u = 0; u < xr.viewcount; u++)
			xr.views[u].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
	}
	res = xrEnumerateViewConfigurationViews(xr.instance, xr.systemid, xr.viewtype, xr.viewcount, &xr.viewcount, xr.views);

	//caller now knows what device/contextversion/etc to init with
	return true;
}

static qboolean XR_Init(vrsetup_t *qreqs, rendererstate_t *info)
{
	if (xr.fake)
		return true;	//nothing to do here.

	xr.srgb = info->srgb;
	switch(qreqs->vrplatform)
	{
	case VR_HEADLESS:
		break;
	default:
		return false;	//error. not supported in this build.
#ifdef XR_USE_GRAPHICS_API_VULKAN
	case VR_VULKAN:
		{
			XrGraphicsBindingVulkanKHR *vk = xr.bindinginfo = calloc(1, sizeof(*vk));
			vk->type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
			vk->instance = qreqs->vk.instance;
			vk->physicalDevice = qreqs->vk.physicaldevice;
			vk->device = qreqs->vk.device;
			vk->queueFamilyIndex = qreqs->vk.queuefamily;
			vk->queueIndex = qreqs->vk.queueindex;
			xr.renderer = QR_VULKAN;
		}
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
#ifdef XR_MND_EGL_ENABLE_EXTENSION_NAME
	case VR_EGL:	//x11-egl, wayland, and hopefully android...
		{
			XrGraphicsBindingEGLMND *egl = xr.bindinginfo = calloc(1, sizeof(*egl));
			egl->type = XR_TYPE_GRAPHICS_BINDING_EGL_MND;
			egl->getProcAddress = qreqs->egl.getprocaddr;
			egl->display = qreqs->egl.egldisplay;
			egl->config = qreqs->egl.eglconfig;
			egl->context = qreqs->egl.eglcontext;
			xr.renderer = QR_OPENGL;
		}
		break;
#elif defined(XR_MNDX_EGL_ENABLE_EXTENSION_NAME)
	case VR_EGL:	//x11-egl, wayland, and hopefully android...
		{
			XrGraphicsBindingEGLMNDX *egl = xr.bindinginfo = calloc(1, sizeof(*egl));
			egl->type = XR_TYPE_GRAPHICS_BINDING_EGL_MNDX;
			egl->getProcAddress = (PFNEGLGETPROCADDRESSPROC)qreqs->egl.getprocaddr;
			egl->display = qreqs->egl.egldisplay;
			egl->config = qreqs->egl.eglconfig;
			egl->context = qreqs->egl.eglcontext;
			xr.renderer = QR_OPENGL;
		}
		break;
#endif
#ifdef XR_USE_PLATFORM_XLIB
	case VR_X11_GLX:
		{
			XrGraphicsBindingOpenGLXlibKHR *glx = xr.bindinginfo = calloc(1, sizeof(*glx));
			glx->type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
			glx->xDisplay = qreqs->x11_glx.display;
			glx->visualid = qreqs->x11_glx.visualid;
			glx->glxFBConfig = qreqs->x11_glx.glxfbconfig;
			glx->glxDrawable = qreqs->x11_glx.drawable;
			glx->glxContext = qreqs->x11_glx.glxcontext;
			xr.renderer = QR_OPENGL;
		}
		break;
#endif
#ifdef XR_USE_PLATFORM_WIN32
	case VR_WIN_WGL:
		{
			XrGraphicsBindingOpenGLWin32KHR *wgl = xr.bindinginfo = calloc(1, sizeof(*wgl));
			wgl->type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
			wgl->hDC = qreqs->wgl.hdc;
			wgl->hGLRC = qreqs->wgl.hglrc;
			xr.renderer = QR_OPENGL;
		}
		break;
#endif
#endif	//def XR_USE_GRAPHICS_API_OPENGL
#ifdef XR_USE_GRAPHICS_API_D3D11
	case VR_D3D11:
		{
			XrGraphicsBindingD3D11KHR *d3d = xr.bindinginfo = calloc(1, sizeof(*d3d));
			d3d->type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
			d3d->device = qreqs->d3d.device;
			xr.renderer = QR_DIRECT3D11;
		}
		break;
#endif
	}


	XR_SetupInputs_Instance();
	return true;
}

static void XR_HapticCommand_f(void)
{
	size_t u;
	char actionname[XR_MAX_ACTION_NAME_SIZE];
	cmdfuncs->Argv(0, actionname, sizeof(actionname));
	for (u = 0; u < xr.numactions; u++)
	{
		if (!strcasecmp(xr.actions[u].actname, actionname) && xr.actions[u].acttype == XR_ACTION_TYPE_VIBRATION_OUTPUT)
		{
			if (xr.session)
			{
				XrHapticActionInfo info = {XR_TYPE_HAPTIC_ACTION_INFO};
				XrHapticVibration haptic = {XR_TYPE_HAPTIC_VIBRATION};
				info.action = xr.actions[u].action;
				info.subactionPath = XR_NULL_PATH;

				cmdfuncs->Argv(1, actionname, sizeof(actionname));
				haptic.duration = *actionname?SECONDS_TO_NANOSECONDS(atof(actionname)):XR_MIN_HAPTIC_DURATION;
				cmdfuncs->Argv(2, actionname, sizeof(actionname));
				haptic.amplitude = *actionname?atof(actionname):0;
				cmdfuncs->Argv(3, actionname, sizeof(actionname));
				haptic.frequency = *actionname?atof(actionname):XR_FREQUENCY_UNSPECIFIED;
				xrApplyHapticFeedback(xr.session, &info, (XrHapticBaseHeader*)&haptic);
			}
			break;
		}
	}
}

static XrAction XR_DefineAction(enum actset_e set, XrActionType type, const char *name, const char *description, const char *root)
{
	XrActionCreateInfo info = {XR_TYPE_ACTION_CREATE_INFO};
	XrResult res;
	char *ffs;

	size_t u;
	int nconflicts = 0;
	int dconflicts = 0;
	for (u = 0; u < xr.numactions; u++)
	{
		if (xr.actions[u].set != set)
			continue;
		if ((xr.actions[u].acttype == type || !type) && !strcmp(xr.actions[u].actname, name) /*&& !strcmp(xr.actions[u].actdescription, description)*/ && !strcmp(xr.actions[u].subactionpath?xr.actions[u].subactionpath:"", root?root:""))
		{	//looks like a dupe...
			return xr.actions[u].action;
		}
		if (!strcasecmp(xr.actions[u].actname, name))
			nconflicts++;	//arse balls knob cock
		if (description && !strcasecmp(xr.actions[u].actdescription, description))
			dconflicts++;	//arse balls knob cock
	}
	if (!description)
		return XR_NULL_HANDLE;	//none found
	if (u == xr.maxactions)
	{
		size_t nm = (xr.maxactions+1)*2;
		void *n = plugfuncs->Realloc(xr.actions, sizeof(*xr.actions) * nm);
		if (!n)
			return XR_NULL_HANDLE;	//erk!
		xr.actions = n;
		xr.maxactions = nm;
	}

	memset(&xr.actions[u], 0, sizeof(xr.actions[u]));
	xr.actions[u].set = set;
	xr.actions[u].acttype = type;
	xr.actions[u].actname = strdup(name);
	xr.actions[u].actdescription = strdup(description);
	xr.actions[u].subactionpath = (root&&*root)?strdup(root):NULL;
	xr.numactions++;

	if (xr.actions[u].subactionpath)
		xrStringToPath(xr.instance, xr.actions[u].subactionpath, &xr.actions[u].path);
	else
		xr.actions[u].path = XR_NULL_PATH;
	if (xr.actions[u].path == XR_NULL_PATH)
	{
		info.countSubactionPaths = 0;
		info.subactionPaths = NULL;
	}
	else
	{
		info.countSubactionPaths = 1;
		info.subactionPaths = &xr.actions[u].path;
	}
	info.actionType = xr.actions[u].acttype;
	Q_strlcpy(info.actionName, xr.actions[u].actname, sizeof(info.actionName));
	for (ffs = info.actionName; *ffs; ffs++)
	{
		if (*ffs >= 'A' && *ffs < 'Z')
			*ffs += 'a'-'A';	//must be lower-case
		else if (*ffs >= 'a' && *ffs <= 'z')
			;	//allowed
		else if (*ffs >= '0' && *ffs <= '9')
			;	//allowed
		else if (*ffs == '.' || *ffs == '-' || *ffs == '_')
			;	//allowed
		// '/' is not allowed as it must be a single segment.
		else
			*ffs = '_';	//everything else is blocked
	}
	Q_strlcpy(info.localizedActionName, xr.actions[u].actdescription, sizeof(info.localizedActionName));
	res = xrCreateAction(xr.actionset[set].actionSet, &info, &xr.actions[u].action);
	if (XR_FAILED(res))
		Con_Printf("openxr: Unable to create action %s [%s] - %s\n", info.actionName, info.localizedActionName, XR_StringForResult(res));

	if (info.actionType == XR_ACTION_TYPE_VIBRATION_OUTPUT)
		cmdfuncs->AddCommand(xr.actions[u].actname, XR_HapticCommand_f, "Linked to an OpenXR haptic feedback.");

	return xr.actions[u].action;
}

static qboolean XR_ReadLine(const char **text, char *buffer, size_t buflen)
{
	char in;
	char *out = buffer;
	size_t len;
	if (buflen <= 1)
		return false;
	len = buflen-1;
	while (len > 0)
	{
		in = *(*text);
		if (!in)
		{
			if (len == buflen-1)
				return false;
			*out = 0;
			return true;
		}
		(*text)++;
		if (in == '\n')
			break;
		*out++ = in;
		len--;
	}
	*out = '\0';

	//if there's a trailing \r, strip it.
	if (out > buffer)
		if (out[-1] == '\r')
			out[-1] = 0;

	return true;
}

static int XR_BindProfileStr(const char *fname, const char *file)
{
	XrAction act;
	XrResult res;
	XrPath profilepath = XR_NULL_PATH;
	char line[1024], *linestart;
	char name[1024];
	char type[256];
	char desc[1024];
	char bind[1024];
	char root[1024];
	unsigned int p;
	char prefix[2][1024];
	enum actset_e set;

	XrInteractionProfileSuggestedBinding suggestedbindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
	unsigned int acts = 0;
	XrActionSuggestedBinding bindings[256];
	unsigned int totalacts = 0;

	while (XR_ReadLine(&file, line, sizeof(line)))
	{
		set = AS_COMMON;
		linestart = line;
		while (*linestart == ' ' || *linestart == '\t')
			linestart++;

		if (!strncasecmp(linestart, "menu:", 5))
		{
			set = AS_MENU;
			linestart+=5;
		}
		else if (!strncasecmp(linestart, "game:", 5))
		{
			set = AS_GAME;
			linestart+=5;
		}
		else if (!strncasecmp(linestart, "common:", 7))
		{	//the default, but nice to be able to be explicit (especially if you want a colon in the action name).
			set = AS_COMMON;
			linestart+=7;
		}

		cmdfuncs->TokenizeString(linestart);
		if (cmdfuncs->Argc())
		{
			cmdfuncs->Argv(0, name, sizeof(name));

			if (!strcasecmp(name, "dev"))
			{
				cmdfuncs->Argv(1, root, sizeof(root));
				continue;
			}
			else if (!strcasecmp(name, "profile"))
			{
				if (acts)
				{
					suggestedbindings.interactionProfile = profilepath;
					suggestedbindings.countSuggestedBindings = acts;
					suggestedbindings.suggestedBindings = bindings;
					res = xrSuggestInteractionProfileBindings(xr.instance, &suggestedbindings);
					if (XR_FAILED(res))
						Con_Printf(CON_ERROR"%s: xrSuggestInteractionProfileBindings failed - %s\n", fname, XR_StringForResult(res));
					totalacts += acts;
					acts = 0;
				}

				cmdfuncs->Argv(1, name, sizeof(name));
				for (p = 0; p < countof(prefix); p++)
					cmdfuncs->Argv(2+p, prefix[p], sizeof(prefix[p]));
				*root = 0;
				if (XR_FAILED(xrStringToPath(xr.instance, name, &profilepath)))
					profilepath = XR_NULL_PATH;
				continue;
			}
			else if (!strcasecmp(name, "action"))
			{
				cmdfuncs->Argv(1, name, sizeof(name));
				cmdfuncs->Argv(2, desc, sizeof(desc));
				cmdfuncs->Argv(3, type, sizeof(type));
				cmdfuncs->Argv(4, bind, sizeof(bind));
			}
			else if (cmdfuncs->Argc() == 2)
			{
				*desc = 0;
				*type = 0;
				cmdfuncs->Argv(1, bind, sizeof(desc));
			}
			else
			{
				if (cmdfuncs->Argc() < 4)
				{
					Con_Printf("Unknown action command \"%s\"\n", name);
					continue;
				}
				cmdfuncs->Argv(1, desc, sizeof(desc));
				cmdfuncs->Argv(2, type, sizeof(type));
				cmdfuncs->Argv(3, bind, sizeof(bind));
				if (cmdfuncs->Argc() >= 5)
					Con_Printf("%s: %s: Extra tokens found\n", fname, name);
			}

			if (*type)
			{
				XrActionType xrtype;
				if (!strcasecmp(type, "button"))
					xrtype = XR_ACTION_TYPE_BOOLEAN_INPUT;
				else if (!strcasecmp(type, "float"))
					xrtype = XR_ACTION_TYPE_FLOAT_INPUT;
				else if (!strcasecmp(type, "vector2f"))
					xrtype = XR_ACTION_TYPE_VECTOR2F_INPUT;
				else if (!strcasecmp(type, "pose"))
					xrtype = XR_ACTION_TYPE_POSE_INPUT;
				else if (!strcasecmp(type, "vibration") || !strcasecmp(type, "haptic"))
					xrtype = XR_ACTION_TYPE_VIBRATION_OUTPUT;
				else
					continue;

				//define our action...
				act = XR_DefineAction(set, xrtype, name, desc, root);
			}
			else
			{
				act = XR_DefineAction(set, (XrActionType)0, name, NULL, root);
				if(act==XR_NULL_HANDLE)
					Con_Printf("Action %s not defined yet\n", name);
			}

			//and add it to the profile we're building.
			if (act != XR_NULL_HANDLE && *bind && profilepath!=XR_NULL_PATH)
			{
				if (*bind == '/')
				{
					res = xrStringToPath(xr.instance, bind, &bindings[acts].binding);
					if (XR_SUCCEEDED(res) && acts < countof(bindings))
						bindings[acts++].action = act;
				}
				else if (*root == '/')
				{
					res = xrStringToPath(xr.instance, va("%s/%s", root, bind), &bindings[acts].binding);
					if (XR_SUCCEEDED(res) && acts < countof(bindings))
						bindings[acts++].action = act;
				}
				else for (p = 0; p < countof(prefix) && *prefix[p]=='/'; p++)
				{
					res = xrStringToPath(xr.instance, va("%s%s", prefix[p], bind), &bindings[acts].binding);
					if (XR_SUCCEEDED(res) && acts < countof(bindings))
						bindings[acts++].action = act;
				}
			}
		}
	}

	if (acts)
	{
		suggestedbindings.interactionProfile = profilepath;
		suggestedbindings.countSuggestedBindings = acts;
		suggestedbindings.suggestedBindings = bindings;
		res = xrSuggestInteractionProfileBindings(xr.instance, &suggestedbindings);
		if (XR_FAILED(res))
			Con_Printf(CON_ERROR"%s: xrSuggestInteractionProfileBindings failed - %s\n", fname, XR_StringForResult(res));
		totalacts += acts;
	}
	return totalacts;
}

static int QDECL XR_BindProfileFile(const char *fname, qofs_t fsize, time_t mtime, void *ctx, struct searchpathfuncs_s *package)
{
	vfsfile_t *f = fsfuncs->OpenVFS(fname, "rb", FS_GAME);
	if (f)
	{
		size_t len = VFS_GETLEN(f);
		char *buf = malloc(len+1);
		VFS_READ(f, buf, len);
		buf[len] = 0;
		*(unsigned int*)ctx += XR_BindProfileStr(fname, buf);
		free(buf);
		VFS_CLOSE(f);
	}
	return true;
}

static const struct
{
	const char *name;
	const char *script;
} xr_knownprofiles[] =
{
	//FIXME: set up some proper bindings!
	{"khr_simple",		"profile /interaction_profiles/khr/simple_controller    /user/hand/left/ /user/hand/right/\n"
			"dev /user/hand/left\n"
				"+attack_left			\"Left Attack\"		button	input/select/click\n"
				"+menu_left				\"Left Menu\"		button	input/menu/click\n"
				"#POSE_LEFT				\"Left Aim Pose\"	pose	input/aim/pose\n"
				//"left_grip			\"Left Grip Pose\"	pose	input/grip/pose\n"
				"haptic_left			\"Left Haptic\"		haptic	output/haptic\n"
			"dev /user/hand/right\n"
				"menu: #GP_START		\"Menu Enter\"		button	input/select/click\n"
				"menu: #GP_BACK			\"Menu Escape\"		button	input/menu/click\n"
				"game: +attack_right	\"Right Attack\"	button	input/select/click\n"
				"game: +menu_right		\"Right Menu\"		button	input/menu/click\n"
				"#POSE_RIGHT			\"Right Aim Pose\"	pose	input/aim/pose\n"
				//"right_grip			\"Right Grip Pose\"	pose	input/grip/pose\n"
				"haptic_right			\"Right Haptic\"	haptic	output/haptic\n"
	},

/*	{"valve_index",		"profile /interaction_profiles/valve/index_controller    /user/hand/left/ /user/hand/right/\n"
			//"unbound		\"Unused Button\"		button		input/system/click\n"
			//"unbound		\"Unused Button\"		button		input/system/touch\n"
			//"unbound		\"Unused Button\"		button		input/a/click\n"
			//"unbound		\"Unused Button\"		button		input/a/touch\n"
			//"unbound		\"Unused Button\"		button		input/b/click\n"
			//"unbound		\"Unused Button\"		button		input/b/touch\n"
			//"unbound		\"Unused Button\"		float		input/squeeze/value\n"
			//"unbound		\"Unused Button\"		button		input/squeeze/force\n"
			//"unbound		\"Unused Button\"		button		input/trigger/click\n"
			//"unbound		\"Unused Button\"		float		input/trigger/value\n"
			//"unbound		\"Unused Button\"		button		input/trigger/touch\n"
			//"unbound		\"Unused Button\"		vector2f	input/thumbstick\n"
			//"unbound		\"Unused Button\"		button		input/thumbstick/click\n"
			//"unbound		\"Unused Button\"		button		input/thumbstick/touch\n"
			//"unbound		\"Unused Button\"		vector2f	input/trackpad\n"
			//"unbound		\"Unused Button\"		button		input/trackpad/force\n"
			//"unbound		\"Unused Button\"		button		input/trackpad/touch\n"
			//"unbound		\"Unused Button\"		pose		input/grip/pose\n"
			//"unbound		\"Unused Button\"		pose		input/aim/pose\n"
			//"unbound		\"Unused Button\"		haptic		output/haptic\n"
	},
*/
/*	{"htc_vive",		"profile /interaction_profiles/htc/vive_controller    /user/hand/left/ /user/hand/right/\n"
			//"unbound		\"Unused Button\"		button		input/system/click\n"
			//"unbound		\"Unused Button\"		button		input/squeeze/click\n"
			//"unbound		\"Unused Button\"		button		input/menu/click\n"
			//"unbound		\"Unused Button\"		button		input/trigger/click\n"
			//"unbound		\"Unused Button\"		float		input/trigger/value\n"
			//"unbound		\"Unused Button\"		vector2f	input/trackpad\n"
			//"unbound		\"Unused Button\"		button		input/trackpad/click\n"
			//"unbound		\"Unused Button\"		button		input/trackpad/touch\n"
			//"unbound		\"Unused Button\"		pose		input/grip/pose\n"
			//"unbound		\"Unused Button\"		pose		input/aim/pose\n"
			//"unbound		\"Unused Button\"		haptic		output/haptic\n"
			);
*/
/*	{"htc_vive_pro",	"profile /interaction_profiles/htc/vive_pro    /user/head/\n"
			//"unbound		\"Unused Button\"		button		input/system/click\n"
			//"unbound		\"Unused Button\"		button		input/volume_up/click\n"
			//"unbound		\"Unused Button\"		button		input/volume_down/click\n"
			//"unbound		\"Unused Button\"		button		input/mute_mic/click\n"
			);
*/

	//just map everything to quake's various buttons so that mods with proper gamepad mapping will work here too.
	{"gamepad", "profile /interaction_profiles/microsoft/xbox_controller    /user/gamepad/\n"
			"#GP_START				\"Start\"				button		input/menu/click\n"
			"#GP_BACK				\"Back\"				button		input/view/click\n"
			"#GP_A					\"A Button\"			button		input/a/click\n"
			"#GP_B					\"B Button\"			button		input/b/click\n"
			"#GP_X					\"X Button\"			button		input/x/click\n"
			"#GP_Y					\"Y Button\"			button		input/y/click\n"
			"#GP_DPAD_DOWN			\"Move Backwards\"		button		input/dpad_down/click\n"
			"#GP_DPAD_RIGHT			\"Move Right\"			button		input/dpad_right/click\n"
			"#GP_DPAD_UP			\"Move Forward\"		button		input/dpad_up/click\n"
			"#GP_DPAD_LEFT			\"Move Left\"			button		input/dpad_left/click\n"
			"#GP_LSHOULDER			\"Jump\"				button		input/shoulder_left/click\n"
			"#GP_RSHOULDER			\"Attack\"				button		input/shoulder_right/click\n"
			"#GP_LTHUMB				\"Left Thumb\"			button		input/thumbstick_left/click\n"
			"#GP_RTHUMB				\"Right Thumb\"			button		input/thumbstick_right/click\n"
			"#GP_AXIS_LTRIGGER		\"Left Trigger\"		float		input/trigger_left/value\n"
			"#GP_AXIS_RTRIGGER		\"Right Trigger\"		float		input/trigger_right/value\n"
			"#GP_AXIS_LEFT_X		\"Left Thumbstick X\"	float		input/thumbstick_left/x\n"
			"#GP_AXIS_LEFT_Y		\"Left Thumbstick y\"	float		input/thumbstick_left/y\n"
			"#GP_AXIS_RIGHT_X		\"Right Thumbstick X\"	float		input/thumbstick_right/x\n"
			"#GP_AXIS_RIGHT_X		\"Right Thumbstick Y\"	float		input/thumbstick_right/y\n"
			"haptic_gp_left			\"Left Haptic (Main)\"		haptic	output/haptic_left\n"
			"haptic_gp_left_trigger	\"Left-Trigger Haptic\"		haptic	output/haptic_left_trigger\n"
			"haptic_gp_right		\"Right Haptic (Main)\"		haptic	output/haptic_right\n"
			"haptic_gp_right_trigger \"Right-Trigger Haptic\"	haptic	output/haptic_right_trigger\n"
	},
};

static void XR_SetupInputs_Instance(void)
{
	unsigned int h;
	XrResult res;
	int set;
	char *actionsetNames[] = {"genericactions", "menuactions", "gameactions"};
	char *actionsetNamesText[] = {"Generic Actions", "Menu Actions", "Game Actions"};

	for (set = 0; set < MAX_ACTIONSETS; set++)
	{
		XrActionSetCreateInfo info = {XR_TYPE_ACTION_SET_CREATE_INFO};
		Q_strlcpy(info.actionSetName, actionsetNames[set], sizeof(info.actionSetName));
		Q_strlcpy(info.localizedActionSetName, actionsetNamesText[set], sizeof(info.localizedActionSetName));
		info.priority = 0;

		xr.actionset[set].subactionPath = XR_NULL_PATH;
		res = xrCreateActionSet(xr.instance, &info, &xr.actionset[set].actionSet);
		if (XR_FAILED(res))
			Con_Printf("openxr: Unable to create actionset - %s\n", XR_StringForResult(res));
	}

	h = 0;
	if (fsfuncs)
		fsfuncs->EnumerateFiles(FS_GAME, "oxr_*.binds", XR_BindProfileFile, &h);
	if (!h)	//no user bindings defined, use fallbacks. probably this needs to be per-mod.
	{
		for (h = 0; h < countof(xr_knownprofiles); h++)
			XR_BindProfileStr(xr_knownprofiles[h].name, xr_knownprofiles[h].script);
	}
}
static void XR_SetupInputs_Session(void)
{
	unsigned int h;
	XrResult res;

	//create action space stuff.
	for (h = 0; h < xr.numactions; h++)
	{
		switch(xr.actions[h].acttype)
		{
		case XR_ACTION_TYPE_POSE_INPUT:
			{
				XrActionSpaceCreateInfo info = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
				info.action = xr.actions[h].action;
				info.subactionPath = xr.actions[h].path;
				info.poseInActionSpace.orientation.w = 1;	//just fill with identity.

				res = xrCreateActionSpace(xr.session, &info, &xr.actions[h].space);
				if (XR_FAILED(res))
					Con_Printf("openxr: xrCreateActionSpace failed - %s\n", XR_StringForResult(res));
			}
			break;
		default:
			xr.actions[h].space = XR_NULL_HANDLE;
			break;
		}
	}

	//and attach it.
	{
		XrSessionActionSetsAttachInfo info = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		XrActionSet sets[MAX_ACTIONSETS];
		unsigned int set;
		for (set = 0; set < MAX_ACTIONSETS; set++)
			if (xr.actionset[set].actionSet)
				sets[info.countActionSets++] = xr.actionset[set].actionSet;
		info.actionSets = sets;
		res = xrAttachSessionActionSets(xr.session, &info);
		if (XR_FAILED(res))
			Con_Printf("openxr: xrAttachSessionActionSets failed - %s\n", XR_StringForResult(res));
	}

#ifdef XR_EXT_hand_tracking
	//create some hand trackers... try to create one for each hand...
	//(note: this is more a finger-joint tracker than a hand tracker, though limited controllers generally mean its good only for fingers)
	if (xrCreateHandTrackerEXT)
	for (h = 0; h < 2; h++)
	{
		XrHandTrackerCreateInfoEXT info = {XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
		info.hand = h?XR_HAND_RIGHT_EXT:XR_HAND_LEFT_EXT;
		info.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		res = xrCreateHandTrackerEXT(xr.session, &info, &xr.hand[h].handle);
		if (XR_FAILED(res))
			Con_Printf("openxr: xrCreateHandTrackerEXT failed - %s\n", XR_StringForResult(res));
	}
#endif
}

static void XR_PrintInputs(void)
{
	XrResult res;
	XrInteractionProfileState profile = {XR_TYPE_INTERACTION_PROFILE_STATE};
	XrPath path;
	unsigned int u;
	static const char *paths[] = {"/user/hand/left", "/user/hand/right", "/user/head", "/user/gamepad"};
	Con_Printf("OpenXR Interaction Profiles:\n");
	for (u = 0; u < countof(paths); u++)
	{
		xrStringToPath(xr.instance, paths[u], &path);
		res = xrGetCurrentInteractionProfile(xr.session, path, &profile);
		if (XR_SUCCEEDED(res))
		{
			char buf[256];
			uint32_t len = sizeof(buf);
			if (!profile.interactionProfile)
				Con_Printf("\t%s: "S_COLOR_GRAY"no profile/device\n", paths[u]);
			else
			{
				res = xrPathToString(xr.instance, profile.interactionProfile, sizeof(buf), &len, buf);
				Con_Printf("\t%s: "S_COLOR_GREEN"%s\n", paths[u], buf);
			}
		}
	}


	Con_Printf("Bound actions:\n");
	for (u = 0; u < xr.numactions; u++)
	{
		XrBoundSourcesForActionEnumerateInfo info = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
		uint32_t inputs, i, bufsize;
		XrPath input[20];
		info.action = xr.actions[u].action;
		res = xrEnumerateBoundSourcesForAction(xr.session, &info, countof(input), &inputs, input);
		if (XR_SUCCEEDED(res))
		{
			Con_Printf("\t%s "S_COLOR_GRAY"(%s)"S_COLOR_WHITE":\n", xr.actions[u].actname, xr.actions[u].actdescription);
			if (!inputs)
				Con_Printf(S_COLOR_GRAY"\t\t(unbound)\n");
			else for (i = 0; i < inputs; i++)
			{
				char buffer[8192];
				XrInputSourceLocalizedNameGetInfo info = {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
				info.sourcePath = input[i];
				info.whichComponents =	XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT|	//'left hand'
										XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT|	//'foo controller'
										XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;	//'trigger'
				res = xrGetInputSourceLocalizedName(xr.session, &info, sizeof(buffer), &bufsize, buffer);
				if (XR_FAILED(res))
					Q_snprintf(buffer, sizeof(buffer), S_COLOR_RED"error %i", res);
				else
					Con_Printf(S_COLOR_GREEN"\t\t%s\n", buffer);
			}
		}
		else if (res == XR_ERROR_HANDLE_INVALID)	//monado reports this for unimplemented things.
			Con_Printf(S_COLOR_RED"\t%s: error XR_ERROR_HANDLE_INVALID (not implemented?)\n", xr.actions[u].actname);
		else
			Con_Printf(S_COLOR_RED"\t%s: error %s\n", xr.actions[u].actname, XR_StringForResult(res));
	}
}

static void XR_UpdateInputs(XrTime time)
{
	XrResult res;
	size_t h;
	qboolean activesets[MAX_ACTIONSETS] = {true};

	if (inputfuncs->GetKeyDest() & ~kdm_game)
		activesets[AS_MENU] = true;	//used while looking at the menuqc/emenus/console/etc.
	else
		activesets[AS_GAME] = true;	//used while the game has focus.

	{
		XrActionsSyncInfo syncinfo = {XR_TYPE_ACTIONS_SYNC_INFO};
		XrActiveActionSet sets[MAX_ACTIONSETS];
		for (h = 0; h < MAX_ACTIONSETS; h++)
			if (activesets[h])
				sets[syncinfo.countActiveActionSets++] = xr.actionset[h];
		syncinfo.activeActionSets = sets;
		res = xrSyncActions(xr.session, &syncinfo);
		if (res == XR_SESSION_NOT_FOCUSED)
			;	//handle it anyway, giving us a chance to disable various inputs.
		else if (XR_FAILED(res))
			return;
	}

	for (h = 0; h < xr.numactions; h++)
	{
		if (xr.actions[h].action == XR_NULL_HANDLE)	//failed to init
			continue;
		safeswitch(xr.actions[h].acttype)
		{
		case XR_ACTION_TYPE_POSE_INPUT:
			{
				XrActionStatePose pose = {XR_TYPE_ACTION_STATE_POSE};
				XrActionStateGetInfo info = {XR_TYPE_ACTION_STATE_GET_INFO};
				info.action = xr.actions[h].action;
				info.subactionPath = xr.actions[h].path;

				if (XR_FAILED(xrGetActionStatePose(xr.session, &info, &pose)))
					break;
				if (pose.isActive && activesets[xr.actions[h].set])
				{	//its mapped to something, woo.
					XrSpaceVelocity vel = {XR_TYPE_SPACE_VELOCITY};
					XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION, &vel};
					vec3_t angles, org, lvel, avel;
					res = xrLocateSpace(xr.actions[h].space, xr.space, time, &loc);
					XR_PoseToAngOrg(&loc.pose, angles, org);

					VectorSet(lvel, vel.linearVelocity.x, vel.linearVelocity.y, vel.linearVelocity.z);
					VectorSet(avel, vel.angularVelocity.x, vel.angularVelocity.y, vel.angularVelocity.z);
					if (!strncasecmp(xr.actions[h].actname, "#POSE_", 6))
					{
						if (inputfuncs->SetHandPosition(xr.actions[h].actname+6,
								(loc.locationFlags&XR_SPACE_LOCATION_POSITION_VALID_BIT)?org:NULL,
								(loc.locationFlags&XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)?angles:NULL,
								(vel.velocityFlags&XR_SPACE_VELOCITY_LINEAR_VALID_BIT)?lvel:NULL,
								(vel.velocityFlags&XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)?avel:NULL))
							;
						else
							Con_Printf("Pose \"%s\" for action \"%s\" is not a known pose name\n", xr.actions[h].actname+1, xr.actions[h].actdescription);
					}
					else
					{	//custom poses that mods might want to handle themselves...
						char cmd[256];
						unsigned int status = 0;
						status |= (loc.locationFlags&XR_SPACE_LOCATION_POSITION_VALID_BIT)?VRSTATUS_ORG:0;
						status |= (loc.locationFlags&XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)?VRSTATUS_ANG:0;
						status |= (vel.velocityFlags&XR_SPACE_VELOCITY_LINEAR_VALID_BIT)?VRSTATUS_VEL:0;
						status |= (vel.velocityFlags&XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)?VRSTATUS_AVEL:0;
						if (status & (VRSTATUS_VEL|VRSTATUS_AVEL))
						{
							Q_snprintf(cmd, sizeof(cmd), "%s %u %g %g %g %g %g %g %g %g %g %g %g %g\n", xr.actions[h].actname, status,
											 angles[0], angles[1], angles[2], org[0], org[1], org[2],
											 vel.angularVelocity.x, vel.angularVelocity.y, vel.angularVelocity.z, vel.linearVelocity.x, vel.linearVelocity.y, vel.linearVelocity.z);
						}
						else if (status & VRSTATUS_ORG)
						{
							Q_snprintf(cmd, sizeof(cmd), "%s %u %g %g %g %g %g %g\n", xr.actions[h].actname, status,
											 angles[0], angles[1], angles[2], org[0], org[1], org[2]);
						}
						else
						{
							Q_snprintf(cmd, sizeof(cmd), "%s %u %g %g %g\n", xr.actions[h].actname, status,
											 angles[0], angles[1], angles[2]);
						}
						cmdfuncs->AddText(cmd, false);
					}
				}
			}
			break;
		case XR_ACTION_TYPE_BOOLEAN_INPUT:
			{
				XrActionStateBoolean state = {XR_TYPE_ACTION_STATE_BOOLEAN};
				XrActionStateGetInfo info = {XR_TYPE_ACTION_STATE_GET_INFO};
				info.action = xr.actions[h].action;
				info.subactionPath = xr.actions[h].path;
				if (XR_FAILED(xrGetActionStateBoolean(xr.session, &info, &state)))
					break;
				if (!state.isActive) state.currentState = XR_FALSE;
				if ((!!state.currentState) != xr.actions[h].held)
				{
					xr.actions[h].held = !!state.currentState;
					if (xr.actions[h].held && !activesets[xr.actions[h].set])
						break;	//don't fire the down event when this action's set isn't meant to be active...
					if (*xr.actions[h].actname == '#')
					{
						int k = inputfuncs->GetKeyCode(xr.actions[h].actname+1, NULL);
						if (k >= 0)
							inputfuncs->KeyEvent(0, xr.actions[h].held, k, 0);
						else
							Con_Printf("Key \"%s\" for action \"%s\" is not a known key code\n", xr.actions[h].actname+1, xr.actions[h].actdescription);
					}
					else if (xr.actions[h].held || *xr.actions[h].actname == '+')
					{
						char cmd[256];
						Q_strlcpy(cmd, xr.actions[h].actname, sizeof(cmd));
						Q_strlcat(cmd, "\n", sizeof(cmd));
						if (!xr.actions[h].held)
							*cmd = '-';	//release events.
						cmdfuncs->AddText(cmd, false);
					}
				}
			}
			break;
		case XR_ACTION_TYPE_FLOAT_INPUT:
			{
				XrActionStateFloat state = {XR_TYPE_ACTION_STATE_FLOAT};
				XrActionStateGetInfo info = {XR_TYPE_ACTION_STATE_GET_INFO};
				info.action = xr.actions[h].action;
				info.subactionPath = xr.actions[h].path;
				if (XR_FAILED(xrGetActionStateFloat(xr.session, &info, &state)))
					break;

				if (state.isActive && activesets[xr.actions[h].set])
				{
					char cmd[256];
					if (!strncasecmp(xr.actions[h].actname, "#GP_AXIS_", 9))
					{
						int axis = -1;
						if (!strcasecmp(xr.actions[h].actname+9, "LTRIGGER"))
							axis = GPAXIS_LT_TRIGGER;
						else if (!strcasecmp(xr.actions[h].actname+9, "RTRIGGER"))
							axis = GPAXIS_RT_TRIGGER;
						else if (!strcasecmp(xr.actions[h].actname+9, "LEFT_X"))
							axis = GPAXIS_LT_RIGHT;
						else if (!strcasecmp(xr.actions[h].actname+9, "LEFT_Y"))
							axis = GPAXIS_LT_DOWN;
						else if (!strcasecmp(xr.actions[h].actname+9, "RIGHT_X"))
							axis = GPAXIS_RT_RIGHT;
						else if (!strcasecmp(xr.actions[h].actname+9, "RIGHT_Y"))
							axis = GPAXIS_RT_DOWN;
						else
							Con_Printf("Unknown gamepad axis: \"%s\"\n", xr.actions[h].actname+1);
						inputfuncs->JoystickAxisEvent(0, axis, state.currentState);
					}
					else
					{
						Q_snprintf(cmd, sizeof(cmd), "%s %g\n", xr.actions[h].actname, state.currentState);
						cmdfuncs->AddText(cmd, false);
					}
				}
			}
			break;
		case XR_ACTION_TYPE_VECTOR2F_INPUT:
			{
				XrActionStateVector2f state = {XR_TYPE_ACTION_STATE_VECTOR2F};
				XrActionStateGetInfo info = {XR_TYPE_ACTION_STATE_GET_INFO};
				info.action = xr.actions[h].action;
				info.subactionPath = xr.actions[h].path;
				if (XR_FAILED(xrGetActionStateVector2f(xr.session, &info, &state)))
					break;

				if (state.isActive && activesets[xr.actions[h].set])
				{
					char cmd[256];
					Q_snprintf(cmd, sizeof(cmd), "%s %g %g\n", xr.actions[h].actname, state.currentState.x, state.currentState.y);
					cmdfuncs->AddText(cmd, false);
				}
			}
			break;
		case XR_ACTION_TYPE_VIBRATION_OUTPUT:	//output only, nothing to read.
		case XR_ACTION_TYPE_MAX_ENUM: //not a real value
		safedefault:
			break;
		}
	}
}

static int64_t XR_CheckSwapFormats(int64_t *tryformat, int64_t *fmts, size_t fmtcount)
{
	size_t i, j;
	for (j = 0; j < fmtcount; j++)
	{	//the openxr driver lists is supposed to list formats in the order that it favours, so favour those.
		for (i = 0; tryformat[i]; i++)
		{	//now make sure its one that we allow. (steamvr lists ones that are NOT renderable - even though they should be)
			if (tryformat[i] == fmts[j])
				return fmts[j];
		}
	}
	return 0;
}
static int64_t XR_FindSwapFormat(int64_t **tryformats, int64_t *fmts, size_t fmtcount)
{
	int64_t fmt;
	/*try to use a format that matches the user's choice*/
	if (xr.srgb == 2)
		fmt = XR_CheckSwapFormats(tryformats[0], fmts, fmtcount);
	else if (xr.srgb)
		fmt = XR_CheckSwapFormats(tryformats[1], fmts, fmtcount);
	else
		fmt = XR_CheckSwapFormats(tryformats[2], fmts, fmtcount);

	/*try others out of desperation*/
	if (!fmt)
		fmt = XR_CheckSwapFormats(tryformats[1], fmts, fmtcount);
	if (!fmt)
		fmt = XR_CheckSwapFormats(tryformats[0], fmts, fmtcount);
	if (!fmt)
		fmt = XR_CheckSwapFormats(tryformats[2], fmts, fmtcount);
	if (!fmt)
		fmt = tryformats[1][0];	//fall back on the first srgb format we know of, in the hope that the driver just failed to list it.
	return fmt;
}
static qboolean XR_Begin(void)
{
	uint32_t u;
	XrResult res;
	uint32_t swapfmts;
	int64_t *fmts, fmttouse=0;

	xr.beginning = false;
	xr.ending = false;
	xr.inputsdirty = true;

	{
		XrSessionCreateInfo sessioninfo = {XR_TYPE_SESSION_CREATE_INFO};
		sessioninfo.next = xr.bindinginfo;
		sessioninfo.createFlags = 0;
		sessioninfo.systemId = xr.systemid;
		res = xrCreateSession(xr.instance, &sessioninfo, &xr.session);
	}
	if (XR_FAILED(res))
	{
		Con_Printf("OpenXR: xrCreateSession failed (%s)\n", XR_StringForResult(res));
		return false;
	}

	{
		XrReferenceSpaceCreateInfo info = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
		info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		info.poseInReferenceSpace.orientation.w = 1;
		res = xrCreateReferenceSpace(xr.session, &info, &xr.space);
		if (XR_FAILED(res))
			return false;
	}

	xrEnumerateSwapchainFormats(xr.session, 0, &swapfmts, NULL);
	fmts = alloca(sizeof(*fmts)*swapfmts);
	res = xrEnumerateSwapchainFormats(xr.session, swapfmts, &swapfmts, fmts);
	if (xr.renderer == QR_HEADLESS)
		;
	else if (!swapfmts)
		Con_Printf("OpenXR: No swapchain formats to use (%s)\n", XR_StringForResult(res));
#ifdef XR_USE_GRAPHICS_API_OPENGL
	else if (xr.renderer == QR_OPENGL)
	{
		static int64_t hdrformats[] = {GL_RGBA16F,GL_RGBA32F, 0};
		static int64_t srgbformats[] = {GL_SRGB8_ALPHA8_EXT, GL_SRGB8_EXT, 0};
		static int64_t rgbformats[] = {GL_RGB10_A2, GL_RGBA8, /*broken on steamvr - GL_RGBA16_EXT,*/ GL_RGB8, 0};
		static int64_t *formats[] = {hdrformats, srgbformats, rgbformats};
		fmttouse = XR_FindSwapFormat(formats, fmts, swapfmts);
	}
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	else if (xr.renderer == QR_VULKAN)
	{
		static int64_t hdrformats[] = {VK_FORMAT_R16G16B16A16_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT,VK_FORMAT_R32G32B32_SFLOAT, 0};
		static int64_t srgbformats[] = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_B8G8R8_SRGB, 0};
		static int64_t rgbformats[] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_B8G8R8_UNORM, 0};
		static int64_t *formats[] = {hdrformats, srgbformats, rgbformats};
		fmttouse = XR_FindSwapFormat(formats, fmts, swapfmts);
	}
#endif
#ifdef XR_USE_GRAPHICS_API_D3D11
	else if (xr.renderer == QR_DIRECT3D11)
	{
		static int64_t hdrformats[] = {DXGI_FORMAT_R16G16B16A16_FLOAT, 0};
		static int64_t srgbformats[] = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, 0};
		static int64_t rgbformats[] = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, 0};
		static int64_t *formats[] = {hdrformats, srgbformats, rgbformats};
		fmttouse = XR_FindSwapFormat(formats, fmts, swapfmts);
	}
#endif
	else
	{
		fmttouse = fmts[0];
		for (u = 0; u < swapfmts; u++)
			Con_Printf("fmt%u: %u / %x\n", u, (unsigned)fmts[u], (unsigned)fmts[u]);
	}

	xr.colourformat = fmttouse;
	for (u = 0; u < xr.viewcount; u++)
	{
		XrSwapchainCreateInfo swapinfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapinfo.createFlags = 0;
		swapinfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapinfo.format = fmttouse;
		swapinfo.sampleCount = 1;//xr.views->recommendedSwapchainSampleCount;
		swapinfo.width = xr.views->recommendedImageRectWidth;
		swapinfo.height = xr.views->recommendedImageRectHeight;
		swapinfo.faceCount = 1;	//2d, not a cube
		swapinfo.arraySize = 1;	//1 for 2d, a 1d array isn't allowed
		swapinfo.mipCount = 1;
		res = xrCreateSwapchain(xr.session, &swapinfo, &xr.eye[u].swapchain);
		if (XR_FAILED(res))
		{
			Con_Printf("OpenXR: xrCreateSwapchain failed (%s)\n", XR_StringForResult(res));
			return false;
		}
		res = xrEnumerateSwapchainImages(xr.eye[u].swapchain, 0, &xr.eye[u].numswapimages, NULL);
		if (XR_FAILED(res))
		{
			Con_Printf("OpenXR: xrEnumerateSwapchainImages failed (%s)\n", XR_StringForResult(res));
			return false;
		}

		//using a separate swapchain for each eye, so just depend upon npot here and use the whole image.
		xr.eye[u].subimage.imageRect.offset.x = 0;
		xr.eye[u].subimage.imageRect.offset.y = 0;
		xr.eye[u].subimage.imageRect.extent.width = swapinfo.width;
		xr.eye[u].subimage.imageRect.extent.height = swapinfo.height;
		xr.eye[u].subimage.swapchain = xr.eye[u].swapchain;
		xr.eye[u].subimage.imageArrayIndex = 0;

		//okay, this is annoying. the returned array size has different strides for different apis, etc.
		//translate it into something the relevant backend should understand.
		switch(xr.renderer)
		{
		default:
			return false;	//erk?
#ifdef XR_USE_GRAPHICS_API_D3D11
		case QR_DIRECT3D11:
			{
				uint32_t i;
				XrSwapchainImageD3D11KHR *xrimg = calloc(xr.eye[u].numswapimages, sizeof(*xrimg));
				for (i = 0; i < xr.eye[u].numswapimages; i++)
					xrimg[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
				res = xrEnumerateSwapchainImages(xr.eye[u].swapchain, xr.eye[u].numswapimages, &xr.eye[u].numswapimages, (XrSwapchainImageBaseHeader*)xrimg);
				if (XR_FAILED(res))
					xr.eye[u].numswapimages = 0;

				xr.eye[u].swapimages = calloc(xr.eye[u].numswapimages, sizeof(*xr.eye[u].swapimages));
				for (i = 0; i < xr.eye[u].numswapimages; i++)
				{
					xr.eye[u].swapimages[i].ptr = xrimg[i].texture;
					xr.eye[u].swapimages[i].ptr2 = NULL;	//view
					xr.eye[u].swapimages[i].width = swapinfo.width;
					xr.eye[u].swapimages[i].height = swapinfo.height;
					xr.eye[u].swapimages[i].depth = 1;
					xr.eye[u].swapimages[i].status = TEX_LOADED;
				}
			}
			break;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
		case QR_VULKAN:
			{
				uint32_t i;
				XrSwapchainImageVulkanKHR *xrimg = calloc(xr.eye[u].numswapimages, sizeof(*xrimg));
				struct vk_image_s *vkimg;
				for (i = 0; i < xr.eye[u].numswapimages; i++)
					xrimg[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
				res = xrEnumerateSwapchainImages(xr.eye[u].swapchain, xr.eye[u].numswapimages, &xr.eye[u].numswapimages, (XrSwapchainImageBaseHeader*)xrimg);
				if (XR_FAILED(res))
					xr.eye[u].numswapimages = 0;

				xr.eye[u].swapimages = calloc(xr.eye[u].numswapimages, sizeof(*xr.eye[u].swapimages)+sizeof(struct vk_image_s));
				vkimg = (struct vk_image_s*)&xr.eye[u].swapimages[xr.eye[u].numswapimages];
				for (i = 0; i < xr.eye[u].numswapimages; i++)
				{
					xr.eye[u].swapimages[i].vkimage = &vkimg[i];
					vkimg[i].vkformat = fmttouse;
					vkimg[i].image = xrimg[i].image;
					//vkimg[i].mem.* = 0;
					vkimg[i].view = VK_NULL_HANDLE;
					vkimg[i].sampler = VK_NULL_HANDLE;
					vkimg[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					vkimg[i].width = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					xr.eye[u].swapimages[i].width = vkimg[i].width = swapinfo.width;
					xr.eye[u].swapimages[i].height = vkimg[i].height = swapinfo.height;
					xr.eye[u].swapimages[i].depth = vkimg[i].layers = 1;
					vkimg[i].mipcount = swapinfo.mipCount;
					vkimg[i].encoding = PTI_INVALID; //blurgh, is this needed?
					vkimg[i].type = PTI_2D;
					xr.eye[u].swapimages[i].status = TEX_LOADED;
				}
			}
			break;
#endif
#ifdef XR_USE_GRAPHICS_API_OPENGL
		case QR_OPENGL:
			{
				uint32_t i;
				XrSwapchainImageOpenGLKHR *xrimg = calloc(xr.eye[u].numswapimages, sizeof(*xrimg));
				for (i = 0; i < xr.eye[u].numswapimages; i++)
					xrimg[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
				res = xrEnumerateSwapchainImages(xr.eye[u].swapchain, xr.eye[u].numswapimages, &xr.eye[u].numswapimages, (XrSwapchainImageBaseHeader*)xrimg);
				if (XR_FAILED(res))
					xr.eye[u].numswapimages = 0;

				xr.eye[u].swapimages = calloc(xr.eye[u].numswapimages, sizeof(*xr.eye[u].swapimages));
				for (i = 0; i < xr.eye[u].numswapimages; i++)
				{
					xr.eye[u].swapimages[i].format = swapinfo.format;
					xr.eye[u].swapimages[i].num = xrimg[i].image;
					xr.eye[u].swapimages[i].width = swapinfo.width;
					xr.eye[u].swapimages[i].height = swapinfo.height;
					xr.eye[u].swapimages[i].depth = 1;
					xr.eye[u].swapimages[i].status = TEX_LOADED;
				}
			}
			break;
#endif
		}
	}
	if (XR_FAILED(res))
		return false;

	XR_SetupInputs_Session();

	return true;
}

static void XR_ProcessEvents(void)
{
	XrEventDataBuffer ev;
	XrResult res;
	for (;;)
	{
		ev.type = XR_TYPE_EVENT_DATA_BUFFER;
		ev.next = NULL;
		res = xrPollEvent(xr.instance, &ev);
		if (res == XR_EVENT_UNAVAILABLE || XR_FAILED(res))
			return;	//nothing interesting here folks

		switch(ev.type)
		{
		default:	//no idea wtf that is
			Con_Printf("openxr event %u\n", ev.type);
			break;

		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
			XR_Shutdown();	//we're meant to try restarting, but that's a hassle. FIXME: expect the user to do a vid_restart.
			return;

		case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
			break;
		case XR_TYPE_EVENT_DATA_EVENTS_LOST:
			{
				XrEventDataEventsLost *s = (XrEventDataEventsLost*)&ev;
				Con_Printf(CON_ERROR"OpenXR: Lost %u events!\n", s->lostEventCount);
			}
			break;
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
			xr.inputsdirty = true;
			break;

		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
			{
				XrEventDataSessionStateChanged *s = (XrEventDataSessionStateChanged*)&ev;
				xr.state = s->state;
				return;	//make sure the outer loop actually sees each state change.
			}
			break;
		}
	}
}
static unsigned int XR_SyncFrame(double *frametime)
{
	unsigned int ret = 0;
	XrResult res;

	if (xr.fake)
	{
		xr.time += SECONDS_TO_NANOSECONDS(*frametime);
		return VRF_UIACTIVE;	//not syncing here. let the engine run at its normal rate
	}

	if (!xr.instance)
		return 0;

	if (xr.needrender)
	{	//something screwed up.
//		*frametime = 0;
		return VRF_UIACTIVE;
	}

	if (!xr.session)
	{
		if (xr_enable->ival && !XR_Begin())
		{
			XR_Shutdown();
			return 0;
		}
	}
	else
	{
		if (!xr_enable->ival && !xr.ending)
		{	//user doesn't want a session apparently. try and end the current session cleanly.
			res = xrRequestExitSession(xr.session);
			if (XR_FAILED(res))
				Con_Printf("openxr: Unable to request session end: %s\n", XR_StringForResult(res));
			xr.ending = true;
		}
	}

	XR_ProcessEvents();

	memset(&xr.framestate, 0, sizeof(xr.framestate));
	xr.framestate.type = XR_TYPE_FRAME_STATE;
	safeswitch(xr.state)
	{
	case XR_SESSION_STATE_IDLE:		//not allowed to progress till the user puts it on their head/etc
		xr.beginning = false;
		break;
	case XR_SESSION_STATE_READY:
		if (!xr.beginning)
		{
			XrSessionBeginInfo info = {XR_TYPE_SESSION_BEGIN_INFO};
			info.primaryViewConfigurationType = xr.viewtype;
			res = xrBeginSession(xr.session, &info);
			if (XR_FAILED(res))
				Con_Printf("Unable to begin session: %s\n", XR_StringForResult(res));
			xr.beginning = true;	//begin our xr loop... (and/or stop the spam just above)
		}
		break;
	case XR_SESSION_STATE_SYNCHRONIZED:	//no rendering or input yet
	case XR_SESSION_STATE_VISIBLE:		//now generating video frames, but no input yet
	case XR_SESSION_STATE_FOCUSED:		//we have inputs! (and still generating video frames)
		break;
	case XR_SESSION_STATE_STOPPING:		//going back to idle (user took it off their head). we'll go back to rendering to our window again.
		xrEndSession(xr.session);
		xr.beginning = false;
		break;
	case XR_SESSION_STATE_LOSS_PENDING:	//terminate for now. recreate later if you want.
		XR_SessionEnded();	//destroys the session but not the instance, so it can be started up again if desired.
		xr.beginning = false;
		break;
	case XR_SESSION_STATE_EXITING:		//terminate with prejudice.
		XR_SessionEnded();
		xr.beginning = false;
		if (!xr.ending)
			XR_Shutdown();	//this doesn't look like one we requested... don't let it start back up again.
		break;
	case XR_SESSION_STATE_UNKNOWN:
	case XR_SESSION_STATE_MAX_ENUM:
	safedefault:
		xr.beginning = false;	//some weird error.
		break;
	}

	if (xr.beginning)
	{
		XrTime time;
		res = xrWaitFrame(xr.session, NULL, &xr.framestate);
		if (XR_FAILED(res))
		{
			Con_Printf("xrWaitFrame: %s\n", XR_StringForResult(res));
			return false;
		}
		ret |= VRF_OVERRIDEFRAMETIME;
		if (xr.framestate.shouldRender)
			ret |= VRF_UIACTIVE;

		time = xr.framestate.predictedDisplayTime;
		if (xr.timeknown)
		{
			if (time < xr.time)	//make sure time doesn't go backward...
				time = xr.time;
			*frametime = NANOSECONDS_TO_SECONDS(time-xr.time);
		}
		xr.time = time;
		xr.timeknown = true;

		xr.needrender = true;
	}

	if (xr.session)
		XR_UpdateInputs(xr.framestate.predictedDisplayTime);

	return ret;
}
static qboolean XR_Render(void(*rendereye)(texid_t tex, const pxrect_t *viewport, const vec4_t fovoverride, const float projmatrix[16], const float eyematrix[12]))
{
	XrFrameEndInfo endframeinfo = {XR_TYPE_FRAME_END_INFO};
	unsigned int u;
	XrResult res;

	XrCompositionLayerProjection proj = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	const XrCompositionLayerBaseHeader *projlist[] = {(XrCompositionLayerBaseHeader*)&proj};
	XrCompositionLayerProjectionView projviews[MAX_VIEW_COUNT];

	if (xr.fake)
	{
		static vec3_t newhax[3], oldhax[3];
		static XrTime lastup;
		vec3_t org, ang = {0, 0, 0};
		float frac;
		const float UPDATEFREQ=1.0;
		float eyemat[12];

		if ((xr.time - lastup) > SECONDS_TO_NANOSECONDS(UPDATEFREQ))
		{
			lastup = xr.time;
			memcpy(oldhax, newhax, sizeof(oldhax));
			newhax[0][0] = crandom()*10;
			newhax[0][1] = crandom()*10;
			newhax[0][2] = crandom()*10;
			newhax[1][0] = crandom()*30;
			newhax[1][1] = crandom()*30;
			newhax[1][2] = crandom()*30;
			newhax[2][0] = crandom()*30;
			newhax[2][1] = crandom()*30;
			newhax[2][2] = crandom()*30;
		}
		frac = NANOSECONDS_TO_SECONDS(xr.time - lastup)/UPDATEFREQ;

		//randomly screw with these inputs. you'll have to handle the clicks separately. :(
		VectorInterpolate(oldhax[0], frac, newhax[0], ang);
		XR_Matrix3x4_RM_FromAngles(ang, org, eyemat);
		inputfuncs->SetHandPosition("head", org, ang, NULL, NULL);
		VectorInterpolate(oldhax[1], frac, newhax[1], ang);
		inputfuncs->SetHandPosition("right", org, ang, NULL, NULL);
		VectorInterpolate(oldhax[2], frac, newhax[2], ang);
		inputfuncs->SetHandPosition("left", org, ang, NULL, NULL);

		rendereye(NULL, NULL, NULL, NULL, eyemat);
		return true;	//skip the main view.
	}

	if (!xr.instance)
		return false;	//err... noooes!

	if (!xr.session || !xr.beginning)
		return false;

	if (xr.inputsdirty && xr.state==XR_SESSION_STATE_FOCUSED)
	{
		xr.inputsdirty = false;
		if (xr_debug->ival)
			XR_PrintInputs();
	}
#ifdef XR_EXT_hand_tracking
	for (u = 0; u < 2; u++)
	{
		vec3_t ang, org;
		unsigned int j;
		static const char *jointnames[] = {
			"PALM",
			"WRIST",
			"THUMB_METACARPAL",
			"THUMB_PROXIMAL",
			"THUMB_DISTAL",
			"THUMB_TIP",
			"INDEX_METACARPAL",
			"INDEX_PROXIMAL",
			"INDEX_INTERMEDIATE",
			"INDEX_DISTAL",
			"INDEX_TIP",
			"MIDDLE_METACARPAL",
			"MIDDLE_PROXIMAL",
			"MIDDLE_INTERMEDIATE",
			"MIDDLE_DISTAL",
			"MIDDLE_TIP",
			"RING_METACARPAL",
			"RING_PROXIMAL",
			"RING_INTERMEDIATE",
			"RING_DISTAL",
			"RING_TIP",
			"LITTLE_METACARPAL",
			"LITTLE_PROXIMAL",
			"LITTLE_INTERMEDIATE",
			"LITTLE_DISTAL",
			"LITTLE_TIP",
		};
		XrHandJointsLocateInfoEXT info = {XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
		XrHandJointLocationsEXT loc = {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
		XrHandJointVelocitiesEXT vel = {XR_TYPE_HAND_JOINT_VELOCITIES_EXT};
		loc.next = &vel;
		loc.jointCount = countof(xr.hand[u].jointloc);
		loc.jointLocations = xr.hand[u].jointloc;

		vel.next = &vel;
		vel.jointCount = countof(xr.hand[u].jointvel);
		vel.jointVelocities = xr.hand[u].jointvel;

		if (xr.hand[u].handle)
			xrLocateHandJointsEXT(xr.hand[u].handle, &info, &loc);
		xr.hand[u].active = loc.isActive;

		if (!xr.hand[u].active || !xr_debug->ival)
			continue;
		for (j = 0; j < countof(jointnames); j++)
		{
			if (!xr.hand[u].jointloc[j].locationFlags && !xr.hand[u].jointvel[j].velocityFlags)
				continue;
			XR_PoseToAngOrg(&xr.hand[u].jointloc[j].pose, ang, org);

			Con_Printf("%s %s: (%g %g %g) [%g %g %g] %g (%g %g %g) [%g %g %g]\n", u?"Right":"Left", jointnames[j],
				ang[0],ang[1],ang[2],org[0],org[1],org[2], xr.hand[u].jointloc[j].radius,
				xr.hand[u].jointvel[j].angularVelocity.x,xr.hand[u].jointvel[j].angularVelocity.y,xr.hand[u].jointvel[j].angularVelocity.z,
				xr.hand[u].jointvel[j].linearVelocity.x,xr.hand[u].jointvel[j].linearVelocity.y,xr.hand[u].jointvel[j].linearVelocity.z);
		}
	}
#endif

	if (!xr.needrender)
		return false;	//xrWaitFrame not called?
	xr.needrender = false;

	res = xrBeginFrame(xr.session, NULL);
	if (XR_FAILED(res))
	{
		Con_Printf("xrBeginFrame: %s\n", XR_StringForResult(res));
		if(res == XR_ERROR_SESSION_LOST)
			XR_SessionEnded();
		else
			XR_Shutdown();
		return false;
	}
	if (xr.framestate.shouldRender)
	{
		uint32_t eyecount;
		XrViewState viewstate = {XR_TYPE_VIEW_STATE};
		XrViewLocateInfo locateinfo = {XR_TYPE_VIEW_LOCATE_INFO};
		XrView eyeview[MAX_VIEW_COUNT]={};
		for (u = 0; u < MAX_VIEW_COUNT; u++)
			eyeview[u].type = XR_TYPE_VIEW;

		locateinfo.displayTime = xr.framestate.predictedDisplayTime;
		locateinfo.space = xr.space;
		res = xrLocateViews(xr.session, &locateinfo, &viewstate, xr.viewcount, &eyecount, eyeview);
		if (XR_FAILED(res))
			Con_Printf("xrLocateViews: %s\n", XR_StringForResult(res));

		proj.layerFlags = 0;
		proj.space = xr.space;
		proj.views = projviews;
		endframeinfo.layerCount = 1;

		//set up the head position, as an average of all the eyes, the eyes, the awful knowing eyes...
		{
			float scale;
			vec3_t ang, org;
			XrPosef apose = {0};
			for (u = 0; u < xr.viewcount && u < eyecount; u++)
			{	//add em up
				apose.orientation.x += eyeview[u].pose.orientation.x;
				apose.orientation.y += eyeview[u].pose.orientation.y;
				apose.orientation.z += eyeview[u].pose.orientation.z;
				apose.orientation.w += eyeview[u].pose.orientation.w;
				apose.position.x += eyeview[u].pose.position.x;
				apose.position.y += eyeview[u].pose.position.y;
				apose.position.z += eyeview[u].pose.position.z;
			}
			//normalize them
			scale = 1 / sqrt(apose.orientation.x*apose.orientation.x+apose.orientation.y*apose.orientation.y+apose.orientation.z*apose.orientation.z+apose.orientation.w*apose.orientation.w);
			apose.orientation.x *= scale;
			apose.orientation.y *= scale;
			apose.orientation.z *= scale;
			apose.orientation.w *= scale;
			apose.position.x /= xr.viewcount;
			apose.position.y /= xr.viewcount;
			apose.position.z /= xr.viewcount;
			XR_PoseToAngOrg(&apose, ang, org);
			inputfuncs->SetHandPosition("head", org, ang, NULL, NULL);
		}

		for (u = 0; u < xr.viewcount && u < eyecount; u++)
		{
			vec4_t fovoverride;
			XrSwapchainImageWaitInfo waitinfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
			unsigned int imgidx = 0;
			float eyematrix[12];
			res = xrAcquireSwapchainImage(xr.eye[u].swapchain, NULL, &imgidx);
			if (XR_FAILED(res))
				Con_Printf("xrAcquireSwapchainImage: %s\n", XR_StringForResult(res));

			memset(&projviews[u], 0, sizeof(projviews[u]));
			projviews[u].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
			projviews[u].pose = eyeview[u].pose;
			projviews[u].fov = eyeview[u].fov;
			projviews[u].subImage = xr.eye[u].subimage;

			XR_PoseToMat12(&eyeview[u].pose, eyematrix);

			fovoverride[0] = eyeview[u].fov.angleLeft * (180/M_PI);
			fovoverride[1] = eyeview[u].fov.angleRight * (180/M_PI);
			fovoverride[2] = eyeview[u].fov.angleDown * (180/M_PI);
			fovoverride[3] = eyeview[u].fov.angleUp * (180/M_PI);

			waitinfo.timeout = SECONDS_TO_NANOSECONDS(0.1);
			res = xrWaitSwapchainImage(xr.eye[u].swapchain, &waitinfo);
			if (XR_FAILED(res))
				Con_Printf("xrWaitSwapchainImage: %s\n", XR_StringForResult(res));
			rendereye(&xr.eye[u].swapimages[imgidx], NULL, fovoverride, NULL/*we're given fov info instead*/, eyematrix);
			//GL note: the OpenXR specification says NOTHING about the application having to glFlush or glFinish.
			//	I take this to mean that the openxr runtime is responsible for setting up barriers or w/e inside ReleaseSwapchainImage.
			//VK note: the OpenXR spec does say that it needs to be color_attachment_optimal+owned by queue. which it is.
			//	I take this to mean that the openxr runtime is responsible for barriers (as it'll need to transition it to general or shader-read anyway).
			res = xrReleaseSwapchainImage(xr.eye[u].swapchain, NULL);
			if (XR_FAILED(res))
				Con_Printf("xrReleaseSwapchainImage: %s\n", XR_StringForResult(res));
		}
		proj.viewCount = u;
	}

	endframeinfo.layers = projlist;
	endframeinfo.displayTime = xr.framestate.predictedDisplayTime;
	endframeinfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;	//we don't do the alpha channel very well.
	res = xrEndFrame(xr.session, &endframeinfo);
	if (XR_FAILED(res))
	{
		Con_Printf("xrEndFrame: %s\n", XR_StringForResult(res));
		if (res == XR_ERROR_SESSION_LOST || res == XR_ERROR_SESSION_NOT_RUNNING || res == XR_ERROR_SWAPCHAIN_RECT_INVALID)
			XR_SessionEnded();	//something sessiony
		else //if (res == XR_ERROR_INSTANCE_LOST)
			XR_Shutdown();	//don't really know what it was, just kill everything
	}

	return xr_skipregularview->ival;
}

static plugvrfuncs_t openxr =
{
	"OpenXR",
	XR_PreInit,
	XR_Init,
	XR_SyncFrame,
	XR_Render,
	XR_Shutdown,
};

qboolean Plug_Init(void)
{
#ifdef XR_NO_PROTOTYPES
	{
		static dllhandle_t *lib;
		static dllfunction_t funcs[] = {
			#define XRFUNC(n) {(void*)&n, #n},
				XRFUNCS
			#undef XRFUNC
			{NULL}};
#ifdef _WIN32
	#define XR_LOADER_LIBNAME "openxr_loader"ARCH_DL_POSTFIX
#else
	#define XR_LOADER_LIBNAME "libopenxr_loader"ARCH_DL_POSTFIX".1"
#endif
		if (!lib)
			lib = plugfuncs->LoadDLL(XR_LOADER_LIBNAME, funcs);
		if (!lib)
		{
			Con_Printf(CON_ERROR"OpenXR: Unable to load "XR_LOADER_LIBNAME"\n");
			return false;
		}
	}
#endif

	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));
	inputfuncs = plugfuncs->GetEngineInterface(pluginputfuncs_name, sizeof(*inputfuncs));
	plugfuncs->ExportFunction("MayUnload", XR_PluginMayUnload);
	if (plugfuncs->ExportInterface(plugvrfuncs_name, &openxr, sizeof(openxr)))
	{
		xr_enable			= cvarfuncs->GetNVFDG("xr_enable",			"1",			0,				"Controls whether to use openxr rendering or not.",									"OpenXR configuration");
		xr_debug			= cvarfuncs->GetNVFDG("xr_debug",			"0",			0,				"Controls whether to spam debug info or not.",										"OpenXR configuration");
		xr_formfactor		= cvarfuncs->GetNVFDG("xr_formfactor",		"head",			CVAR_ARCHIVE,	"Controls which VR system to try to use. Valid options are head, or hand",			"OpenXR configuration");
		xr_viewconfig		= cvarfuncs->GetNVFDG("xr_viewconfig",		"",				CVAR_ARCHIVE,	"Controls the type of view we aim for. Valid options are mono, stereo, or quad",	"OpenXR configuration");
		xr_metresize		= cvarfuncs->GetNVFDG("xr_metresize",		"26.24671916",	CVAR_ARCHIVE,	"Size of a metre in game units",													"OpenXR configuration");
		xr_skipregularview	= cvarfuncs->GetNVFDG("xr_skipregularview", "1",			CVAR_ARCHIVE,	"Skip rendering the regular view when OpenXR is active.",							"OpenXR configuration");
		xr_fingertracking	= cvarfuncs->GetNVFDG("xr_fingertracking",	"0",			0,	"Attempt to track individual finger joints.",													"OpenXR configuration");
		return true;
	}
	return false;
}
