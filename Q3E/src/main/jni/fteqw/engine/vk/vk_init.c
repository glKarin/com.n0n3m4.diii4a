#include "quakedef.h"
#ifdef VKQUAKE
#include "vkrenderer.h"
#include "gl_draw.h"
#include "shader.h"
#include "renderque.h"	//is anything still using this?

#include "vr.h"

extern qboolean vid_isfullscreen;

cvar_t vk_stagingbuffers						= CVARFD ("vk_stagingbuffers",			"", CVAR_RENDERERLATCH, "Configures which dynamic buffers are copied into gpu memory for rendering, instead of reading from shared memory. Empty for default settings.\nAccepted chars are u(niform), e(lements), v(ertex), 0(none).");
static cvar_t vk_submissionthread				= CVARD	("vk_submissionthread",			"", "Execute submits+presents on a thread dedicated to executing them. This may be a significant speedup on certain drivers.");
static cvar_t vk_debug							= CVARFD("vk_debug",					"0", CVAR_VIDEOLATCH, "Register a debug handler to display driver/layer messages. 2 enables the standard validation layers.");
static cvar_t vk_dualqueue						= CVARFD("vk_dualqueue",				"", CVAR_VIDEOLATCH, "Attempt to use a separate queue for presentation. Blank for default.");
static cvar_t vk_busywait						= CVARD ("vk_busywait",					"", "Force busy waiting until the GPU finishes doing its thing.");
static cvar_t vk_waitfence						= CVARD ("vk_waitfence",				"", "Waits on fences, instead of semaphores. This is more likely to result in gpu stalls while the cpu waits.");
static cvar_t vk_usememorypools					= CVARFD("vk_usememorypools",			"",	CVAR_VIDEOLATCH, "Allocates memory pools for sub allocations. Vulkan has a limit to the number of memory allocations allowed so this should always be enabled, however at this time FTE is unable to reclaim pool memory, and would require periodic vid_restarts to flush them.");
static cvar_t vk_khr_get_memory_requirements2	= CVARFD("vk_khr_get_memory_requirements2", "", CVAR_VIDEOLATCH, "Enable extended memory info querires");
static cvar_t vk_khr_dedicated_allocation		= CVARFD("vk_khr_dedicated_allocation",	"", CVAR_VIDEOLATCH, "Flag vulkan memory allocations as dedicated, where applicable.");
static cvar_t vk_khr_push_descriptor			= CVARFD("vk_khr_push_descriptor",		"", CVAR_VIDEOLATCH, "Enables better descriptor streaming.");
static cvar_t vk_amd_rasterization_order		= CVARFD("vk_amd_rasterization_order",	"",	CVAR_VIDEOLATCH, "Enables the use of relaxed rasterization ordering, for a small speedup at the minor risk of a little zfighting.");
#ifdef VK_KHR_fragment_shading_rate
static cvar_t vK_khr_fragment_shading_rate		= CVARFD("vK_khr_fragment_shading_rate","",	CVAR_VIDEOLATCH, "Enables the use of variable shading rates.");
#endif
#ifdef VK_EXT_astc_decode_mode
static cvar_t vk_ext_astc_decode_mode			= CVARFD("vk_ext_astc_decode_mode",		"",	CVAR_VIDEOLATCH, "Enables reducing texture cache sizes for LDR ASTC-compressed textures.");
#endif
#ifdef VK_KHR_ray_query
static cvar_t vk_khr_ray_query					= CVARFD("vk_khr_ray_query",			"",	CVAR_VIDEOLATCH, "Required for the use of hardware raytraced shadows.");
#endif
extern cvar_t vid_srgb, vid_vsync, vid_triplebuffer, r_stereo_method, vid_multisample, vid_bpp;

texid_t r_blackcubeimage, r_whitecubeimage;


void VK_RegisterVulkanCvars(void)
{
#define VKRENDEREROPTIONS	"Vulkan-Specific Renderer Options"
	Cvar_Register (&vk_stagingbuffers,			VKRENDEREROPTIONS);
	Cvar_Register (&vk_submissionthread,		VKRENDEREROPTIONS);
	Cvar_Register (&vk_debug,					VKRENDEREROPTIONS);
	Cvar_Register (&vk_dualqueue,				VKRENDEREROPTIONS);
	Cvar_Register (&vk_busywait,				VKRENDEREROPTIONS);
	Cvar_Register (&vk_waitfence,				VKRENDEREROPTIONS);
	Cvar_Register (&vk_usememorypools,			VKRENDEREROPTIONS);

	Cvar_Register (&vk_khr_get_memory_requirements2,VKRENDEREROPTIONS);
	Cvar_Register (&vk_khr_dedicated_allocation,	VKRENDEREROPTIONS);
	Cvar_Register (&vk_khr_push_descriptor,			VKRENDEREROPTIONS);
	Cvar_Register (&vk_amd_rasterization_order,		VKRENDEREROPTIONS);
#ifdef VK_KHR_fragment_shading_rate
	Cvar_Register (&vK_khr_fragment_shading_rate,	VKRENDEREROPTIONS);
#endif
#ifdef VK_EXT_astc_decode_mode
	Cvar_Register (&vk_ext_astc_decode_mode,	VKRENDEREROPTIONS);
#endif
#ifdef VK_KHR_ray_query
	Cvar_Register (&vk_khr_ray_query,			VKRENDEREROPTIONS);
#endif
}
void R2D_Console_Resize(void);
static void VK_DestroySampler(VkSampler s);

extern qboolean		scr_con_forcedraw;

#ifndef MULTITHREAD
#define Sys_LockConditional(c)
#define Sys_UnlockConditional(c)
#endif

static const char *vklayerlist[] =
{
#if 1
	"VK_LAYER_KHRONOS_validation"
#elif 1
	"VK_LAYER_LUNARG_standard_validation"
#else
		//older versions of the sdk were crashing out on me,
//	"VK_LAYER_LUNARG_api_dump",
"VK_LAYER_LUNARG_device_limits",
//"VK_LAYER_LUNARG_draw_state",
"VK_LAYER_LUNARG_image",
//"VK_LAYER_LUNARG_mem_tracker",
"VK_LAYER_LUNARG_object_tracker",
"VK_LAYER_LUNARG_param_checker",
"VK_LAYER_LUNARG_screenshot",
"VK_LAYER_LUNARG_swapchain",
"VK_LAYER_GOOGLE_threading",
"VK_LAYER_GOOGLE_unique_objects",
//"VK_LAYER_LUNARG_vktrace",
#endif
};
#define vklayercount (vk_debug.ival>1?countof(vklayerlist):0)


//code to initialise+destroy vulkan contexts.
//this entire file is meant to be platform-agnostic.
//the vid code still needs to set up vkGetInstanceProcAddr, and do all the window+input stuff.

#ifdef VK_NO_PROTOTYPES
	#define VKFunc(n) PFN_vk##n vk##n;
	#ifdef VK_EXT_debug_utils
		static VKFunc(CreateDebugUtilsMessengerEXT)
		static VKFunc(DestroyDebugUtilsMessengerEXT)
	#endif
	#ifdef VK_EXT_debug_report
		static VKFunc(CreateDebugReportCallbackEXT)
		static VKFunc(DestroyDebugReportCallbackEXT)
	#endif
	VKFuncs
	#undef VKFunc
#endif

void VK_Submit_Work(VkCommandBuffer cmdbuf, VkSemaphore semwait, VkPipelineStageFlags semwaitstagemask, VkSemaphore semsignal, VkFence fencesignal, struct vkframe *presentframe, struct vk_fencework *fencedwork);
#ifdef MULTITHREAD
static int VK_Submit_Thread(void *arg);
#endif
static void VK_Submit_DoWork(void);

static void VK_DestroyRenderPasses(void);
VkRenderPass VK_GetRenderPass(int pass);
static void VK_Shutdown_PostProc(void);
		
struct vulkaninfo_s vk;
static struct vk_rendertarg postproc[4];
static unsigned int postproc_buf;
static struct vk_rendertarg_cube vk_rt_cubemap;

qboolean VK_SCR_GrabBackBuffer(void);

#if defined(__linux__) && defined(__GLIBC__)
#include <execinfo.h>
#define DOBACKTRACE()					\
do {							\
	void *bt[16];					\
	int i, fr = backtrace(bt, countof(bt));		\
	char **strings = backtrace_symbols(bt, fr);	\
	for (i = 0; i < fr; i++)			\
		if (strings)				\
			Con_Printf("\t%s\n", strings[i]);	\
		else					\
			Con_Printf("\t%p\n", bt[i]);	\
	free(strings);					\
} while(0)
#else
#define DOBACKTRACE()
#endif

char *VK_VKErrorToString(VkResult err)
{
	switch(err)
	{
	//positive codes
	case VK_SUCCESS:						return "VK_SUCCESS";
	case VK_NOT_READY:						return "VK_NOT_READY";
	case VK_TIMEOUT:						return "VK_TIMEOUT";
	case VK_EVENT_SET:						return "VK_EVENT_SET";
	case VK_EVENT_RESET:					return "VK_EVENT_RESET";
	case VK_INCOMPLETE:						return "VK_INCOMPLETE";

	//core errors
	case VK_ERROR_OUT_OF_HOST_MEMORY:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED:	return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST:				return "VK_ERROR_DEVICE_LOST";	//by far the most common.
	case VK_ERROR_MEMORY_MAP_FAILED:		return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT:		return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT:	return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT:		return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER:		return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS:			return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_FRAGMENTED_POOL:			return "VK_ERROR_FRAGMENTED_POOL";

	case VK_ERROR_SURFACE_LOST_KHR:			return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR:					return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR:			return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:	return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";

	case VK_ERROR_VALIDATION_FAILED_EXT:	return "VK_ERROR_VALIDATION_FAILED_EXT";
	case VK_ERROR_INVALID_SHADER_NV:		return "VK_ERROR_INVALID_SHADER_NV";
	case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:	return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:	return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR";

#ifdef VK_EXT_image_drm_format_modifier
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:		return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
#endif
#ifdef VK_EXT_descriptor_indexing
    case VK_ERROR_FRAGMENTATION_EXT:			return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
#endif
#ifdef VK_EXT_global_priority
    case VK_ERROR_NOT_PERMITTED_EXT:			return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
#endif
#ifdef VK_EXT_buffer_device_address
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:	return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
#endif

	//irrelevant parts of the enum
//	case VK_RESULT_RANGE_SIZE:
	case VK_RESULT_MAX_ENUM:
	default:
		break;
	}
	return va("%d", (int)err);
}
#ifdef VK_EXT_debug_utils
static void DebugSetName(VkObjectType objtype, uint64_t handle, const char *name)
{
	if (vkSetDebugUtilsObjectNameEXT)
	{
		VkDebugUtilsObjectNameInfoEXT info =
		{
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			NULL,
			objtype,
			handle,
			name
		};
		vkSetDebugUtilsObjectNameEXT(vk.device, &info);
	}
}
static VkDebugUtilsMessengerEXT vk_debugucallback;
char *DebugAnnotObjectToString(VkObjectType t)
{
	switch(t)
	{
	case VK_OBJECT_TYPE_UNKNOWN:						return "VK_OBJECT_TYPE_UNKNOWN";
    case VK_OBJECT_TYPE_INSTANCE:						return "VK_OBJECT_TYPE_INSTANCE";
	case VK_OBJECT_TYPE_PHYSICAL_DEVICE:				return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
	case VK_OBJECT_TYPE_DEVICE:							return "VK_OBJECT_TYPE_DEVICE";
	case VK_OBJECT_TYPE_QUEUE:							return "VK_OBJECT_TYPE_QUEUE";
	case VK_OBJECT_TYPE_SEMAPHORE:						return "VK_OBJECT_TYPE_SEMAPHORE";
	case VK_OBJECT_TYPE_COMMAND_BUFFER:					return "VK_OBJECT_TYPE_COMMAND_BUFFER";
	case VK_OBJECT_TYPE_FENCE:							return "VK_OBJECT_TYPE_FENCE";
	case VK_OBJECT_TYPE_DEVICE_MEMORY:					return "VK_OBJECT_TYPE_DEVICE_MEMORY";
	case VK_OBJECT_TYPE_BUFFER:							return "VK_OBJECT_TYPE_BUFFER";
	case VK_OBJECT_TYPE_IMAGE:							return "VK_OBJECT_TYPE_IMAGE";
	case VK_OBJECT_TYPE_EVENT:							return "VK_OBJECT_TYPE_EVENT";
	case VK_OBJECT_TYPE_QUERY_POOL:						return "VK_OBJECT_TYPE_QUERY_POOL";
	case VK_OBJECT_TYPE_BUFFER_VIEW:					return "VK_OBJECT_TYPE_BUFFER_VIEW";
	case VK_OBJECT_TYPE_IMAGE_VIEW:						return "VK_OBJECT_TYPE_IMAGE_VIEW";
	case VK_OBJECT_TYPE_SHADER_MODULE:					return "VK_OBJECT_TYPE_SHADER_MODULE";
	case VK_OBJECT_TYPE_PIPELINE_CACHE:					return "VK_OBJECT_TYPE_PIPELINE_CACHE";
	case VK_OBJECT_TYPE_PIPELINE_LAYOUT:				return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
	case VK_OBJECT_TYPE_RENDER_PASS:					return "VK_OBJECT_TYPE_RENDER_PASS";
	case VK_OBJECT_TYPE_PIPELINE:						return "VK_OBJECT_TYPE_PIPELINE";
	case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:			return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
	case VK_OBJECT_TYPE_SAMPLER:						return "VK_OBJECT_TYPE_SAMPLER";
	case VK_OBJECT_TYPE_DESCRIPTOR_POOL:				return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
	case VK_OBJECT_TYPE_DESCRIPTOR_SET:					return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
	case VK_OBJECT_TYPE_FRAMEBUFFER:					return "VK_OBJECT_TYPE_FRAMEBUFFER";
	case VK_OBJECT_TYPE_COMMAND_POOL:					return "VK_OBJECT_TYPE_COMMAND_POOL";
	case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:		return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
	case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:		return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
	case VK_OBJECT_TYPE_SURFACE_KHR:					return "VK_OBJECT_TYPE_SURFACE_KHR";
	case VK_OBJECT_TYPE_SWAPCHAIN_KHR:					return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
	case VK_OBJECT_TYPE_DISPLAY_KHR:					return "VK_OBJECT_TYPE_DISPLAY_KHR";
	case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:				return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
	case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:		return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
#ifdef VK_NVX_device_generated_commands
	case VK_OBJECT_TYPE_OBJECT_TABLE_NVX:				return "VK_OBJECT_TYPE_OBJECT_TABLE_NVX";
	case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX:	return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX";
#endif
	case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:		return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
	case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:			return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
#ifdef VK_NV_ray_tracing
	case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:		return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
#endif
#ifdef VK_KHR_acceleration_structure
	case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:		return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
#endif
//	case VK_OBJECT_TYPE_RANGE_SIZE:
    case VK_OBJECT_TYPE_MAX_ENUM:
		break;
	default:
		break;
	}
	return "UNKNOWNTYPE";
}
static VKAPI_ATTR VkBool32 VKAPI_CALL mydebugutilsmessagecallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT*pCallbackData, void* pUserData)
{
	char prefix[64];
	int l = 0;	//developer level
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{	//spam?
		strcpy(prefix, "VERBOSE:");
		l = 2;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{	//generally stuff like 'object created'
		strcpy(prefix, "INFO:");
		l = 1;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		strcpy(prefix, CON_WARNING"WARNING:");
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		strcpy(prefix, CON_ERROR "ERROR:");

	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		strcat(prefix, "GENERAL");
	else
	{
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			strcat(prefix, "SPEC");
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		{
			if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			{
				strcat(prefix, "|");
			}
			strcat(prefix,"PERF");
		}
	}
	Con_DLPrintf(l, "%s[%d] %s - %s\n", prefix, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName?pCallbackData->pMessageIdName:"", pCallbackData->pMessage);

	if (pCallbackData->objectCount > 0)
	{
		uint32_t object;
		for(object = 0; object < pCallbackData->objectCount; ++object)
			Con_DLPrintf(l, "       Object[%d] - Type %s, Value %"PRIx64", Name \"%s\"\n", object,
						DebugAnnotObjectToString(pCallbackData->pObjects[object].objectType),
						pCallbackData->pObjects[object].objectHandle,
						pCallbackData->pObjects[object].pObjectName);
	}

	if (pCallbackData->cmdBufLabelCount > 0)
	{
		uint32_t label;
		for (label = 0; label < pCallbackData->cmdBufLabelCount; ++label)
			Con_DLPrintf(l, "       Label[%d] - %s { %f, %f, %f, %f}\n", label,
						pCallbackData->pCmdBufLabels[label].pLabelName,
						pCallbackData->pCmdBufLabels[label].color[0],
						pCallbackData->pCmdBufLabels[label].color[1],
						pCallbackData->pCmdBufLabels[label].color[2],
						pCallbackData->pCmdBufLabels[label].color[3]);
	}
	return false;
}
#else
#define DebugSetName(objtype,handle,name)
#endif
#ifdef VK_EXT_debug_report
static VkDebugReportCallbackEXT vk_debugcallback;
static VkBool32 VKAPI_PTR mydebugreportcallback(
				VkDebugReportFlagsEXT                       flags,
				VkDebugReportObjectTypeEXT                  objectType,
				uint64_t                                    object,
				size_t                                      location,
				int32_t                                     messageCode,
				const char*                                 pLayerPrefix,
				const char*                                 pMessage,
				void*                                       pUserData)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		Con_Printf("ERR: %s: %s\n", pLayerPrefix, pMessage);
//		DOBACKTRACE();
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		if (!strncmp(pMessage, "Additional bits in Source accessMask", 36) && strstr(pMessage, "VK_IMAGE_LAYOUT_UNDEFINED"))
			return false;	//I don't give a fuck. undefined can be used to change layouts on a texture that already exists too.
		Con_Printf("WARN: %s: %s\n", pLayerPrefix, pMessage);
		DOBACKTRACE();
	}
	else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		Con_DPrintf("DBG: %s: %s\n", pLayerPrefix, pMessage);
//		DOBACKTRACE();
	}
	else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
#ifdef _WIN32
//		OutputDebugString(va("INF: %s\n", pMessage));
#else
		Con_Printf("INF: %s: %s\n", pLayerPrefix, pMessage);
//		DOBACKTRACE();
#endif
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		Con_Printf("PERF: %s: %s\n", pLayerPrefix, pMessage);    
		DOBACKTRACE();
	}
	else
	{
		Con_Printf("OTHER: %s: %s\n", pLayerPrefix, pMessage);
		DOBACKTRACE();
	}
	return false;
}
#endif

//typeBits is some vulkan requirement thing (like textures must be device-local).
//requirements_mask are things that the engine may require (like host-visible).
//note that there is absolutely no guarentee that hardware requirements will match what the host needs.
//thus you may need to use staging.
uint32_t vk_find_memory_try(uint32_t typeBits, VkFlags requirements_mask)
{
	uint32_t i;
	for (i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((vk.memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
				return i;
		}
		typeBits >>= 1;
	}
	return ~0u;
}
uint32_t vk_find_memory_require(uint32_t typeBits, VkFlags requirements_mask)
{
	uint32_t ret = vk_find_memory_try(typeBits, requirements_mask);
	if (ret == ~0)
		Sys_Error("Unable to find suitable vulkan memory pool\n");
	return ret;
}

void VK_DestroyVkTexture(vk_image_t *img)
{
	if (!img)
		return;
	if (img->sampler)
		VK_DestroySampler(img->sampler);
	if (img->view)
		vkDestroyImageView(vk.device, img->view, vkallocationcb);
	if (img->image)
		vkDestroyImage(vk.device, img->image, vkallocationcb);
	VK_ReleasePoolMemory(&img->mem);
}
static void VK_DestroyVkTexture_Delayed(void *w)
{
	VK_DestroyVkTexture(w);
}

static void VK_DestroySwapChain(void)
{
	uint32_t i;

#ifdef MULTITHREAD
	if (vk.submitcondition)
	{
		Sys_LockConditional(vk.submitcondition);
		vk.neednewswapchain = true;
		Sys_ConditionSignal(vk.submitcondition);
		Sys_UnlockConditional(vk.submitcondition);
	}
	if (vk.submitthread)
	{
		Sys_WaitOnThread(vk.submitthread);
		vk.submitthread = NULL;
	}
#endif
	while (vk.work)
	{
		Sys_LockConditional(vk.submitcondition);
		VK_Submit_DoWork();
		Sys_UnlockConditional(vk.submitcondition);
	}
	if (vk.dopresent)
		vk.dopresent(NULL);
	if (vk.device)
		vkDeviceWaitIdle(vk.device);
	/*while (vk.aquirenext < vk.aquirelast)
	{
		VkWarnAssert(vkWaitForFences(vk.device, 1, &vk.acquirefences[vk.aquirenext%ACQUIRELIMIT], VK_FALSE, UINT64_MAX));
		vk.aquirenext++;
	}*/
	VK_FencedCheck();
	while(vk.frameendjobs)
	{	//we've fully synced the gpu now, we can clean up any resources that were pending but not assigned yet.
		struct vk_frameend *job = vk.frameendjobs;
		vk.frameendjobs = job->next;
		job->FrameEnded(job+1);
		Z_Free(job);
	}

	if (vk.frame)
	{
		vk.frame->next = vk.unusedframes;
		vk.unusedframes = vk.frame;
		vk.frame = NULL;
	}

	if (vk.dopresent)
		vk.dopresent(NULL);

	//wait for it to all finish first...
	if (vk.device)
		vkDeviceWaitIdle(vk.device);
#if 0	//don't bother waiting as they're going to be destroyed anyway, and we're having a lot of fun with drivers that don't bother signalling them on teardown
	vk.acquirenext = vk.acquirelast;
#else
	//clean up our acquires so we know the driver isn't going to update anything.
	while (vk.acquirenext < vk.acquirelast)
	{
		if (vk.acquirefences[vk.acquirenext%ACQUIRELIMIT])
			VkWarnAssert(vkWaitForFences(vk.device, 1, &vk.acquirefences[vk.acquirenext%ACQUIRELIMIT], VK_FALSE, 1000000000u));	//drivers suck, especially in times of error, and especially if its nvidia's vulkan driver.
		vk.acquirenext++;
	}
#endif
	for (i = 0; i < ACQUIRELIMIT; i++)
	{
		if (vk.acquirefences[i])
			vkDestroyFence(vk.device, vk.acquirefences[i], vkallocationcb);
		vk.acquirefences[i] = VK_NULL_HANDLE;
	}

	for (i = 0; i < vk.backbuf_count; i++)
	{
		//swapchain stuff
		if (vk.backbufs[i].framebuffer)
			vkDestroyFramebuffer(vk.device, vk.backbufs[i].framebuffer, vkallocationcb);
		vk.backbufs[i].framebuffer = VK_NULL_HANDLE;
		if (vk.backbufs[i].colour.view)
			vkDestroyImageView(vk.device, vk.backbufs[i].colour.view, vkallocationcb);
		vk.backbufs[i].colour.view = VK_NULL_HANDLE;
		VK_DestroyVkTexture(&vk.backbufs[i].depth);
		VK_DestroyVkTexture(&vk.backbufs[i].mscolour);
		vkDestroySemaphore(vk.device, vk.backbufs[i].presentsemaphore, vkallocationcb);
	}

	while(vk.unusedframes)
	{
		struct vkframe *frame = vk.unusedframes;
		vk.unusedframes = frame->next;

		VKBE_ShutdownFramePools(frame);

		vkFreeCommandBuffers(vk.device, vk.cmdpool, frame->maxcbufs, frame->cbufs);
		BZ_Free(frame->cbufs);
		vkDestroyFence(vk.device, frame->finishedfence, vkallocationcb);
		Z_Free(frame);
	}

	if (vk.swapchain)
	{
		vkDestroySwapchainKHR(vk.device, vk.swapchain, vkallocationcb);
		vk.swapchain = VK_NULL_HANDLE;
	}

	if (vk.backbufs)
		free(vk.backbufs);
	vk.backbufs = NULL;
	vk.backbuf_count = 0;
}

static qboolean VK_CreateSwapChain(void)
{
	qboolean reloadshaders = false;
	uint32_t fmtcount;
	VkSurfaceFormatKHR *surffmts;
	uint32_t presentmodes;
	VkPresentModeKHR *presentmode;
	VkSurfaceCapabilitiesKHR surfcaps;
	VkSwapchainCreateInfoKHR swapinfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	uint32_t i, curpri, preaquirecount;
	VkSwapchainKHR newvkswapchain;
	VkImage *images;
	VkDeviceMemory *memories;
	VkImageView attachments[3];
	VkFramebufferCreateInfo fb_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	VkSampleCountFlagBits oldms;
	uint32_t rpassflags = 0;
	VkResult err;

	VkFormat oldformat = vk.backbufformat;
	VkFormat olddepthformat = vk.depthformat;

#ifdef _DIII4A //karin: init fixed screen size
	extern int screen_width;
	extern int screen_height;
	vid.pixelwidth = screen_width;
	vid.pixelheight = screen_height;
#endif
	vk.dopresent(NULL);	//make sure they're all pushed through.


	vid_vsync.modified = false;
	vid_triplebuffer.modified = false;
	vid_srgb.modified = false;
	vk_submissionthread.modified = false;
	vk_waitfence.modified = false;
	vid_multisample.modified = false;

	vk.triplebuffer = vid_triplebuffer.ival;
	vk.vsync = vid_vsync.ival;

	if (!vk.khr_swapchain)
	{	//headless
		if (vk.swapchain || vk.backbuf_count)
			VK_DestroySwapChain();

		vk.backbufformat = ((vid.flags&VID_SRGBAWARE)||vid_srgb.ival)?VK_FORMAT_B8G8R8A8_SRGB:VK_FORMAT_B8G8R8A8_UNORM;
		vk.backbuf_count = 4;

		swapinfo.imageExtent.width = vid.pixelwidth;
		swapinfo.imageExtent.height = vid.pixelheight;

		images = malloc(sizeof(VkImage)*vk.backbuf_count);
		memset(images, 0, sizeof(VkImage)*vk.backbuf_count);
		memories = malloc(sizeof(VkDeviceMemory)*vk.backbuf_count);
		memset(memories, 0, sizeof(VkDeviceMemory)*vk.backbuf_count);

		vk.acquirelast = vk.acquirenext = 0;
		for (i = 0; i < ACQUIRELIMIT; i++)
		{
			if (1)
			{
				VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
				fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VkAssert(vkCreateFence(vk.device,&fci,vkallocationcb,&vk.acquirefences[i]));
				vk.acquiresemaphores[i] = VK_NULL_HANDLE;
			}
			else
			{
				VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
				VkAssert(vkCreateSemaphore(vk.device, &sci, vkallocationcb, &vk.acquiresemaphores[i]));
				DebugSetName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)vk.acquiresemaphores[i], "vk.acquiresemaphores");
				vk.acquirefences[i] = VK_NULL_HANDLE;
			}

			vk.acquirebufferidx[vk.acquirelast%ACQUIRELIMIT] = vk.acquirelast%vk.backbuf_count;
			vk.acquirelast++;
		}

		for (i = 0; i < vk.backbuf_count; i++)
		{
			VkMemoryRequirements mem_reqs;
			VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			VkMemoryDedicatedAllocateInfoKHR khr_mdai = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};
			VkImageCreateInfo ici = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

			ici.flags = 0;
			ici.imageType = VK_IMAGE_TYPE_2D;
			ici.format = vk.backbufformat;
			ici.extent.width = vid.pixelwidth;
			ici.extent.height = vid.pixelheight;
			ici.extent.depth = 1;
			ici.mipLevels = 1;
			ici.arrayLayers = 1;
			ici.samples = VK_SAMPLE_COUNT_1_BIT;
			ici.tiling = VK_IMAGE_TILING_OPTIMAL;
			ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			ici.queueFamilyIndexCount = 0;
			ici.pQueueFamilyIndices = NULL;
			ici.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAssert(vkCreateImage(vk.device, &ici, vkallocationcb, &images[i]));
			DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)images[i], "backbuffer");

			vkGetImageMemoryRequirements(vk.device, images[i], &mem_reqs);

			memAllocInfo.allocationSize = mem_reqs.size;
			memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (memAllocInfo.memoryTypeIndex == ~0)
				memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (memAllocInfo.memoryTypeIndex == ~0)
				memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			if (memAllocInfo.memoryTypeIndex == ~0)
				memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, 0);

			if (vk.khr_dedicated_allocation)
			{
				khr_mdai.pNext = memAllocInfo.pNext;
				khr_mdai.image = images[i];
				memAllocInfo.pNext = &khr_mdai;
			}

			VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &memories[i]));
			DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)memories[i], "VK_CreateSwapChain");
			VkAssert(vkBindImageMemory(vk.device, images[i], memories[i], 0));
		}
	}
	else
	{	//using vulkan's presentation engine.
		int BOOST_UNORM, BOOST_SNORM, BOOST_SRGB, BOOST_UFLOAT, BOOST_SFLOAT;

		if (vid_srgb.ival > 1)
		{	//favour float formats, then srgb, then unorms
			BOOST_UNORM		= 0;
			BOOST_SNORM		= 0;
			BOOST_SRGB		= 128;
			BOOST_UFLOAT	= 256;
			BOOST_SFLOAT	= 256;
		}
		else if (vid_srgb.ival)
		{
			BOOST_UNORM		= 0;
			BOOST_SNORM		= 0;
			BOOST_SRGB		= 256;
			BOOST_UFLOAT	= 128;
			BOOST_SFLOAT	= 128;
		}
		else
		{
			BOOST_UNORM		= 256;
			BOOST_SNORM		= 256;
			BOOST_SRGB		= 0;
			BOOST_UFLOAT	= 128;
			BOOST_SFLOAT	= 128;
		}

		VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &fmtcount, NULL));
		surffmts = malloc(sizeof(VkSurfaceFormatKHR)*fmtcount);
		VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &fmtcount, surffmts));

		VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &presentmodes, NULL));
		presentmode = malloc(sizeof(VkPresentModeKHR)*presentmodes);
		VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &presentmodes, presentmode));

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.gpu, vk.surface, &surfcaps);

		swapinfo.surface = vk.surface;
		swapinfo.minImageCount = surfcaps.minImageCount+vk.triplebuffer;
		if (swapinfo.minImageCount > surfcaps.maxImageCount)
			swapinfo.minImageCount = surfcaps.maxImageCount;
		if (swapinfo.minImageCount < surfcaps.minImageCount)
			swapinfo.minImageCount = surfcaps.minImageCount;

		// With offscreen rendering, the size is not known at first
		if (surfcaps.currentExtent.width == UINT32_MAX && surfcaps.currentExtent.height == UINT32_MAX)
		{
			swapinfo.imageExtent.width = bound(surfcaps.minImageExtent.width, vid.pixelwidth, surfcaps.maxImageExtent.width);
			swapinfo.imageExtent.height = bound(surfcaps.minImageExtent.height, vid.pixelheight, surfcaps.maxImageExtent.height);
		}
		else
		{
#ifdef _DIII4A //karin: use identity orientation
			swapinfo.imageExtent.width = bound(surfcaps.minImageExtent.width, vid.pixelwidth, surfcaps.maxImageExtent.width);
			swapinfo.imageExtent.height = bound(surfcaps.minImageExtent.height, vid.pixelheight, surfcaps.maxImageExtent.height);
#else
			swapinfo.imageExtent.width = surfcaps.currentExtent.width;
			swapinfo.imageExtent.height = surfcaps.currentExtent.height;
#endif
		}
		printf("Create swapchain extent: %d x %d\n", swapinfo.imageExtent.width, swapinfo.imageExtent.height);

		swapinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
#ifdef _DIII4A //karin: use identity orientation
		swapinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
#else
		swapinfo.preTransform = surfcaps.currentTransform;//VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
#endif
		if (surfcaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
			swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		else if (surfcaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		{
			swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
			Con_Printf(CON_WARNING"Vulkan swapchain using composite alpha premultiplied\n");
		}
		else if (surfcaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		{
			swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
			Con_Printf(CON_WARNING"Vulkan swapchain using composite alpha postmultiplied\n");
		}
		else
		{
			swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;	//erk?
			Con_Printf(CON_WARNING"composite alpha inherit\n");
		}
		swapinfo.imageArrayLayers = /*(r_stereo_method.ival==1)?2:*/1;
		swapinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapinfo.queueFamilyIndexCount = 0;
		swapinfo.pQueueFamilyIndices = NULL;
		swapinfo.clipped = vid_isfullscreen?VK_FALSE:VK_TRUE;	//allow fragment shaders to be skipped on parts that are obscured by another window. screenshots might get weird, so use proper captures if required/automagic.

		swapinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;	//support is guarenteed by spec, in theory.
		for (i = 0, curpri = 0; i < presentmodes; i++)
		{
			uint32_t priority = 0;
			switch(presentmode[i])
			{
			default://ignore it if we don't know it.
				break;
				//this is awkward. normally we use vsync<0 to allow tearing-with-vsync, but that leaves us with a problem as far as what 0 should signify - tearing or not.
				//if we're using mailbox then we could instead discard the command buffers and skip rendering of the actual scenes.
				//we could have our submission thread wait some time period after the last vswap (ie: before the next) before submitting the command.
				//this could reduce gpu load at higher resolutions without lying too much about cpu usage...
			case VK_PRESENT_MODE_IMMEDIATE_KHR:
				priority = (vk.vsync?0:2) + 2;	//for most quake players, latency trumps tearing.
				break;
			case VK_PRESENT_MODE_MAILBOX_KHR:
				priority = (vk.vsync?0:2) + 1;
				break;
			case VK_PRESENT_MODE_FIFO_KHR:
				priority = (vk.vsync?2:0) + 1;
				break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
				priority = (vk.vsync?2:0) + 2;	//strict vsync results in weird juddering if rtlights etc caues framerates to drop below the refreshrate. and nvidia just suck with vsync, so I'm not taking any chances.
				break;
			}
			if (priority > curpri)
			{
				curpri = priority;
				swapinfo.presentMode = presentmode[i];
			}
		}

		if (!vk.vsync && swapinfo.presentMode != VK_PRESENT_MODE_IMMEDIATE_KHR)
			if (!vk.swapchain)	//only warn on vid_restart, otherwise its annoying when resizing.
				Con_Printf("Warning: vulkan graphics driver does not support VK_PRESENT_MODE_IMMEDIATE_KHR.\n");

		vk.srgbcapable = false;
		swapinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapinfo.imageFormat = VK_FORMAT_UNDEFINED;
		for (i = 0, curpri = 0; i < fmtcount; i++)
		{
			uint32_t priority = 0;

			switch(surffmts[i].format)
			{
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
				priority = ((vid_bpp.ival>=24)?24:11)+BOOST_UNORM;
				break;
			case VK_FORMAT_B8G8R8A8_SNORM:
			case VK_FORMAT_R8G8B8A8_SNORM:
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
				priority = ((vid_bpp.ival>=21)?21:2)+BOOST_SNORM;
				break;
			case VK_FORMAT_B8G8R8A8_SRGB:
			case VK_FORMAT_R8G8B8A8_SRGB:
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
				priority = ((vid_bpp.ival>=24)?24:11)+BOOST_SRGB;
				vk.srgbcapable = true;
				break;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
				priority = ((vid_bpp.ival==30)?30:10)+BOOST_UNORM;
				break;

			case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
				priority = ((vid_srgb.ival>=3||vid_bpp.ival==32)?32:11)+BOOST_UFLOAT;
				break;
			case VK_FORMAT_R16G16B16A16_SFLOAT:	//16bit per-channel formats
				priority = ((vid_srgb.ival>=3||vid_bpp.ival>=48)?48:9)+BOOST_SFLOAT;
				break;
			case VK_FORMAT_R16G16B16A16_UNORM:
				priority = ((vid_srgb.ival>=3||vid_bpp.ival>=48)?48:9)+BOOST_UNORM;
				break;
			case VK_FORMAT_R16G16B16A16_SNORM:
				priority = ((vid_srgb.ival>=3||vid_bpp.ival>=48)?48:9)+BOOST_SFLOAT;
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:	//32bit per-channel formats
				priority = ((vid_bpp.ival>=47)?96:8)+BOOST_SFLOAT;
				break;

			case VK_FORMAT_B5G6R5_UNORM_PACK16:
			case VK_FORMAT_R5G6B5_UNORM_PACK16:
				priority = 16+BOOST_UNORM;
				break;
			case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
				priority = 12+BOOST_UNORM;
				break;
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
				priority = 15+BOOST_UNORM;
				break;

			default:	//no idea, use as lowest priority.
				priority = 1;
				break;
			}

			if (surffmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&				//sRGB
				surffmts[i].colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT &&	//scRGB
				surffmts[i].colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)			//linear vaugely like sRGB
				priority += 512;	//always favour supported colour spaces.

			if (priority > curpri)
			{
				curpri = priority;
				swapinfo.imageColorSpace = surffmts[i].colorSpace;
				swapinfo.imageFormat = surffmts[i].format;
			}
		}

		if (swapinfo.imageFormat == VK_FORMAT_UNDEFINED)
		{	//if we found this format then it means the drivers don't really give a damn. pick a real format.
			if (vid_srgb.ival > 1 && swapinfo.imageColorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
				swapinfo.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			else if (vid_srgb.ival)
				swapinfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
			else
				swapinfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		}

		if (vk.backbufformat != swapinfo.imageFormat)
		{
			VK_DestroyRenderPasses();
			reloadshaders = true;
		}
		vk.backbufformat = swapinfo.imageFormat;

		//VK_COLORSPACE_SRGB_NONLINEAR means the presentation engine will interpret the image as SRGB whether its a UNORM or SRGB format or not.
		//an SRGB format JUST means rendering converts linear->srgb and does not apply to the presentation engine.
		vid.flags &= ~VID_SRGB_FB;
		if (swapinfo.imageColorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
			vid.flags |= VID_SRGB_FB_LINEAR;
		else
		{
			switch(vk.backbufformat)
			{
			case VK_FORMAT_R8G8B8_SRGB:
			case VK_FORMAT_B8G8R8_SRGB:
			case VK_FORMAT_B8G8R8A8_SRGB:
			case VK_FORMAT_R8G8B8A8_SRGB:
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
				vid.flags |= VID_SRGB_FB_LINEAR;
				break;
			default:
				break;	//non-srgb (or compressed)
			}
		}

		free(presentmode);
		free(surffmts);

		if (vid_isfullscreen)	//nvidia really doesn't like this. its fine when windowed though.
			VK_DestroySwapChain();
		swapinfo.oldSwapchain = vk.swapchain;

		newvkswapchain = VK_NULL_HANDLE;
		err = vkCreateSwapchainKHR(vk.device, &swapinfo, vkallocationcb, &newvkswapchain);
		switch(err)
		{
		case VK_SUCCESS:
			break;
		default:
			Sys_Error("vkCreateSwapchainKHR returned undocumented error!\n");
		case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_INITIALIZATION_FAILED:
			if (swapinfo.oldSwapchain)
				Con_Printf(CON_WARNING"vkCreateSwapchainKHR(%u * %u) failed with error %s\n", swapinfo.imageExtent.width, swapinfo.imageExtent.height, VK_VKErrorToString(err));
			else
				Sys_Error("vkCreateSwapchainKHR(%u * %u) failed with error %s\n", swapinfo.imageExtent.width, swapinfo.imageExtent.height, VK_VKErrorToString(err));
			VK_DestroySwapChain();
			return false;
        }
		if (!newvkswapchain)
			return false;
		if (vk.swapchain)
		{
			VK_DestroySwapChain();
		}
		vk.swapchain = newvkswapchain;

		VkAssert(vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.backbuf_count, NULL));
		images = malloc(sizeof(VkImage)*vk.backbuf_count);
		memories = NULL;
		VkAssert(vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.backbuf_count, images));

		vk.acquirelast = vk.acquirenext = 0;
		for (i = 0; i < ACQUIRELIMIT; i++)
		{
			if (vk_waitfence.ival || !*vk_waitfence.string)
			{
				VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
				VkAssert(vkCreateFence(vk.device,&fci,vkallocationcb,&vk.acquirefences[i]));
				vk.acquiresemaphores[i] = VK_NULL_HANDLE;
			}
			else
			{
				VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
				VkAssert(vkCreateSemaphore(vk.device, &sci, vkallocationcb, &vk.acquiresemaphores[i]));
				DebugSetName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)vk.acquiresemaphores[i], "vk.acquiresemaphores");
				vk.acquirefences[i] = VK_NULL_HANDLE;
			}
		}
		if (!vk_submissionthread.value && *vk_submissionthread.string)
			preaquirecount = 1;	//no real point asking for more.
		else
			preaquirecount = vk.backbuf_count;
		/*-1 to hide any weird thread issues*/
		while (vk.acquirelast < ACQUIRELIMIT-1 && vk.acquirelast < preaquirecount && vk.acquirelast <= vk.backbuf_count-surfcaps.minImageCount)
		{
			VkAssert(vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.acquiresemaphores[vk.acquirelast%ACQUIRELIMIT], vk.acquirefences[vk.acquirelast%ACQUIRELIMIT], &vk.acquirebufferidx[vk.acquirelast%ACQUIRELIMIT]));
			vk.acquirelast++;
		}
	}

	oldms = vk.multisamplebits;

	vk.multisamplebits = VK_SAMPLE_COUNT_1_BIT;
	if (vid_multisample.ival>1)
	{
		VkSampleCountFlags fl = vk.limits.framebufferColorSampleCounts & vk.limits.framebufferDepthSampleCounts;
//		Con_Printf("Warning: vulkan multisample does not work with rtlights or render targets etc etc\n");
		for (i = 1; i < 30; i++)
			if ((fl & (1<<i)) && (1<<i) <= vid_multisample.ival)
				vk.multisamplebits = (1<<i);
	}

	rpassflags = RP_PRESENTABLE;

	//destroy+recreate the renderpass if something changed that prevents them being compatible (this also requires rebuilding all the pipelines too, which sucks).
	if (oldms != vk.multisamplebits || oldformat != vk.backbufformat || olddepthformat != vk.depthformat)
	{
		VK_DestroyRenderPasses();
		reloadshaders = true;
	}

	if (reloadshaders)
	{
		Shader_NeedReload(true);
		Shader_DoReload();
	}

	attachments[0] = VK_NULL_HANDLE;	//colour
	attachments[1] = VK_NULL_HANDLE;	//depth
	attachments[2] = VK_NULL_HANDLE;	//mscolour

	if (rpassflags & RP_MULTISAMPLE)
		fb_info.attachmentCount = 3;
	else
	{
		rpassflags &= ~RP_PRESENTABLE;
		fb_info.attachmentCount = 2;
	}
	fb_info.renderPass = VK_GetRenderPass(RP_FULLCLEAR|rpassflags);
	fb_info.pAttachments = attachments;
	fb_info.width = swapinfo.imageExtent.width;
	fb_info.height = swapinfo.imageExtent.height;
	fb_info.layers = 1;


	vk.backbufs = malloc(sizeof(*vk.backbufs)*vk.backbuf_count);
	memset(vk.backbufs, 0, sizeof(*vk.backbufs)*vk.backbuf_count);
	for (i = 0; i < vk.backbuf_count; i++)
	{
		VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		vk.backbufs[i].colour.image = images[i];
		DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)vk.backbufs[i].colour.image, "backbuffer");

		ivci.format = vk.backbufformat;
//		ivci.components.r = VK_COMPONENT_SWIZZLE_R;
//		ivci.components.g = VK_COMPONENT_SWIZZLE_G;
//		ivci.components.b = VK_COMPONENT_SWIZZLE_B;
//		ivci.components.a = VK_COMPONENT_SWIZZLE_A;
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.flags = 0;
		ivci.image = images[i];
		if (memories)
			vk.backbufs[i].colour.mem.memory = memories[i];
		vk.backbufs[i].colour.width = swapinfo.imageExtent.width;
		vk.backbufs[i].colour.height = swapinfo.imageExtent.height;
		VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &vk.backbufs[i].colour.view));

		vk.backbufs[i].firstuse = true;

		//create the depth buffer texture. possibly multisampled.
		{
			//depth image
			{
				VkImageCreateInfo depthinfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
				depthinfo.flags = 0;
				depthinfo.imageType = VK_IMAGE_TYPE_2D;
				depthinfo.format = vk.depthformat;
				depthinfo.extent.width = swapinfo.imageExtent.width;
				depthinfo.extent.height = swapinfo.imageExtent.height;
				depthinfo.extent.depth = 1;
				depthinfo.mipLevels = 1;
				depthinfo.arrayLayers = 1;
				depthinfo.samples = (rpassflags & RP_MULTISAMPLE)?vk.multisamplebits:VK_SAMPLE_COUNT_1_BIT;
				depthinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				depthinfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				depthinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				depthinfo.queueFamilyIndexCount = 0;
				depthinfo.pQueueFamilyIndices = NULL;
				depthinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkAssert(vkCreateImage(vk.device, &depthinfo, vkallocationcb, &vk.backbufs[i].depth.image));
				DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)vk.backbufs[i].depth.image, "backbuffer depth");
			}

			//depth memory
			VK_AllocateBindImageMemory(&vk.backbufs[i].depth, true);

			//depth view
			{
				VkImageViewCreateInfo depthviewinfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
				depthviewinfo.format = vk.depthformat;
				depthviewinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				depthviewinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				depthviewinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				depthviewinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				depthviewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;//|VK_IMAGE_ASPECT_STENCIL_BIT;
				depthviewinfo.subresourceRange.baseMipLevel = 0;
				depthviewinfo.subresourceRange.levelCount = 1;
				depthviewinfo.subresourceRange.baseArrayLayer = 0;
				depthviewinfo.subresourceRange.layerCount = 1;
				depthviewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthviewinfo.flags = 0;
				depthviewinfo.image = vk.backbufs[i].depth.image;
				VkAssert(vkCreateImageView(vk.device, &depthviewinfo, vkallocationcb, &vk.backbufs[i].depth.view));
				attachments[1] = vk.backbufs[i].depth.view;
			}
		}

		//if we're using multisampling, create the intermediate multisample texture that we're actually going to render to.
		if (rpassflags & RP_MULTISAMPLE)
		{
			//mscolour image
			{
				VkImageCreateInfo mscolourinfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
				mscolourinfo.flags = 0;
				mscolourinfo.imageType = VK_IMAGE_TYPE_2D;
				mscolourinfo.format = vk.backbufformat;
				mscolourinfo.extent.width = swapinfo.imageExtent.width;
				mscolourinfo.extent.height = swapinfo.imageExtent.height;
				mscolourinfo.extent.depth = 1;
				mscolourinfo.mipLevels = 1;
				mscolourinfo.arrayLayers = 1;
				mscolourinfo.samples = vk.multisamplebits;
				mscolourinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				mscolourinfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				mscolourinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				mscolourinfo.queueFamilyIndexCount = 0;
				mscolourinfo.pQueueFamilyIndices = NULL;
				mscolourinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkAssert(vkCreateImage(vk.device, &mscolourinfo, vkallocationcb, &vk.backbufs[i].mscolour.image));
				DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)vk.backbufs[i].mscolour.image, "multisample");
			}

			//mscolour memory
			VK_AllocateBindImageMemory(&vk.backbufs[i].mscolour, true);

			//mscolour view
			{
				VkImageViewCreateInfo mscolourviewinfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
				mscolourviewinfo.format = vk.backbufformat;
				mscolourviewinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				mscolourviewinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				mscolourviewinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				mscolourviewinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				mscolourviewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mscolourviewinfo.subresourceRange.baseMipLevel = 0;
				mscolourviewinfo.subresourceRange.levelCount = 1;
				mscolourviewinfo.subresourceRange.baseArrayLayer = 0;
				mscolourviewinfo.subresourceRange.layerCount = 1;
				mscolourviewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				mscolourviewinfo.flags = 0;
				mscolourviewinfo.image = vk.backbufs[i].mscolour.image;
				VkAssert(vkCreateImageView(vk.device, &mscolourviewinfo, vkallocationcb, &vk.backbufs[i].mscolour.view));
				attachments[2] = vk.backbufs[i].mscolour.view;
			}
		}


		attachments[0] = vk.backbufs[i].colour.view;
		VkAssert(vkCreateFramebuffer(vk.device, &fb_info, vkallocationcb, &vk.backbufs[i].framebuffer));

		{
			VkSemaphoreCreateInfo seminfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
			VkAssert(vkCreateSemaphore(vk.device, &seminfo, vkallocationcb, &vk.backbufs[i].presentsemaphore));
			DebugSetName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)vk.backbufs[i].presentsemaphore, "vk.backbufs.presentsemaphore");
		}
	}
	free(images);
	free(memories);

	vid.pixelwidth = swapinfo.imageExtent.width;
	vid.pixelheight = swapinfo.imageExtent.height;
	R2D_Console_Resize();

	return true;
}

	
void	VK_Draw_Init(void)
{
	R2D_Init();
}
void	VK_Draw_Shutdown(void)
{
	R2D_Shutdown();
	Shader_Shutdown();
	Image_Shutdown();
}

static void VK_DestroySampler(VkSampler s)
{
	struct vksamplers_s *ref;
	for (ref = vk.samplers; ref; ref = ref->next)
	{
		if (ref->samp == s)
		{
			if (--ref->usages == 0)
			{
				vkDestroySampler(vk.device, ref->samp, vkallocationcb);
				*ref->link = ref->next;
				if (ref->next)
					ref->next->link = ref->link;
				Z_Free(ref);
			}
		}
	}
}
static void VK_DestroySampler_FrameEnd(void *w)
{
	VK_DestroySampler(*(VkSampler*)w);
}

void VK_CreateSamplerInfo(VkSamplerCreateInfo *info, vk_image_t *img)
{
	unsigned int flags = IF_RENDERTARGET;
	struct vksamplers_s *ref;

	if (img->sampler)
		VK_DestroySampler(img->sampler);


	for (ref = vk.samplers; ref; ref = ref->next)
		if (ref->flags == flags)
			if (!memcmp(&ref->props, info, sizeof(*info)))
				break;

	if (!ref)
	{
		ref = Z_Malloc(sizeof(*ref));
		ref->flags = flags;
		ref->props = *info;
		ref->next = vk.samplers;
		ref->link = &vk.samplers;
		if (vk.samplers)
			vk.samplers->link = &ref->next;
		vk.samplers = ref;
		VkAssert(vkCreateSampler(vk.device, &ref->props, NULL, &ref->samp));
	}
	ref->usages++;
	img->sampler = ref->samp;
}
void VK_CreateSampler(unsigned int flags, vk_image_t *img)
{
	struct vksamplers_s *ref;
	qboolean clamptoedge = flags & IF_CLAMP;
	VkSamplerCreateInfo lmsampinfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};


	if (img->sampler)
		VK_DestroySampler(img->sampler);

	if (flags & IF_LINEAR)
	{
		lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_LINEAR;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	else if (flags & IF_NEAREST)
	{
		lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_NEAREST;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	else
	{
		int *filter = (flags & IF_UIPIC)?vk.filterpic:vk.filtermip;
		if (filter[0])
			lmsampinfo.minFilter = VK_FILTER_LINEAR;
		else
			lmsampinfo.minFilter = VK_FILTER_NEAREST;
		if (filter[1])
			lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		else
			lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		if (filter[2])
			lmsampinfo.magFilter = VK_FILTER_LINEAR;
		else
			lmsampinfo.magFilter = VK_FILTER_NEAREST;
	}

	lmsampinfo.addressModeU = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.addressModeV = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.addressModeW = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.mipLodBias = vk.lodbias;
	lmsampinfo.anisotropyEnable = (flags & IF_NEAREST)?false:(vk.max_anistophy > 1);
	lmsampinfo.maxAnisotropy = vk.max_anistophy;
	lmsampinfo.compareEnable = VK_FALSE;
	lmsampinfo.compareOp = VK_COMPARE_OP_NEVER;
	lmsampinfo.minLod = vk.mipcap[0];	//this isn't quite right
	lmsampinfo.maxLod = vk.mipcap[1];
	lmsampinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	lmsampinfo.unnormalizedCoordinates = VK_FALSE;

	for (ref = vk.samplers; ref; ref = ref->next)
		if (ref->flags == flags)
			if (!memcmp(&ref->props, &lmsampinfo, sizeof(lmsampinfo)))
				break;

	if (!ref)
	{
		ref = Z_Malloc(sizeof(*ref));
		ref->flags = flags;
		ref->props = lmsampinfo;
		ref->next = vk.samplers;
		ref->link = &vk.samplers;
		if (vk.samplers)
			vk.samplers->link = &ref->next;
		vk.samplers = ref;
		VkAssert(vkCreateSampler(vk.device, &ref->props, NULL, &ref->samp));
	}
	ref->usages++;
	img->sampler = ref->samp;
}

void VK_UpdateFiltering(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis)
{
	uint32_t i;
	for (i = 0; i < countof(vk.filtermip); i++)
		vk.filtermip[i] = filtermip[i];
	for (i = 0; i < countof(vk.filterpic); i++)
		vk.filterpic[i] = filterpic[i];
	for (i = 0; i < countof(vk.mipcap); i++)
		vk.mipcap[i] = mipcap[i];
	vk.lodbias = lodbias;
	vk.max_anistophy = bound(1.0, anis, vk.limits.maxSamplerAnisotropy);

	while(imagelist)
	{
		if (imagelist->vkimage)
		{
			if (imagelist->vkimage->sampler)
			{	//the sampler might still be in use, so clean it up at the end of the frame.
				//all this to avoid syncing all the queues...
				VK_AtFrameEnd(VK_DestroySampler_FrameEnd, &imagelist->vkimage->sampler, sizeof(imagelist->vkimage->sampler));
				imagelist->vkimage->sampler = VK_NULL_HANDLE;
			}
			VK_CreateSampler(imagelist->flags, imagelist->vkimage);
		}
		imagelist = imagelist->next;
	}
}

qboolean VK_AllocatePoolMemory(uint32_t pooltype, VkDeviceSize memsize, VkDeviceSize poolalignment, vk_poolmem_t *mem)
{
	struct vk_mempool_s *p;
	VkDeviceSize pad;

	if (!vk_usememorypools.ival)
		return false;

//	if (memsize > 1024*1024*4)
//		return false;
	for (p = vk.mempools; p; p = p->next)
	{
		if (p->memtype == pooltype)
		{
			if (p->memoryoffset + poolalignment + memsize < p->memorysize)
				break;
		}
	}
	if (!p)
	{
		VkMemoryAllocateInfo poolai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		p = Z_Malloc(sizeof(*p));
		p->memorysize = poolai.allocationSize = 512*1024*1024;	//lets just allocate big...
		p->memtype = poolai.memoryTypeIndex = pooltype;

		if (VK_SUCCESS != vkAllocateMemory(vk.device, &poolai, vkallocationcb, &p->memory))
		{	//out of memory? oh well, a smaller dedicated allocation might still work.
			Z_Free(p);
			return false;
		}
		DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)p->memory, "VK_AllocatePoolMemory");
		p->next = vk.mempools;
		vk.mempools = p;
	}
	pad = ((p->memoryoffset+poolalignment-1)&~(poolalignment-1)) - p->memoryoffset;
	p->memoryoffset = (p->memoryoffset+poolalignment-1)&~(poolalignment-1);
	p->gaps += pad;
	mem->offset = p->memoryoffset;
	mem->size = memsize;	//FIXME: we have no way to deal with gaps due to alignment
	mem->memory = p->memory;
	mem->pool = p;

	p->memoryoffset += memsize;
	return true;
}
void VK_ReleasePoolMemory(vk_poolmem_t *mem)
{
	if (mem->pool)
	{
		//FIXME: track power-of-two holes?
		mem->pool->gaps += mem->size;
		mem->pool = NULL;
		mem->memory = VK_NULL_HANDLE;
	}
	else if (mem->memory)
	{
		vkFreeMemory(vk.device, mem->memory, vkallocationcb);
		mem->memory = VK_NULL_HANDLE;
	}
}


//does NOT bind.
//image memory is NOT expected to be host-visible. you'll get what vulkan gives you.
qboolean VK_AllocateImageMemory(VkImage image, qboolean dedicated, vk_poolmem_t *mem)
{
	uint32_t pooltype;
	VkMemoryRequirements2KHR mem_reqs2 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR};

	if (!dedicated && vk.khr_get_memory_requirements2)
	{
		VkImageMemoryRequirementsInfo2KHR imri = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR};
		VkMemoryDedicatedRequirementsKHR mdr = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR};
		imri.image = image;
		if (vk.khr_dedicated_allocation)
			mem_reqs2.pNext = &mdr;	//chain the result struct
		vkGetImageMemoryRequirements2KHR(vk.device, &imri, &mem_reqs2);

		//and now we know if it should be dedicated or not.
		dedicated |= mdr.prefersDedicatedAllocation || mdr.requiresDedicatedAllocation;
	}
	else
		vkGetImageMemoryRequirements(vk.device, image, &mem_reqs2.memoryRequirements);

	pooltype = vk_find_memory_try(mem_reqs2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (pooltype == ~0)
		pooltype = vk_find_memory_require(mem_reqs2.memoryRequirements.memoryTypeBits, 0);

	if (!dedicated && VK_AllocatePoolMemory(pooltype, mem_reqs2.memoryRequirements.size, mem_reqs2.memoryRequirements.alignment, mem))
		return true;	//got a shared allocation.
	else
	{	//make it dedicated one way or another.
		VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		VkMemoryDedicatedAllocateInfoKHR khr_mdai = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};
		VkResult err;

		//shouldn't really happen, but just in case...
		mem_reqs2.memoryRequirements.size = max(1,mem_reqs2.memoryRequirements.size);

		memAllocInfo.allocationSize = mem_reqs2.memoryRequirements.size;
		memAllocInfo.memoryTypeIndex = pooltype;
		if (vk.khr_dedicated_allocation)
		{
			khr_mdai.image = image;
			khr_mdai.pNext = memAllocInfo.pNext;
			memAllocInfo.pNext = &khr_mdai;
		}

		mem->pool = NULL;
		mem->offset = 0;
		mem->size = mem_reqs2.memoryRequirements.size;
		mem->memory = VK_NULL_HANDLE;

		err = vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &mem->memory);
		if (err != VK_SUCCESS)
			return false;
		DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)mem->memory, "VK_AllocateImageMemory");
		return true;
	}
}
qboolean VK_AllocateBindImageMemory(vk_image_t *image, qboolean dedicated)
{
	if (VK_AllocateImageMemory(image->image, dedicated, &image->mem))
	{
		VkAssert(vkBindImageMemory(vk.device, image->image, image->mem.memory, image->mem.offset));
		return true;
	}
	return false;	//out of memory?
}


vk_image_t VK_CreateTexture2DArray(uint32_t width, uint32_t height, uint32_t layers, uint32_t mips, uploadfmt_t encoding, unsigned int type, qboolean rendertarget, const char *debugname)
{
	vk_image_t ret;
	VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	VkImageCreateInfo ici = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	VkFormat format = VK_FORMAT_UNDEFINED;
#ifdef VK_EXT_astc_decode_mode
	VkImageViewASTCDecodeModeEXT astcmode;
#endif

	ret.width = width;
	ret.height = height;
	ret.layers = layers;
	ret.mipcount = mips;
	ret.encoding = encoding;
	ret.type = type;
	ret.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	//vulkan expresses packed formats in terms of native endian (if big-endian, then everything makes sense), non-packed formats are expressed in byte order (consistent with big-endian).
	//PTI formats are less well-defined...
	if ((int)encoding < 0) 
		format = -(int)encoding;
	else switch(encoding)
	{
	//16bit formats.
	case PTI_RGB565:			format = VK_FORMAT_R5G6B5_UNORM_PACK16;			break;
	case PTI_RGBA4444:			format = VK_FORMAT_R4G4B4A4_UNORM_PACK16;		break;
	case PTI_ARGB4444:			/*format = VK_FORMAT_A4R4G4B4_UNORM_PACK16;*/	break;
	case PTI_RGBA5551:			format = VK_FORMAT_R5G5B5A1_UNORM_PACK16;		break;
	case PTI_ARGB1555:			format = VK_FORMAT_A1R5G5B5_UNORM_PACK16;		break;
	case PTI_R16:				format = VK_FORMAT_R16_UNORM;					break;
	case PTI_RGBA16:			format = VK_FORMAT_R16G16B16A16_UNORM;			break;
	//float formats
	case PTI_R16F:				format = VK_FORMAT_R16_SFLOAT;					break;
	case PTI_R32F:				format = VK_FORMAT_R32_SFLOAT;					break;
	case PTI_RGBA16F:			format = VK_FORMAT_R16G16B16A16_SFLOAT;			break;
	case PTI_RGBA32F:			format = VK_FORMAT_R32G32B32A32_SFLOAT;			break;
	//weird formats
	case PTI_P8:
	case PTI_R8:				format = VK_FORMAT_R8_UNORM;					break;
	case PTI_RG8:				format = VK_FORMAT_R8G8_UNORM;					break;
	case PTI_R8_SNORM:			format = VK_FORMAT_R8_SNORM;					break;
	case PTI_RG8_SNORM:			format = VK_FORMAT_R8G8_SNORM;					break;
	case PTI_A2BGR10:			format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;	break;
	case PTI_E5BGR9:			format = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;		break;
	case PTI_B10G11R11F:		format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;		break;
	//swizzled/legacy formats
	case PTI_L8:				format = VK_FORMAT_R8_UNORM;					break;
	case PTI_L8A8:				format = VK_FORMAT_R8G8_UNORM;					break;
	case PTI_L8_SRGB:			format = VK_FORMAT_R8_SRGB;						break;
	case PTI_L8A8_SRGB:			/*unsupportable*/								break;
	//compressed formats
	case PTI_BC1_RGB:			format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;			break;
	case PTI_BC1_RGB_SRGB:		format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;			break;
	case PTI_BC1_RGBA:			format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;		break;
	case PTI_BC1_RGBA_SRGB:		format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;			break;
	case PTI_BC2_RGBA:			format = VK_FORMAT_BC2_UNORM_BLOCK;				break;
	case PTI_BC2_RGBA_SRGB:		format = VK_FORMAT_BC2_SRGB_BLOCK;				break;
	case PTI_BC3_RGBA:			format = VK_FORMAT_BC3_UNORM_BLOCK;				break;
	case PTI_BC3_RGBA_SRGB:		format = VK_FORMAT_BC3_SRGB_BLOCK;				break;
	case PTI_BC4_R:				format = VK_FORMAT_BC4_UNORM_BLOCK;				break;
	case PTI_BC4_R_SNORM:		format = VK_FORMAT_BC4_SNORM_BLOCK;				break;
	case PTI_BC5_RG:			format = VK_FORMAT_BC5_UNORM_BLOCK;				break;
	case PTI_BC5_RG_SNORM:		format = VK_FORMAT_BC5_SNORM_BLOCK;				break;
	case PTI_BC6_RGB_UFLOAT:	format = VK_FORMAT_BC6H_UFLOAT_BLOCK;			break;
	case PTI_BC6_RGB_SFLOAT:	format = VK_FORMAT_BC6H_SFLOAT_BLOCK;			break;
	case PTI_BC7_RGBA:			format = VK_FORMAT_BC7_UNORM_BLOCK;				break;
	case PTI_BC7_RGBA_SRGB:		format = VK_FORMAT_BC7_SRGB_BLOCK;				break;
	case PTI_ETC1_RGB8:			format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;		break;	//vulkan doesn't support etc1, but etc2 is a superset so its all okay.
	case PTI_ETC2_RGB8:			format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;		break;
	case PTI_ETC2_RGB8_SRGB:	format = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;		break;
	case PTI_ETC2_RGB8A1:		format = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;	break;
	case PTI_ETC2_RGB8A1_SRGB:	format = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;	break;
	case PTI_ETC2_RGB8A8:		format = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;	break;
	case PTI_ETC2_RGB8A8_SRGB:	format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;	break;
	case PTI_EAC_R11:			format = VK_FORMAT_EAC_R11_UNORM_BLOCK;			break;
	case PTI_EAC_R11_SNORM:		format = VK_FORMAT_EAC_R11_SNORM_BLOCK;			break;
	case PTI_EAC_RG11:			format = VK_FORMAT_EAC_R11G11_UNORM_BLOCK;		break;
	case PTI_EAC_RG11_SNORM:	format = VK_FORMAT_EAC_R11G11_SNORM_BLOCK;		break;

	case PTI_ASTC_4X4_LDR:		format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;		break;
	case PTI_ASTC_4X4_SRGB:		format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;			break;
	case PTI_ASTC_5X4_LDR:		format = VK_FORMAT_ASTC_5x4_UNORM_BLOCK;		break;
	case PTI_ASTC_5X4_SRGB:		format = VK_FORMAT_ASTC_5x4_SRGB_BLOCK;			break;
	case PTI_ASTC_5X5_LDR:		format = VK_FORMAT_ASTC_5x5_UNORM_BLOCK;		break;
	case PTI_ASTC_5X5_SRGB:		format = VK_FORMAT_ASTC_5x5_SRGB_BLOCK;			break;
	case PTI_ASTC_6X5_LDR:		format = VK_FORMAT_ASTC_6x5_UNORM_BLOCK;		break;
	case PTI_ASTC_6X5_SRGB:		format = VK_FORMAT_ASTC_6x5_SRGB_BLOCK;			break;
	case PTI_ASTC_6X6_LDR:		format = VK_FORMAT_ASTC_6x6_UNORM_BLOCK;		break;
	case PTI_ASTC_6X6_SRGB:		format = VK_FORMAT_ASTC_6x6_SRGB_BLOCK;			break;
	case PTI_ASTC_8X5_LDR:		format = VK_FORMAT_ASTC_8x5_UNORM_BLOCK;		break;
	case PTI_ASTC_8X5_SRGB:		format = VK_FORMAT_ASTC_8x5_SRGB_BLOCK;			break;
	case PTI_ASTC_8X6_LDR:		format = VK_FORMAT_ASTC_8x6_UNORM_BLOCK;		break;
	case PTI_ASTC_8X6_SRGB:		format = VK_FORMAT_ASTC_8x6_SRGB_BLOCK;			break;
	case PTI_ASTC_8X8_LDR:		format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;		break;
	case PTI_ASTC_8X8_SRGB:		format = VK_FORMAT_ASTC_8x8_SRGB_BLOCK;			break;
	case PTI_ASTC_10X5_LDR:		format = VK_FORMAT_ASTC_10x5_UNORM_BLOCK;		break;
	case PTI_ASTC_10X5_SRGB:	format = VK_FORMAT_ASTC_10x5_SRGB_BLOCK;		break;
	case PTI_ASTC_10X6_LDR:		format = VK_FORMAT_ASTC_10x6_UNORM_BLOCK;		break;
	case PTI_ASTC_10X6_SRGB:	format = VK_FORMAT_ASTC_10x6_SRGB_BLOCK;		break;
	case PTI_ASTC_10X8_LDR:		format = VK_FORMAT_ASTC_10x8_UNORM_BLOCK;		break;
	case PTI_ASTC_10X8_SRGB:	format = VK_FORMAT_ASTC_10x8_SRGB_BLOCK;		break;
	case PTI_ASTC_10X10_LDR:	format = VK_FORMAT_ASTC_10x10_UNORM_BLOCK;		break;
	case PTI_ASTC_10X10_SRGB:	format = VK_FORMAT_ASTC_10x10_SRGB_BLOCK;		break;
	case PTI_ASTC_12X10_LDR:	format = VK_FORMAT_ASTC_12x10_UNORM_BLOCK;		break;
	case PTI_ASTC_12X10_SRGB:	format = VK_FORMAT_ASTC_12x10_SRGB_BLOCK;		break;
	case PTI_ASTC_12X12_LDR:	format = VK_FORMAT_ASTC_12x12_UNORM_BLOCK;		break;
	case PTI_ASTC_12X12_SRGB:	format = VK_FORMAT_ASTC_12x12_SRGB_BLOCK;		break;
#ifdef VK_EXT_texture_compression_astc_hdr
	case PTI_ASTC_4X4_HDR:		format = VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_5X4_HDR:		format = VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_5X5_HDR:		format = VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_6X5_HDR:		format = VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_6X6_HDR:		format = VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_8X5_HDR:		format = VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_8X6_HDR:		format = VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_8X8_HDR:		format = VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_10X5_HDR:		format = VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_10X6_HDR:		format = VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_10X8_HDR:		format = VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_10X10_HDR:	format = VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_12X10_HDR:	format = VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT;	break;
	case PTI_ASTC_12X12_HDR:	format = VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT;	break;
#else	//better than crashing.
	case PTI_ASTC_4X4_HDR:		format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;	break;
	case PTI_ASTC_5X4_HDR:		format = VK_FORMAT_ASTC_5x4_UNORM_BLOCK;	break;
	case PTI_ASTC_5X5_HDR:		format = VK_FORMAT_ASTC_5x5_UNORM_BLOCK;	break;
	case PTI_ASTC_6X5_HDR:		format = VK_FORMAT_ASTC_6x5_UNORM_BLOCK;	break;
	case PTI_ASTC_6X6_HDR:		format = VK_FORMAT_ASTC_6x6_UNORM_BLOCK;	break;
	case PTI_ASTC_8X5_HDR:		format = VK_FORMAT_ASTC_8x5_UNORM_BLOCK;	break;
	case PTI_ASTC_8X6_HDR:		format = VK_FORMAT_ASTC_8x6_UNORM_BLOCK;	break;
	case PTI_ASTC_8X8_HDR:		format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;	break;
	case PTI_ASTC_10X5_HDR:		format = VK_FORMAT_ASTC_10x5_UNORM_BLOCK;	break;
	case PTI_ASTC_10X6_HDR:		format = VK_FORMAT_ASTC_10x6_UNORM_BLOCK;	break;
	case PTI_ASTC_10X8_HDR:		format = VK_FORMAT_ASTC_10x8_UNORM_BLOCK;	break;
	case PTI_ASTC_10X10_HDR:	format = VK_FORMAT_ASTC_10x10_UNORM_BLOCK;	break;
	case PTI_ASTC_12X10_HDR:	format = VK_FORMAT_ASTC_12x10_UNORM_BLOCK;	break;
	case PTI_ASTC_12X12_HDR:	format = VK_FORMAT_ASTC_12x12_UNORM_BLOCK;	break;
#endif

#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:	//vulkan doesn't support these for some reason
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_3X3X3_LDR:
	case PTI_ASTC_4X3X3_LDR:
	case PTI_ASTC_4X4X3_LDR:
	case PTI_ASTC_4X4X4_LDR:
	case PTI_ASTC_5X4X4_LDR:
	case PTI_ASTC_5X5X4_LDR:
	case PTI_ASTC_5X5X5_LDR:
	case PTI_ASTC_6X5X5_LDR:
	case PTI_ASTC_6X6X5_LDR:
	case PTI_ASTC_6X6X6_LDR:
	case PTI_ASTC_3X3X3_SRGB:
	case PTI_ASTC_4X3X3_SRGB:
	case PTI_ASTC_4X4X3_SRGB:
	case PTI_ASTC_4X4X4_SRGB:
	case PTI_ASTC_5X4X4_SRGB:
	case PTI_ASTC_5X5X4_SRGB:
	case PTI_ASTC_5X5X5_SRGB:
	case PTI_ASTC_6X5X5_SRGB:
	case PTI_ASTC_6X6X5_SRGB:
	case PTI_ASTC_6X6X6_SRGB:	break;
#endif

	//depth formats
	case PTI_DEPTH16:			format = VK_FORMAT_D16_UNORM;					break;
	case PTI_DEPTH24:			format = VK_FORMAT_X8_D24_UNORM_PACK32;			break;
	case PTI_DEPTH32:			format = VK_FORMAT_D32_SFLOAT;					break;
	case PTI_DEPTH24_8:			format = VK_FORMAT_D24_UNORM_S8_UINT;			break;
	//srgb formats
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:		format = VK_FORMAT_B8G8R8A8_SRGB;				break;
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:		format = VK_FORMAT_R8G8B8A8_SRGB;				break;
	//standard formats
	case PTI_BGRA8:
	case PTI_BGRX8:				format = VK_FORMAT_B8G8R8A8_UNORM;				break;
	case PTI_RGBA8:
	case PTI_RGBX8:				format = VK_FORMAT_R8G8B8A8_UNORM;				break;
	//misaligned formats
	case PTI_RGB8:				format = VK_FORMAT_R8G8B8_UNORM;				break;
	case PTI_BGR8:				format = VK_FORMAT_B8G8R8_UNORM;				break;
	case PTI_RGB32F:			format = VK_FORMAT_R32G32B32_SFLOAT;			break;

	case PTI_RGB8_SRGB:			format = VK_FORMAT_R8G8B8_SRGB;					break;
	case PTI_BGR8_SRGB:			format = VK_FORMAT_B8G8R8_SRGB;					break;

	//unsupported 'formats'
	case PTI_MAX:
#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:
#endif
	case PTI_EMULATED:
		break;
	}
	if (format == VK_FORMAT_UNDEFINED)	//no default case means warnings for unsupported formats above.
		Sys_Error("VK_CreateTexture2DArray: Unsupported image encoding: %u(%s)\n", encoding, Image_FormatName(encoding));

	ici.flags = (ret.type==PTI_CUBE)?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = format;
	ici.extent.width = width;
	ici.extent.height = height;
	ici.extent.depth = 1;
	ici.mipLevels = mips;
	ici.arrayLayers = layers;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT|(rendertarget?0:VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ici.queueFamilyIndexCount = 0;
	ici.pQueueFamilyIndices = NULL;
	ici.initialLayout = ret.layout;

	VkAssert(vkCreateImage(vk.device, &ici, vkallocationcb, &ret.image));
	DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)ret.image, debugname);

	ret.view = VK_NULL_HANDLE;
	ret.sampler = VK_NULL_HANDLE;

	if (!VK_AllocateBindImageMemory(&ret, false))
		return ret;	//oom?


	viewInfo.flags = 0;
	viewInfo.image = ret.image;
	switch(ret.type)
	{
	default:
		return ret;
	case PTI_CUBE:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		break;
	case PTI_2D:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case PTI_2D_ARRAY:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		break;
	}
	viewInfo.format = format;
	switch(encoding)
	{
	//formats that explicitly drop the alpha
	case PTI_BC1_RGB:
	case PTI_BC1_RGB_SRGB:
	case PTI_RGBX8:
	case PTI_RGBX8_SRGB:
	case PTI_BGRX8:
	case PTI_BGRX8_SRGB:
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;
		break;
	case PTI_L8:		//must be an R8 texture
	case PTI_L8_SRGB:	//must be an R8 texture
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;
		break;
	case PTI_L8A8:	//must be an RG8 texture
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_G;
		break;
	default:
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		break;

#ifdef VK_EXT_astc_decode_mode
	case PTI_ASTC_4X4_LDR:	//set these to use rgba8 decoding, because we know they're not hdr and the format is basically 8bit anyway.
	case PTI_ASTC_5X4_LDR:	//we do NOT do this for the hdr, as that would cause data loss.
	case PTI_ASTC_5X5_LDR:	//we do NOT do this for sRGB because its pointless.
	case PTI_ASTC_6X5_LDR:
	case PTI_ASTC_6X6_LDR:
	case PTI_ASTC_8X5_LDR:
	case PTI_ASTC_8X6_LDR:
	case PTI_ASTC_8X8_LDR:
	case PTI_ASTC_10X5_LDR:
	case PTI_ASTC_10X6_LDR:
	case PTI_ASTC_10X8_LDR:
	case PTI_ASTC_10X10_LDR:
	case PTI_ASTC_12X10_LDR:
	case PTI_ASTC_12X12_LDR:
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		if (vk.ext_astc_decode_mode)
		{
			astcmode.pNext = viewInfo.pNext;
			astcmode.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
			astcmode.decodeMode = VK_FORMAT_R8G8B8A8_UNORM;
			viewInfo.pNext = &astcmode;
		}
		break;
#endif
	}
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mips;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;
	VkAssert(vkCreateImageView(vk.device, &viewInfo, NULL, &ret.view));

	return ret;
}
void set_image_layout(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspectMask, 
		VkImageLayout old_image_layout, VkAccessFlags srcaccess, VkPipelineStageFlagBits srcstagemask,
	       	VkImageLayout new_image_layout, VkAccessFlags dstaccess, VkPipelineStageFlagBits dststagemask)
{
	//images have weird layout representations.
	//we need to use a side-effect of memory barriers in order to convert from one layout to another, so that we can actually use the image.
	VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	imgbarrier.pNext = NULL;
	imgbarrier.srcAccessMask = srcaccess;
	imgbarrier.dstAccessMask = dstaccess;
	imgbarrier.oldLayout = old_image_layout;
	imgbarrier.newLayout = new_image_layout;
	imgbarrier.image = image;
	imgbarrier.subresourceRange.aspectMask = aspectMask;
	imgbarrier.subresourceRange.baseMipLevel = 0;
	imgbarrier.subresourceRange.levelCount = 1;
	imgbarrier.subresourceRange.baseArrayLayer = 0;
	imgbarrier.subresourceRange.layerCount = 1;
	imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
/*
	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)	// Make sure anything that was copying from this image has completed
		imgbarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	else if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)	// Make sure anything that was copying from this image has completed
		imgbarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	else if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		imgbarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	else if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		imgbarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	else if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // Make sure any Copy or CPU writes to image are flushed 
		imgbarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED)
		imgbarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	else if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		imgbarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	else if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		imgbarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
*/
	vkCmdPipelineBarrier(cmd, srcstagemask, dststagemask, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
}

void VK_FencedCheck(void)
{
	while(vk.fencework)
	{
		Sys_LockConditional(vk.submitcondition);
		if (VK_SUCCESS == vkGetFenceStatus(vk.device, vk.fencework->fence))
		{
			struct vk_fencework *w;
			w = vk.fencework;
			vk.fencework = w->next;
			if (!vk.fencework)
				vk.fencework_last = NULL;
			Sys_UnlockConditional(vk.submitcondition);

			if (w->Passed)
				w->Passed(w);
			if (w->cbuf)
				vkFreeCommandBuffers(vk.device, vk.cmdpool, 1, &w->cbuf);
			if (w->fence)
				vkDestroyFence(vk.device, w->fence, vkallocationcb);
			Z_Free(w);
			continue;
		}
		Sys_UnlockConditional(vk.submitcondition);
		break;
	}
}
//allocate and begin a commandbuffer so we can do the copies
void *VK_FencedBegin(void (*passed)(void *work), size_t worksize)
{
	struct vk_fencework *w = BZ_Malloc(worksize?worksize:sizeof(*w));

	VkCommandBufferAllocateInfo cbai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	VkCommandBufferInheritanceInfo cmdinh = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
	VkCommandBufferBeginInfo cmdinf = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	cbai.commandPool = vk.cmdpool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = 1;
	VkAssert(vkAllocateCommandBuffers(vk.device, &cbai, &w->cbuf));
	DebugSetName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)w->cbuf, "VK_FencedBegin");
	cmdinf.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmdinf.pInheritanceInfo = &cmdinh;
	vkBeginCommandBuffer(w->cbuf, &cmdinf);

	w->Passed = passed;
	w->next = NULL;

	return w;
}
//end+submit a commandbuffer, and set up a fence so we know when its complete. this is not within the context of any frame, so make sure any textures are safe to rewrite early...
//completion can be signalled before the current frame finishes, so watch out for that too.
void VK_FencedSubmit(void *work)
{
	struct vk_fencework *w = work;
	VkFenceCreateInfo fenceinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

	if (w->cbuf)
		vkEndCommandBuffer(w->cbuf);

	//check if we can release anything yet.
	VK_FencedCheck();

	//FIXME: this seems to be an excessively expensive function.
	vkCreateFence(vk.device, &fenceinfo, vkallocationcb, &w->fence);

	VK_Submit_Work(w->cbuf, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, w->fence, NULL, w);
}

void VK_FencedSync(void *work)
{
	struct vk_fencework *w = work;
	VK_FencedSubmit(w);

#ifdef MULTITHREAD
	//okay, this is crazy, but it ensures that the work was submitted BEFORE the WaitForFence call.
	//we should probably come up with a better sync method.
	if (vk.submitthread)
	{
		qboolean nnsc = vk.neednewswapchain;
		vk.neednewswapchain = true;
		Sys_LockConditional(vk.submitcondition);	//annoying, but required for it to be reliable with respect to other things.
		Sys_ConditionSignal(vk.submitcondition);
		Sys_UnlockConditional(vk.submitcondition);
		Sys_WaitOnThread(vk.submitthread);
		vk.submitthread = NULL;

		while (vk.work)
		{
			Sys_LockConditional(vk.submitcondition);
			VK_Submit_DoWork();
			Sys_UnlockConditional(vk.submitcondition);
		}

		//we know all work is synced now...

		vk.neednewswapchain = nnsc;
		vk.submitthread = Sys_CreateThread("vksubmission", VK_Submit_Thread, NULL, THREADP_HIGHEST, 0);
	}
#endif

	//fixme: waiting for the fence while it may still be getting created by the worker is unsafe.
	vkWaitForFences(vk.device, 1, &w->fence, VK_FALSE, UINT64_MAX);
}

//called to schedule the release of a resource that may be referenced by an active command buffer.
//the command buffer in question may even have not yet been submitted yet.
void *VK_AtFrameEnd(void (*frameended)(void *work), void *workdata, size_t worksize)
{
	struct vk_frameend *w = Z_Malloc(sizeof(*w) + worksize);

	w->FrameEnded = frameended;
	w->next = vk.frameendjobs;
	vk.frameendjobs = w;

	if (workdata)
		memcpy(w+1, workdata, worksize);

	return w+1;
}

struct texturefence
{
	struct vk_fencework w;

	int mips;
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingmemory;
};
static void VK_TextureLoaded(void *ctx)
{
	struct texturefence *w = ctx;
	vkDestroyBuffer(vk.device, w->stagingbuffer, vkallocationcb);
	vkFreeMemory(vk.device, w->stagingmemory, vkallocationcb);
}
qboolean VK_LoadTextureMips (texid_t tex, const struct pendingtextureinfo *mips)
{
	VkBufferCreateInfo bci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	void *mapdata;

	struct texturefence *fence;
	VkCommandBuffer vkloadcmd;
	vk_image_t target;
	uint32_t i;
	uint32_t blockwidth, blockheight, blockdepth;
	uint32_t blockbytes;
	uint32_t layers;
	uint32_t mipcount = mips->mipcount;
	switch(mips->type)
	{
	case PTI_2D:
		if (!mipcount || mips->mip[0].width == 0 || mips->mip[0].height == 0 || mips->mip[0].depth != 1)
			return false;
		break;
	case PTI_2D_ARRAY:
		if (!mipcount || mips->mip[0].width == 0 || mips->mip[0].height == 0 || mips->mip[0].depth == 0)
			return false;
		break;
	case PTI_CUBE:
		if (!mipcount || mips->mip[0].width == 0 || mips->mip[0].height == 0 || mips->mip[0].depth != 6)
			return false;
		break;
	default:
		return false;
	}

	layers = mips->mip[0].depth;

	if (layers == 1 && mipcount > 1)
	{	//npot mipmapped textures are awkward.
		//vulkan floors.
		for (i = 1; i < mipcount; i++)
		{
			if (mips->mip[i].width != max(1,(mips->mip[i-1].width>>1)) ||
				mips->mip[i].height != max(1,(mips->mip[i-1].height>>1)))
			{	//okay, this mip looks like it was sized wrongly.
				mipcount = i;
				break;
			}
		}
	}

	Image_BlockSizeForEncoding(mips->encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	fence = VK_FencedBegin(VK_TextureLoaded, sizeof(*fence));
	fence->mips = mipcount;
	vkloadcmd = fence->w.cbuf;

	//create our target image

	if (tex->vkimage)
	{
		if (tex->vkimage->width != mips->mip[0].width ||
			tex->vkimage->height != mips->mip[0].height ||
			tex->vkimage->layers != layers ||
			tex->vkimage->mipcount != mipcount ||
			tex->vkimage->encoding != mips->encoding ||
			tex->vkimage->type != mips->type)
		{
			VK_AtFrameEnd(VK_DestroyVkTexture_Delayed, tex->vkimage, sizeof(*tex->vkimage));
//			vkDeviceWaitIdle(vk.device);	//erk, we can't cope with a commandbuffer poking the texture while things happen
//			VK_FencedCheck();
//			VK_DestroyVkTexture(tex->vkimage);
			Z_Free(tex->vkimage);
			tex->vkimage = NULL;
		}
	}

	if (tex->vkimage)
	{
		target = *tex->vkimage;	//can reuse it
		Z_Free(tex->vkimage);
		//we're meant to be replacing the entire thing, so we can just transition from undefined here
//		set_image_layout(vkloadcmd, target.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);

		{
			//images have weird layout representations.
			//we need to use a side-effect of memory barriers in order to convert from one layout to another, so that we can actually use the image.
			VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			imgbarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imgbarrier.newLayout = target.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgbarrier.image = target.image;
			imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgbarrier.subresourceRange.baseMipLevel = 0;
			imgbarrier.subresourceRange.levelCount = mipcount;
			imgbarrier.subresourceRange.baseArrayLayer = 0;
			imgbarrier.subresourceRange.layerCount = layers;
			imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imgbarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imgbarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(vkloadcmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
		}
	}
	else
	{
		target = VK_CreateTexture2DArray(mips->mip[0].width, mips->mip[0].height, layers, mipcount, mips->encoding, mips->type, !!(tex->flags&IF_RENDERTARGET), tex->ident);

		if (target.mem.memory == VK_NULL_HANDLE)
		{
			VK_DestroyVkTexture(&target);
			return false;	//the alloc failed? can't copy to that which does not exist.
		}

		{
			//images have weird layout representations.
			//we need to use a side-effect of memory barriers in order to convert from one layout to another, so that we can actually use the image.
			VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			imgbarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imgbarrier.newLayout = target.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imgbarrier.image = target.image;
			imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgbarrier.subresourceRange.baseMipLevel = 0;
			imgbarrier.subresourceRange.levelCount = mipcount;
			imgbarrier.subresourceRange.baseArrayLayer = 0;
			imgbarrier.subresourceRange.layerCount = layers;
			imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imgbarrier.srcAccessMask = 0;
			imgbarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(vkloadcmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
		}
	}

	//figure out how big our staging buffer needs to be
	bci.size = 0;
	for (i = 0; i < mipcount; i++)
	{
		uint32_t blockswidth = (mips->mip[i].width+blockwidth-1) / blockwidth;
		uint32_t blocksheight = (mips->mip[i].height+blockheight-1) / blockheight;
		uint32_t blocksdepth = (mips->mip[i].depth+blockdepth-1) / blockdepth;
		bci.size += blockswidth*blocksheight*blocksdepth*blockbytes;
	}
	bci.flags = 0;
	bci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bci.queueFamilyIndexCount = 0;
	bci.pQueueFamilyIndices = NULL;

	//FIXME: nvidia's vkCreateBuffer ends up calling NtYieldExecution.
	//which is basically a waste of time, and its hurting framerates.

	//create+map the staging buffer
	VkAssert(vkCreateBuffer(vk.device, &bci, vkallocationcb, &fence->stagingbuffer));
	vkGetBufferMemoryRequirements(vk.device, fence->stagingbuffer, &mem_reqs);
	memAllocInfo.allocationSize = mem_reqs.size;
	memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	if (VK_SUCCESS != vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &fence->stagingmemory))
	{
		VK_FencedSubmit(fence);
		return false;	//some sort of oom error?
	}
	DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)fence->stagingmemory, "VK_LoadTextureMips");
	VkAssert(vkBindBufferMemory(vk.device, fence->stagingbuffer, fence->stagingmemory, 0));
	VkAssert(vkMapMemory(vk.device, fence->stagingmemory, 0, bci.size, 0, &mapdata));
	if (!mapdata)
		Sys_Error("Unable to map staging image\n");

	bci.size = 0;
	for (i = 0; i < mipcount; i++)
	{
		size_t mipofs = 0;
		VkBufferImageCopy region;
		//figure out the number of 'blocks' in the image.
		//for non-compressed formats this is just the width directly.
		//for compressed formats (ie: s3tc/dxt) we need to round up to deal with npot.
		uint32_t blockswidth = (mips->mip[i].width+blockwidth-1) / blockwidth;
		uint32_t blocksheight = (mips->mip[i].height+blockheight-1) / blockheight;
		uint32_t blocksdepth = (mips->mip[i].depth+blockdepth-1) / blockdepth, z;

		//build it in layers...
		for (z = 0; z < blocksdepth; z++)
		{
			if (mips->mip[i].data)
				memcpy((char*)mapdata + bci.size, (char*)mips->mip[i].data+mipofs, blockswidth*blockbytes*blocksheight*blockdepth);
			else
				memset((char*)mapdata + bci.size, 0, blockswidth*blockbytes*blocksheight*blockdepth);

			//queue up a buffer->image copy for this mip
			region.bufferOffset = bci.size;
			region.bufferRowLength = blockswidth*blockwidth;
			region.bufferImageHeight = blocksheight*blockheight;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = i;
			region.imageSubresource.baseArrayLayer = z*blockdepth;
			region.imageSubresource.layerCount = blockdepth;
			region.imageOffset.x = 0;
			region.imageOffset.y = 0;
			region.imageOffset.z = 0;
			region.imageExtent.width = mips->mip[i].width;
			region.imageExtent.height = mips->mip[i].height;
			region.imageExtent.depth = blockdepth;

			vkCmdCopyBufferToImage(vkloadcmd, fence->stagingbuffer, target.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			bci.size += blockdepth*blockswidth*blocksheight*blockbytes;
			mipofs += blockdepth*blockswidth*blocksheight*blockbytes;
		}
	}
	vkUnmapMemory(vk.device, fence->stagingmemory);

	//layouts are annoying. and weird.
	{
		//images have weird layout representations.
		//we need to use a side-effect of memory barriers in order to convert from one layout to another, so that we can actually use the image.
		VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imgbarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgbarrier.newLayout = target.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgbarrier.image = target.image;
		imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgbarrier.subresourceRange.baseMipLevel = 0;
		imgbarrier.subresourceRange.levelCount = mipcount;
		imgbarrier.subresourceRange.baseArrayLayer = 0;
		imgbarrier.subresourceRange.layerCount = layers;
		imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		imgbarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgbarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		vkCmdPipelineBarrier(vkloadcmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
	}

	VK_FencedSubmit(fence);

	//FIXME: should probably reuse these samplers.
	if (!target.sampler)
		VK_CreateSampler(tex->flags, &target);

	tex->vkdescriptor = VK_NULL_HANDLE;

	tex->vkimage = Z_Malloc(sizeof(*tex->vkimage));
	*tex->vkimage = target;

	return true;
}
void    VK_DestroyTexture			(texid_t tex)
{
	if (tex->vkimage)
	{
		VK_DestroyVkTexture(tex->vkimage);
		Z_Free(tex->vkimage);
		tex->vkimage = NULL;
	}
	tex->vkdescriptor = VK_NULL_HANDLE;
}




void	VK_R_Init					(void)
{
	uint32_t white[6] = {~0u,~0u,~0u,~0u,~0u,~0u};
	r_blackcubeimage = Image_CreateTexture("***blackcube***", NULL, IF_NEAREST|IF_TEXTYPE_CUBE|IF_NOPURGE);
	Image_Upload(r_blackcubeimage, TF_RGBX32, NULL, NULL, 1, 1, 6, IF_NEAREST|IF_NOMIPMAP|IF_NOGAMMA|IF_TEXTYPE_CUBE);

	r_whitecubeimage = Image_CreateTexture("***whitecube***", NULL, IF_NEAREST|IF_TEXTYPE_CUBE|IF_NOPURGE);
	Image_Upload(r_whitecubeimage, TF_RGBX32, white, NULL, 1, 1, 6, IF_NEAREST|IF_NOMIPMAP|IF_NOGAMMA|IF_TEXTYPE_CUBE);
}
void	VK_R_DeInit					(void)
{
	R_GAliasFlushSkinCache(true);
	Surf_DeInit();
	VK_Shutdown_PostProc();
	VK_DestroySwapChain();
	VKBE_Shutdown();

	R2D_Shutdown();
	Shader_Shutdown();
	Image_Shutdown();
}

void VK_SetupViewPortProjection(qboolean flipy, const float eyematrix[12], const float fovoverrides[4], const float projmatrix[16])
{
	float fov_x, fov_y;
	float fovv_x, fovv_y;

	float fov_l, fov_r, fov_d, fov_u;

	if (eyematrix)
	{
		extern cvar_t in_vraim;
		matrix3x4 basematrix;
		matrix3x4 viewmatrix;
		vec3_t newa;

		if (r_refdef.base_known)
		{	//mod is specifying its own base ang+org.
			Matrix3x4_RM_FromAngles(r_refdef.base_angles, r_refdef.base_origin, basematrix[0]);
		}
		else
		{	//mod provides no info.
			//client will fiddle with input_angles
			newa[0] = newa[2] = 0;	//ignore player pitch+roll. sorry. apply the eye's transform on top.
			newa[1] = r_refdef.viewangles[1];
			if (in_vraim.ival)
				newa[1] -= SHORT2ANGLE(r_refdef.playerview->vrdev[VRDEV_HEAD].angles[YAW]);
			Matrix3x4_RM_FromAngles(newa, r_refdef.vieworg, basematrix[0]);
		}
		Matrix3x4_Multiply(eyematrix, basematrix[0], viewmatrix[0]);
		Matrix3x4_RM_ToVectors(viewmatrix[0], vpn, vright, vup, r_origin);
		VectorNegate(vright, vright);

	}
	else
	{
		AngleVectors (r_refdef.viewangles, vpn, vright, vup);
		VectorCopy (r_refdef.vieworg, r_origin);
	}

//	screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;

	/*view matrix*/
	if (flipy)	//mimic gl and give bottom-up
	{
		vec3_t down;
		VectorNegate(vup, down);
		VectorCopy(down, vup);
		Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, down, r_refdef.vieworg);
		r_refdef.flipcull = SHADER_CULL_FRONT | SHADER_CULL_BACK;
	}
	else
	{
		Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);
		r_refdef.flipcull = 0;
	}

	if (projmatrix)
	{
		memcpy(r_refdef.m_projection_std, projmatrix, sizeof(r_refdef.m_projection_std));
		memcpy(r_refdef.m_projection_view, projmatrix, sizeof(r_refdef.m_projection_std));
	}
	else
	{
		fov_x = r_refdef.fov_x;//+sin(cl.time)*5;
		fov_y = r_refdef.fov_y;//-sin(cl.time+1)*5;
		fovv_x = r_refdef.fovv_x;
		fovv_y = r_refdef.fovv_y;
		if ((r_refdef.flags & RDF_UNDERWATER) && !(r_refdef.flags & RDF_WATERWARP))
		{
			fov_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
			fov_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
			fovv_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
			fovv_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
		}
		if (fovoverrides)
		{
			fov_l = fovoverrides[0];
			fov_r = fovoverrides[1];
			fov_d = fovoverrides[2];
			fov_u = fovoverrides[3];

			fov_x = fov_r-fov_l;
			fov_y = fov_u-fov_d;

			fovv_x = fov_x;
			fovv_y = fov_y;
			r_refdef.flipcull = ((fov_u < fov_d)^(fov_r < fov_l))?SHADER_CULL_FLIP:0;
		}
		else
		{
			fov_l = -fov_x / 2;
			fov_r = fov_x / 2;
			fov_d = -fov_y / 2;
			fov_u = fov_y / 2;
		}

		if (r_xflip.ival)
		{
			float t = fov_l;
			fov_l = fov_r;
			fov_r = t;
			r_refdef.flipcull ^= SHADER_CULL_FLIP;
			fovv_x *= -1;
		}

		Matrix4x4_CM_Projection_Offset(r_refdef.m_projection_std, fov_l, fov_r, fov_d, fov_u, r_refdef.mindist, r_refdef.maxdist, false);
		Matrix4x4_CM_Projection_Offset(r_refdef.m_projection_view, -fovv_x/2, fovv_x/2, -fovv_y/2, fovv_y/2, r_refdef.mindist, r_refdef.maxdist, false);
	}

	r_refdef.m_projection_view[2+4*0] *= 0.333;
	r_refdef.m_projection_view[2+4*1] *= 0.333;
	r_refdef.m_projection_view[2+4*2] *= 0.333;
	r_refdef.m_projection_view[2+4*3] *= 0.333;
}

void VK_Set2D(void)
{
	vid.fbvwidth = vid.width;
	vid.fbvheight = vid.height;
	vid.fbpwidth = vid.pixelwidth;
	vid.fbpheight = vid.pixelheight;

	r_refdef.pxrect.x = 0;
	r_refdef.pxrect.y = 0;
	r_refdef.pxrect.width = vid.fbpwidth;
	r_refdef.pxrect.height = vid.fbpheight;
	r_refdef.pxrect.maxheight = vid.pixelheight;

/*
	{
		VkClearDepthStencilValue val;
		VkImageSubresourceRange range;
		val.depth = 1;
		val.stencil = 0;
		range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		range.baseArrayLayer = 0;
		range.baseMipLevel = 0;
		range.layerCount = 1;
		range.levelCount = 1;
		vkCmdClearDepthStencilImage(vk.frame->cbuf, vk.depthbuf.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &val, 1, &range);
	}
*/
	
/*
	vkCmdEndRenderPass(vk.frame->cbuf);
	{
		VkRenderPassBeginInfo rpiinfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		VkClearValue	clearvalues[1];
		clearvalues[0].depthStencil.depth = 1.0;
		clearvalues[0].depthStencil.stencil = 0;
		rpiinfo.renderPass = VK_GetRenderPass(RP_CLEARDEPTH);
		rpiinfo.renderArea.offset.x = r_refdef.pxrect.x;
		rpiinfo.renderArea.offset.y = r_refdef.pxrect.y;
		rpiinfo.renderArea.extent.width = r_refdef.pxrect.width;
		rpiinfo.renderArea.extent.height = r_refdef.pxrect.height;
		rpiinfo.framebuffer = vk.frame->backbuf->framebuffer;
		rpiinfo.clearValueCount = 1;
		rpiinfo.pClearValues = clearvalues;
		vkCmdBeginRenderPass(vk.frame->cbuf, &rpiinfo, VK_SUBPASS_CONTENTS_INLINE);
	}
*/
	{
		VkViewport vp[1];
		VkRect2D scissor[1];
		vp[0].x = r_refdef.pxrect.x;
		vp[0].y = r_refdef.pxrect.y;
		vp[0].width = r_refdef.pxrect.width;
		vp[0].height = r_refdef.pxrect.height;
		vp[0].minDepth = 0.0;
		vp[0].maxDepth = 1.0;
		scissor[0].offset.x = r_refdef.pxrect.x;
		scissor[0].offset.y = r_refdef.pxrect.y;
		scissor[0].extent.width = r_refdef.pxrect.width;
		scissor[0].extent.height = r_refdef.pxrect.height;
		vkCmdSetViewport(vk.rendertarg->cbuf, 0, countof(vp), vp);
		vkCmdSetScissor(vk.rendertarg->cbuf, 0, countof(scissor), scissor);
	}

	VKBE_Set2D(true);

	r_refdef.flipcull = 0;
	if (0)
		Matrix4x4_CM_Orthographic(r_refdef.m_projection_std, 0, vid.fbvwidth, 0, vid.fbvheight, -99999, 99999);
	else
		Matrix4x4_CM_Orthographic(r_refdef.m_projection_std, 0, vid.fbvwidth, vid.fbvheight, 0, -99999, 99999);
	Matrix4x4_Identity(r_refdef.m_view);

	BE_SelectEntity(&r_worldentity);
}

static void VK_Shutdown_PostProc(void)
{
	unsigned int i;
	if (vk.device)
	{
		for (i = 0; i < countof(postproc); i++)
			VKBE_RT_Gen(&postproc[i], NULL, 0, 0, true, RT_IMAGEFLAGS);
		VK_R_BloomShutdown();
	}

	vk.scenepp_waterwarp = NULL;
	vk.scenepp_antialias = NULL;
}
static void VK_Init_PostProc(void)
{
	texid_t scenepp_texture_warp, scenepp_texture_edge;
	//this block liberated from the opengl code
	{
#define PP_WARP_TEX_SIZE 64
#define PP_AMP_TEX_SIZE 64
#define PP_AMP_TEX_BORDER 4
		int i, x, y;
		unsigned char pp_warp_tex[PP_WARP_TEX_SIZE*PP_WARP_TEX_SIZE*4];
		unsigned char pp_edge_tex[PP_AMP_TEX_SIZE*PP_AMP_TEX_SIZE*4];

//		scenepp_postproc_cube = r_nulltex;

//		TEXASSIGN(sceneblur_texture, Image_CreateTexture("***postprocess_blur***", NULL, 0));

		TEXASSIGN(scenepp_texture_warp, Image_CreateTexture("***postprocess_warp***", NULL, IF_NOMIPMAP|IF_NOGAMMA|IF_LINEAR));
		TEXASSIGN(scenepp_texture_edge, Image_CreateTexture("***postprocess_edge***", NULL, IF_NOMIPMAP|IF_NOGAMMA|IF_LINEAR));

		// init warp texture - this specifies offset in
		for (y=0; y<PP_WARP_TEX_SIZE; y++)
		{
			for (x=0; x<PP_WARP_TEX_SIZE; x++)
			{
				float fx, fy;

				i = (x + y*PP_WARP_TEX_SIZE) * 4;

				fx = sin(((double)y / PP_WARP_TEX_SIZE) * M_PI * 2);
				fy = cos(((double)x / PP_WARP_TEX_SIZE) * M_PI * 2);

				pp_warp_tex[i  ] = (fx+1.0f)*127.0f;
				pp_warp_tex[i+1] = (fy+1.0f)*127.0f;
				pp_warp_tex[i+2] = 0;
				pp_warp_tex[i+3] = 0xff;
			}
		}

		Image_Upload(scenepp_texture_warp, TF_RGBX32, pp_warp_tex, NULL, PP_WARP_TEX_SIZE, PP_WARP_TEX_SIZE, 1, IF_LINEAR|IF_NOMIPMAP|IF_NOGAMMA);

		// TODO: init edge texture - this is ampscale * 2, with ampscale calculated
		// init warp texture - this specifies offset in
		for (y=0; y<PP_AMP_TEX_SIZE; y++)
		{
			for (x=0; x<PP_AMP_TEX_SIZE; x++)
			{
				float fx = 1, fy = 1;

				i = (x + y*PP_AMP_TEX_SIZE) * 4;

				if (x < PP_AMP_TEX_BORDER)
				{
					fx = (float)x / PP_AMP_TEX_BORDER;
				}
				if (x > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
				{
					fx = (PP_AMP_TEX_SIZE - (float)x) / PP_AMP_TEX_BORDER;
				}
				
				if (y < PP_AMP_TEX_BORDER)
				{
					fy = (float)y / PP_AMP_TEX_BORDER;
				}
				if (y > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
				{
					fy = (PP_AMP_TEX_SIZE - (float)y) / PP_AMP_TEX_BORDER;
				}

				//avoid any sudden changes.
				fx=sin(fx*M_PI*0.5);
				fy=sin(fy*M_PI*0.5);

				//lame
				fx = fy = min(fx, fy);

				pp_edge_tex[i  ] = fx * 255;
				pp_edge_tex[i+1] = fy * 255;
				pp_edge_tex[i+2] = 0;
				pp_edge_tex[i+3] = 0xff;
			}
		}

		Image_Upload(scenepp_texture_edge, TF_RGBX32, pp_edge_tex, NULL, PP_AMP_TEX_SIZE, PP_AMP_TEX_SIZE, 1, IF_LINEAR|IF_NOMIPMAP|IF_NOGAMMA);
	}


	vk.scenepp_waterwarp = R_RegisterShader("waterwarp", SUF_NONE,
		"{\n"
			"program underwaterwarp\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
			"{\n"
				"map $upperoverlay\n"
			"}\n"
			"{\n"
				"map $loweroverlay\n"
			"}\n"
		"}\n"
		);
	vk.scenepp_waterwarp->defaulttextures->upperoverlay = scenepp_texture_warp;
	vk.scenepp_waterwarp->defaulttextures->loweroverlay = scenepp_texture_edge;

	vk.scenepp_antialias = R_RegisterShader("fte_ppantialias", 0, 
		"{\n"
			"program fxaa\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
		"}\n"
		);
}



static qboolean VK_R_RenderScene_Cubemap(struct vk_rendertarg *fb)
{
	int cmapsize = 512;
	int i;
	static vec3_t ang[6] =
				{	{0, -90, 0}, {0, 90, 0},
					{90, 0, 0}, {-90, 0, 0},
					{0, 0, 0}, {0, -180, 0}	};
	vec3_t saveang;
	vec3_t saveorg;

	vrect_t vrect;
	pxrect_t prect;
	extern cvar_t ffov;

	shader_t *shader;
	int facemask;
	extern cvar_t r_projection;
	int osm;
	struct vk_rendertarg_cube *rtc = &vk_rt_cubemap;

	if (!*ffov.string || !strcmp(ffov.string, "0"))
	{
		if (ffov.vec4[0] != scr_fov.value)
		{
			ffov.value = ffov.vec4[0] = scr_fov.value;
			Shader_NeedReload(false);	//gah!
		}
	}

	facemask = 0;
	switch(r_projection.ival)
	{
	default:	//invalid.
		return false;
	case PROJ_STEREOGRAPHIC:
		shader = R_RegisterShader("postproc_stereographic", SUF_NONE,
				"{\n"
					"program postproc_stereographic\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		facemask |= 1<<4; /*front view*/
		if (ffov.value > 70)
		{
			facemask |= (1<<0) | (1<<1); /*side/top*/
			if (ffov.value > 85)
				facemask |= (1<<2) | (1<<3); /*bottom views*/
			if (ffov.value > 300)
				facemask |= 1<<5; /*back view*/
		}
		break;
	case PROJ_FISHEYE:
		shader = R_RegisterShader("postproc_fisheye", SUF_NONE,
				"{\n"
					"program postproc_fisheye\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		//fisheye view sees up to a full sphere
		facemask |= 1<<4; /*front view*/
		if (ffov.value > 77)
			facemask |= (1<<0) | (1<<1) | (1<<2) | (1<<3); /*side/top/bottom views*/
		if (ffov.value > 270)
			facemask |= 1<<5; /*back view*/
		break;
	case PROJ_PANORAMA:
		shader = R_RegisterShader("postproc_panorama", SUF_NONE,
				"{\n"
					"program postproc_panorama\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		//panoramic view needs at most the four sides
		facemask |= 1<<4; /*front view*/
		if (ffov.value > 90)
		{
			facemask |= (1<<0) | (1<<1); /*side views*/
			if (ffov.value > 270)
				facemask |= 1<<5; /*back view*/
		}
		facemask = 0x3f;
		break;
	case PROJ_LAEA:
		shader = R_RegisterShader("postproc_laea", SUF_NONE,
				"{\n"
					"program postproc_laea\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		facemask |= 1<<4; /*front view*/
		if (ffov.value > 90)
		{
			facemask |= (1<<0) | (1<<1) | (1<<2) | (1<<3); /*side/top/bottom views*/
			if (ffov.value > 270)
				facemask |= 1<<5; /*back view*/
		}
		break;

	case PROJ_EQUIRECTANGULAR:
		shader = R_RegisterShader("postproc_equirectangular", SUF_NONE,
				"{\n"
					"program postproc_equirectangular\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		facemask = 0x3f;
#if 0
		facemask |= 1<<4; /*front view*/
		if (ffov.value > 90)
		{
			facemask |= (1<<0) | (1<<1) | (1<<2) | (1<<3); /*side/top/bottom views*/
			if (ffov.value > 270)
				facemask |= 1<<5; /*back view*/
		}
#endif
		break;
	case PROJ_PANINI:
		shader = R_RegisterShader("postproc_panini", SUF_NONE,
				"{\n"
					"program postproc_panini\n"
					"{\n"
						"map $sourcecube\n"
					"}\n"
				"}\n"
				);

		facemask |= 1<<4; /*front view*/
		if (ffov.value > 70)
		{
			facemask |= (1<<0) | (1<<1); /*side/top*/
			if (ffov.value > 85)
				facemask |= (1<<2) | (1<<3); /*bottom views*/
			if (ffov.value > 300)
				facemask |= 1<<5; /*back view*/
		}
		break;
	}

	if (!shader || !shader->prog)
		return false;	//erk. shader failed.

	//FIXME: we should be able to rotate the view

	vrect = r_refdef.vrect;
	prect = r_refdef.pxrect;
//	prect.x = (vrect.x * vid.pixelwidth)/vid.width;
//	prect.width = (vrect.width * vid.pixelwidth)/vid.width;
//	prect.y = (vrect.y * vid.pixelheight)/vid.height;
//	prect.height = (vrect.height * vid.pixelheight)/vid.height;

	if (sh_config.texture_non_power_of_two_pic)
	{
		cmapsize = prect.width > prect.height?prect.width:prect.height;
		if (cmapsize > 4096)//sh_config.texture_maxsize)
			cmapsize = 4096;//sh_config.texture_maxsize;
	}


	r_refdef.flags |= RDF_FISHEYE;
	vid.fbpwidth = vid.fbpheight = cmapsize;

	//FIXME: gl_max_size

	VectorCopy(r_refdef.vieworg, saveorg);
	VectorCopy(r_refdef.viewangles, saveang);
	saveang[2] = 0;

	osm = r_refdef.stereomethod;
	r_refdef.stereomethod = STEREO_OFF;

	VKBE_RT_Gen_Cube(rtc, cmapsize, r_clear.ival?true:false);

	vrect = r_refdef.vrect;	//save off the old vrect

	r_refdef.vrect.width = (cmapsize * vid.fbvwidth) / vid.fbpwidth;
	r_refdef.vrect.height = (cmapsize * vid.fbvheight) / vid.fbpheight;
	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = prect.y;

	ang[0][0] = -saveang[0];
	ang[0][1] = -90;
	ang[0][2] = -saveang[0];

	ang[1][0] = -saveang[0];
	ang[1][1] = 90;
	ang[1][2] = saveang[0];
	ang[5][0] = -saveang[0]*2;

	//in theory, we could use a geometry shader to duplicate the polygons to each face.
	//that would of course require that every bit of glsl had such a geometry shader.
	//it would at least reduce cpu load quite a bit.
	for (i = 0; i < 6; i++)
	{
		if (!(facemask & (1<<i)))
			continue;

		VKBE_RT_Begin(&rtc->face[i]);

		r_refdef.fov_x = 90;
		r_refdef.fov_y = 90;
		r_refdef.viewangles[0] = saveang[0]+ang[i][0];
		r_refdef.viewangles[1] = saveang[1]+ang[i][1];
		r_refdef.viewangles[2] = saveang[2]+ang[i][2];


		VK_SetupViewPortProjection(true, NULL, NULL, NULL);

		/*if (!vk.rendertarg->depthcleared)
		{
			VkClearAttachment clr;
			VkClearRect rect;
			clr.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			clr.clearValue.depthStencil.depth = 1;
			clr.clearValue.depthStencil.stencil = 0;
			clr.colorAttachment = 1;
			rect.rect.offset.x = r_refdef.pxrect.x;
			rect.rect.offset.y = r_refdef.pxrect.y;
			rect.rect.extent.width = r_refdef.pxrect.width;
			rect.rect.extent.height = r_refdef.pxrect.height;
			rect.layerCount = 1;
			rect.baseArrayLayer = 0;
			vkCmdClearAttachments(vk.frame->cbuf, 1, &clr, 1, &rect);
			vk.rendertarg->depthcleared = true;
		}*/

		VKBE_SelectEntity(&r_worldentity);

		R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
		RQ_BeginFrame();
//		if (!(r_refdef.flags & RDF_NOWORLDMODEL))
//		{
//			if (cl.worldmodel)
//				P_DrawParticles ();
//		}
		Surf_DrawWorld();
		RQ_RenderBatchClear();

		vk.rendertarg->depthcleared = false;

		if (R2D_Flush)
			Con_Printf("no flush\n");

		VKBE_RT_End(&rtc->face[i]);
	}

	r_refdef.vrect = vrect;
	r_refdef.pxrect = prect;
	VectorCopy(saveorg, r_refdef.vieworg);
	r_refdef.stereomethod = osm;

	VKBE_RT_Begin(fb);

	r_refdef.flipcull = 0;
	VK_Set2D();

	shader->defaulttextures->reflectcube = &rtc->q_colour;

	// draw it through the shader
	if (r_projection.ival == PROJ_EQUIRECTANGULAR)
	{
		//note vr screenshots have requirements here
		R2D_Image(vrect.x, vrect.y, vrect.width, vrect.height, 0, 1, 1, 0, shader);
	}
	else if (r_projection.ival == PROJ_PANORAMA)
	{
		float saspect = .5;
		float taspect = vrect.height / vrect.width * ffov.value / 90;//(0.5 * vrect.width) / vrect.height;
		R2D_Image(vrect.x, vrect.y, vrect.width, vrect.height, -saspect, taspect, saspect, -taspect, shader);
	}
	else if (vrect.width > vrect.height)
	{
		float aspect = (0.5 * vrect.height) / vrect.width;
		R2D_Image(vrect.x, vrect.y, vrect.width, vrect.height, -0.5, aspect, 0.5, -aspect, shader);
	}
	else
	{
		float aspect = (0.5 * vrect.width) / vrect.height;
		R2D_Image(vrect.x, vrect.y, vrect.width, vrect.height, -aspect, 0.5, aspect, -0.5, shader);
	}

	if (R2D_Flush)
		R2D_Flush();

	return true;
}

void VK_R_RenderEye(texid_t image, const pxrect_t *viewport, const vec4_t fovoverride, const float projmatrix[16], const float eyematrix[12])
{
	struct vk_rendertarg *rt;

	VK_SetupViewPortProjection(false, eyematrix, fovoverride, projmatrix);

	rt = &postproc[postproc_buf++%countof(postproc)];
	rt->rpassflags |= RP_VR;
	VKBE_RT_Gen(rt, image?image->vkimage:NULL, 320, 200, false, RT_IMAGEFLAGS);
	VKBE_RT_Begin(rt);


	if (!vk.rendertarg->depthcleared)
	{
		VkClearAttachment clr;
		VkClearRect rect;
		clr.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		clr.clearValue.depthStencil.depth = 1;
		clr.clearValue.depthStencil.stencil = 0;
		clr.colorAttachment = 1;
		rect.rect.offset.x = r_refdef.pxrect.x;
		rect.rect.offset.y = r_refdef.pxrect.y;
		rect.rect.extent.width = r_refdef.pxrect.width;
		rect.rect.extent.height = r_refdef.pxrect.height;
		rect.layerCount = 1;
		rect.baseArrayLayer = 0;
		vkCmdClearAttachments(vk.rendertarg->cbuf, 1, &clr, 1, &rect);
		vk.rendertarg->depthcleared = true;
	}

	VKBE_SelectEntity(&r_worldentity);

	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
	RQ_BeginFrame();
//	if (!(r_refdef.flags & RDF_NOWORLDMODEL))
//	{
//		if (cl.worldmodel)
//			P_DrawParticles ();
//	}
	Surf_DrawWorld();
	RQ_RenderBatchClear();

	vk.rendertarg->depthcleared = false;

	VKBE_RT_End(rt);
	rt->rpassflags &= ~RP_VR;
}

void	VK_R_RenderView				(void)
{
	extern unsigned int r_viewcontents;
	struct vk_rendertarg *rt, *rtscreen = vk.rendertarg;
	extern cvar_t r_fxaa;
	extern	cvar_t r_renderscale, r_postprocshader;
	float renderscale = r_renderscale.value;
	shader_t *custompostproc;

	if (r_norefresh.value || !vid.fbpwidth || !vid.fbpwidth)
	{
		VK_Set2D ();
		return;
	}

	VKBE_Set2D(false);

	Surf_SetupFrame();

	if (vid.vr && vid.vr->Render(VK_R_RenderEye))
	{
		VK_Set2D ();
		return;
	}

	//check if we can do underwater warp
	if (cls.protocol != CP_QUAKE2)	//quake2 tells us directly
	{
		if (r_viewcontents & FTECONTENTS_FLUID)
			r_refdef.flags |= RDF_UNDERWATER;
		else
			r_refdef.flags &= ~RDF_UNDERWATER;
	}
	if (r_refdef.flags & RDF_UNDERWATER)
	{
		extern cvar_t r_projection;
		if (!r_waterwarp.value || r_projection.ival)
			r_refdef.flags &= ~RDF_UNDERWATER;	//no warp at all
		else if (r_waterwarp.value > 0)
			r_refdef.flags |= RDF_WATERWARP;	//try fullscreen warp instead if we can
	}

	if (!r_refdef.globalfog.density)
	{
		extern cvar_t r_fog_linear;

		int fogtype = ((r_refdef.flags & RDF_UNDERWATER) && cl.fog[FOGTYPE_WATER].density)?FOGTYPE_WATER:FOGTYPE_AIR;
		CL_BlendFog(&r_refdef.globalfog, &cl.oldfog[fogtype], realtime, &cl.fog[fogtype]);
		if (!r_fog_linear.ival)
			r_refdef.globalfog.density /= 64;	//FIXME
	}

	custompostproc = NULL;
	if (r_refdef.flags & RDF_NOWORLDMODEL)
		renderscale = 1;	//with no worldmodel, this is probably meant to be transparent so make sure that there's no post-proc stuff messing up transparencies.
	else
	{
		if (*r_postprocshader.string)
		{
			custompostproc = R_RegisterCustom(NULL, r_postprocshader.string, SUF_NONE, NULL, NULL);
			if (custompostproc)
				r_refdef.flags |= RDF_CUSTOMPOSTPROC;
		}

		if (r_fxaa.ival) //overlays will have problems.
			r_refdef.flags |= RDF_ANTIALIAS;

		if (R_CanBloom())
			r_refdef.flags |= RDF_BLOOM;

		if (vid_hardwaregamma.ival == 4 && (v_gamma.value!=1||v_contrast.value!=1||v_contrastboost.value!=1||v_brightness.value!=0))
			r_refdef.flags |= RDF_SCENEGAMMA;
	}

//	if (vk.multisamplebits != VK_SAMPLE_COUNT_1_BIT)	//these are unsupported right now.
//		r_refdef.flags &= ~(RDF_CUSTOMPOSTPROC|RDF_ANTIALIAS|RDF_BLOOM);

	//
	// figure out the viewport
	//
	{
		int x = r_refdef.vrect.x * vid.pixelwidth/(int)vid.width;
		int x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * vid.pixelwidth/(int)vid.width;
		int y = (r_refdef.vrect.y) * vid.pixelheight/(int)vid.height;
		int y2 = ((int)(r_refdef.vrect.y + r_refdef.vrect.height)) * vid.pixelheight/(int)vid.height;

		// fudge around because of frac screen scale
		if (x > 0)
			x--;
		if (x2 < vid.pixelwidth)
			x2++;
		if (y < 0)
			y--;
		if (y2 < vid.pixelheight)
			y2++;

		r_refdef.pxrect.x = x;
		r_refdef.pxrect.y = y;
		r_refdef.pxrect.width = x2 - x;
		r_refdef.pxrect.height = y2 - y;
		r_refdef.pxrect.maxheight = vid.pixelheight;
	}

	if (renderscale != 1.0 || vk.multisamplebits != VK_SAMPLE_COUNT_1_BIT)
	{
		r_refdef.flags |= RDF_RENDERSCALE;
		if (renderscale < 0)
			renderscale *= -1;
		r_refdef.pxrect.width *= renderscale;
		r_refdef.pxrect.height *= renderscale;
		r_refdef.pxrect.maxheight = r_refdef.pxrect.height;
	}

	if (r_refdef.pxrect.width <= 0 || r_refdef.pxrect.height <= 0)
		return;	//you're not allowed to do that, dude.

	//FIXME: VF_RT_*
	//FIXME: if we're meant to be using msaa, render the scene to an msaa target and then resolve.

	postproc_buf = 0;
	if (r_refdef.flags & (RDF_ALLPOSTPROC|RDF_RENDERSCALE|RDF_SCENEGAMMA))
	{
		r_refdef.pxrect.x = 0;
		r_refdef.pxrect.y = 0;
		rt = &postproc[postproc_buf++%countof(postproc)];
		rt->rpassflags = 0;
		if (vk.multisamplebits!=VK_SAMPLE_COUNT_1_BIT)
			rt->rpassflags |= RP_MULTISAMPLE;
		if (r_refdef.flags&RDF_SCENEGAMMA)	//if we're doing scenegamma here, use an fp16 target for extra precision
			rt->rpassflags |= RP_FP16;
		VKBE_RT_Gen(rt, NULL, r_refdef.pxrect.width, r_refdef.pxrect.height, false, (r_renderscale.value < 0)?RT_IMAGEFLAGS-IF_LINEAR+IF_NEAREST:RT_IMAGEFLAGS);
	}
	else
		rt = rtscreen;

	if (!(r_refdef.flags & RDF_NOWORLDMODEL) && VK_R_RenderScene_Cubemap(rt))
	{
	}
	else
	{
		VK_SetupViewPortProjection(false, NULL, NULL, NULL);

		if (rt != rtscreen)
			VKBE_RT_Begin(rt);
		else
		{
			VkViewport vp[1];
			VkRect2D scissor[1];
			vp[0].x = r_refdef.pxrect.x;
			vp[0].y = r_refdef.pxrect.y;
			vp[0].width = r_refdef.pxrect.width;
			vp[0].height = r_refdef.pxrect.height;
			vp[0].minDepth = 0.0;
			vp[0].maxDepth = 1.0;
			scissor[0].offset.x = r_refdef.pxrect.x;
			scissor[0].offset.y = r_refdef.pxrect.y;
			scissor[0].extent.width = r_refdef.pxrect.width;
			scissor[0].extent.height = r_refdef.pxrect.height;
			vkCmdSetViewport(vk.rendertarg->cbuf, 0, countof(vp), vp);
			vkCmdSetScissor(vk.rendertarg->cbuf, 0, countof(scissor), scissor);
		}

		if (!vk.rendertarg->depthcleared)
		{
			VkClearAttachment clr;
			VkClearRect rect;
			clr.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			clr.clearValue.depthStencil.depth = 1;
			clr.clearValue.depthStencil.stencil = 0;
			clr.colorAttachment = 1;
			rect.rect.offset.x = r_refdef.pxrect.x;
			rect.rect.offset.y = r_refdef.pxrect.y;
			rect.rect.extent.width = r_refdef.pxrect.width;
			rect.rect.extent.height = r_refdef.pxrect.height;
			rect.layerCount = 1;
			rect.baseArrayLayer = 0;
			vkCmdClearAttachments(vk.rendertarg->cbuf, 1, &clr, 1, &rect);
			vk.rendertarg->depthcleared = true;
		}

		VKBE_SelectEntity(&r_worldentity);

		R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);
		RQ_BeginFrame();
//		if (!(r_refdef.flags & RDF_NOWORLDMODEL))
//		{
//			if (cl.worldmodel)
//				P_DrawParticles ();
//		}
		Surf_DrawWorld();
		RQ_RenderBatchClear();

		vk.rendertarg->depthcleared = false;

		VK_Set2D ();

		if (rt != rtscreen)
			VKBE_RT_End(rt);
	}

	if (r_refdef.flags & RDF_ALLPOSTPROC)
	{
		if (!vk.scenepp_waterwarp)
			VK_Init_PostProc();
		//FIXME: chain renderpasses as required.

		if (r_refdef.flags & RDF_SCENEGAMMA)
		{
			shader_t *s = R_RegisterShader("fte_scenegamma", 0,
				"{\n"
					"program defaultgammacb\n"
					"affine\n"
					"{\n"
						"map $sourcecolour\n"
						"nodepthtest\n"
					"}\n"
				"}\n"
				);

			r_refdef.flags &= ~RDF_SCENEGAMMA;
			vk.sourcecolour = &rt->q_colour;
			if (r_refdef.flags & RDF_ALLPOSTPROC)
			{
				rt = &postproc[postproc_buf++];
				rt->rpassflags = 0;
				VKBE_RT_Gen(rt, NULL, 320, 200, false, RT_IMAGEFLAGS);
			}
			else
				rt = rtscreen;
			if (rt != rtscreen)
				VKBE_RT_Begin(rt);
			R2D_ImageColours (v_gammainverted.ival?v_gamma.value:(1/v_gamma.value), v_contrast.value, v_brightness.value, v_contrastboost.value);
			R2D_Image(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, 0, 0, 1, 1, s);
			R2D_ImageColours (1, 1, 1, 1);
			R2D_Flush();
			if (rt != rtscreen)
				VKBE_RT_End(rt);
		}

		if (r_refdef.flags & RDF_WATERWARP)
		{
			r_refdef.flags &= ~RDF_WATERWARP;
			vk.sourcecolour = &rt->q_colour;
			if (r_refdef.flags & RDF_ALLPOSTPROC)
			{
				rt = &postproc[postproc_buf++];
				rt->rpassflags = 0;
				VKBE_RT_Gen(rt, NULL, 320, 200, false, RT_IMAGEFLAGS);
			}
			else
				rt = rtscreen;
			if (rt != rtscreen)
				VKBE_RT_Begin(rt);
			R2D_Image(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, 0, 0, 1, 1, vk.scenepp_waterwarp);
			R2D_Flush();
			if (rt != rtscreen)
				VKBE_RT_End(rt);
		}
		if (r_refdef.flags & RDF_CUSTOMPOSTPROC)
		{
			r_refdef.flags &= ~RDF_CUSTOMPOSTPROC;
			vk.sourcecolour = &rt->q_colour;
			if (r_refdef.flags & RDF_ALLPOSTPROC)
			{
				rt = &postproc[postproc_buf++];
				rt->rpassflags = 0;
				VKBE_RT_Gen(rt, NULL, 320, 200, false, RT_IMAGEFLAGS);
			}
			else
				rt = rtscreen;
			if (rt != rtscreen)
				VKBE_RT_Begin(rt);
			R2D_Image(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, 0, 1, 1, 0, custompostproc);
			R2D_Flush();
			if (rt != rtscreen)
				VKBE_RT_End(rt);
		}
		if (r_refdef.flags & RDF_ANTIALIAS)
		{
			r_refdef.flags &= ~RDF_ANTIALIAS;
			R2D_ImageColours(rt->width, rt->height, 1, 1);
			vk.sourcecolour = &rt->q_colour;
			if (r_refdef.flags & RDF_ALLPOSTPROC)
			{
				rt = &postproc[postproc_buf++];
				rt->rpassflags = 0;
				VKBE_RT_Gen(rt, NULL, 320, 200, false, RT_IMAGEFLAGS);
			}
			else
				rt = rtscreen;
			if (rt != rtscreen)
				VKBE_RT_Begin(rt);
			R2D_Image(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, 0, 1, 1, 0, vk.scenepp_antialias);
			R2D_ImageColours(1, 1, 1, 1);
			R2D_Flush();
			if (rt != rtscreen)
				VKBE_RT_End(rt);
		}
		if (r_refdef.flags & RDF_BLOOM)
		{
			VK_R_BloomBlend(&rt->q_colour, r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height);
			rt = rtscreen;
		}
	}
	else if (r_refdef.flags & RDF_RENDERSCALE)
	{
		if (!vk.scenepp_rescale)
			vk.scenepp_rescale = R_RegisterShader("fte_rescaler", 0, 
				"{\n"
					"program default2d\n"
					"{\n"
						"map $sourcecolour\n"
					"}\n"
				"}\n"
				);
		vk.sourcecolour = &rt->q_colour;
		rt = rtscreen;
		R2D_Image(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, 0, 0, 1, 1, vk.scenepp_rescale);
		R2D_Flush();
	}
	vk.sourcecolour = r_nulltex;
}


typedef struct
{
	uint32_t imageformat;
	uint32_t imagestride;
	uint32_t imagewidth;
	uint32_t imageheight;
	VkBuffer buffer;
	size_t memsize;
	VkDeviceMemory memory;
	void (*gotrgbdata) (void *rgbdata, intptr_t bytestride, size_t width, size_t height, enum uploadfmt fmt);
} vkscreencapture_t;

static void VKVID_CopiedRGBData (void*ctx)
{	//some fence got hit, we did our copy, data is now cpu-visible, cache-willing.
	vkscreencapture_t *capt = ctx;
	void *imgdata;
	VkAssert(vkMapMemory(vk.device, capt->memory, 0, capt->memsize, 0, &imgdata));
	capt->gotrgbdata(imgdata, capt->imagestride, capt->imagewidth, capt->imageheight, capt->imageformat);
	vkUnmapMemory(vk.device, capt->memory);
	vkDestroyBuffer(vk.device, capt->buffer, vkallocationcb);
	vkFreeMemory(vk.device, capt->memory, vkallocationcb);
}
void VKVID_QueueGetRGBData			(void (*gotrgbdata) (void *rgbdata, intptr_t bytestride, size_t width, size_t height, enum uploadfmt fmt))
{
	//should be half way through rendering
	vkscreencapture_t *capt;

	VkBufferImageCopy icpy;

	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	VkBufferCreateInfo bci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};


	if (!VK_SCR_GrabBackBuffer())
		return;

	if (!vk.frame->backbuf->colour.width || !vk.frame->backbuf->colour.height)
		return; //erm, some kind of error?

	capt = VK_AtFrameEnd(VKVID_CopiedRGBData, NULL, sizeof(*capt));
	capt->gotrgbdata = gotrgbdata;

	//FIXME: vkCmdBlitImage the image to convert it from half-float or whatever to a format that our screenshot etc code can cope with.
	capt->imageformat = TF_BGRA32;
	capt->imagestride = vk.frame->backbuf->colour.width*4;	//vulkan is top-down, so this should be positive.
	capt->imagewidth = vk.frame->backbuf->colour.width;
	capt->imageheight = vk.frame->backbuf->colour.height;

	bci.flags = 0;
	bci.size = capt->memsize = capt->imagewidth*capt->imageheight*4;
	bci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bci.queueFamilyIndexCount = 0;
	bci.pQueueFamilyIndices = NULL;

	VkAssert(vkCreateBuffer(vk.device, &bci, vkallocationcb, &capt->buffer));
	vkGetBufferMemoryRequirements(vk.device, capt->buffer, &mem_reqs);
	memAllocInfo.allocationSize = mem_reqs.size;
	memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	if (memAllocInfo.memoryTypeIndex == ~0u)
		memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &capt->memory));
	DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)capt->memory, "VKVID_QueueGetRGBData");
	VkAssert(vkBindBufferMemory(vk.device, capt->buffer, capt->memory, 0));

	set_image_layout(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	icpy.bufferOffset = 0;
	icpy.bufferRowLength = 0;	//packed
	icpy.bufferImageHeight = 0;	//packed
	icpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	icpy.imageSubresource.mipLevel = 0;
	icpy.imageSubresource.baseArrayLayer = 0;
	icpy.imageSubresource.layerCount = 1;
	icpy.imageOffset.x = 0;
	icpy.imageOffset.y = 0;
	icpy.imageOffset.z = 0;
	icpy.imageExtent.width = capt->imagewidth;
	icpy.imageExtent.height = capt->imageheight;
	icpy.imageExtent.depth = 1;

	vkCmdCopyImageToBuffer(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, capt->buffer, 1, &icpy);

	set_image_layout(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}

char	*VKVID_GetRGBInfo			(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	//in order to deal with various backbuffer formats (like half-float) etc, we play safe and blit the framebuffer to a safe format.
	//we then transfer that into a buffer that we can then directly read.
	//and then we allocate a C buffer that we then copy it into...
	//so yeah, 3 copies. life sucks.
	//blit requires support for VK_IMAGE_USAGE_TRANSFER_DST_BIT on our image, which means we need optimal, which means we can't directly map it, which means we need the buffer copy too.
	//this might be relaxed on mobile, but who really takes screenshots on mobiles anyway?!? anyway, video capture shouldn't be using this either way so top performance isn't a concern
	if (VK_SCR_GrabBackBuffer())
	{
		VkImageLayout framebufferlayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//vk.frame->backbuf->colour.layout;

		void *imgdata, *outdata;
		struct vk_fencework *fence = VK_FencedBegin(NULL, 0);
		VkImage tempimage;
		VkDeviceMemory tempmemory;
		VkBufferCreateInfo bci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		VkBuffer tempbuffer;
		VkDeviceMemory tempbufmemory;
		VkMemoryRequirements mem_reqs;
		VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		VkImageCreateInfo ici = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		//VkFormatProperties vkfmt;

		ici.flags = 0;
		ici.imageType = VK_IMAGE_TYPE_2D;
		/*vkGetPhysicalDeviceFormatProperties(vk.gpu, VK_FORMAT_B8G8R8_UNORM, &vkfmt);
		if ((vkfmt.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) && (vkfmt.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR))
		{	//if we can do BGR, then use it, because that's what most PC file formats use, like tga.
			//we don't really want alpha data anyway.
			if (vid.flags & VID_SRGB_FB)
				ici.format = VK_FORMAT_B8G8R8_SRGB;
			else
				ici.format = VK_FORMAT_B8G8R8_UNORM;
		}
		else*/
		{	//otherwise lets just get bgra data.
			if (vid.flags & VID_SRGB_FB)
				ici.format = VK_FORMAT_B8G8R8A8_SRGB;
			else
				ici.format = VK_FORMAT_B8G8R8A8_UNORM;
		}
		ici.extent.width = vid.pixelwidth;
		ici.extent.height = vid.pixelheight;
		ici.extent.depth = 1;
		ici.mipLevels = 1;
		ici.arrayLayers = 1;
		ici.samples = VK_SAMPLE_COUNT_1_BIT;
		ici.tiling = VK_IMAGE_TILING_OPTIMAL;
		ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ici.queueFamilyIndexCount = 0;
		ici.pQueueFamilyIndices = NULL;
		ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkAssert(vkCreateImage(vk.device, &ici, vkallocationcb, &tempimage));
		DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)tempimage, "VKVID_GetRGBInfo staging");
		vkGetImageMemoryRequirements(vk.device, tempimage, &mem_reqs);
		memAllocInfo.allocationSize = mem_reqs.size;
		memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, 0);
		VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &tempmemory));
		DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)tempmemory, "VKVID_GetRGBInfo staging");
		VkAssert(vkBindImageMemory(vk.device, tempimage, tempmemory, 0));

		bci.flags = 0;
		bci.size = vid.pixelwidth*vid.pixelheight*4;
		bci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bci.queueFamilyIndexCount = 0;
		bci.pQueueFamilyIndices = NULL;

		VkAssert(vkCreateBuffer(vk.device, &bci, vkallocationcb, &tempbuffer));
		DebugSetName(VK_OBJECT_TYPE_BUFFER, (uint64_t)tempbuffer, "VKVID_GetRGBInfo buffer");
		vkGetBufferMemoryRequirements(vk.device, tempbuffer, &mem_reqs);
		memAllocInfo.allocationSize = mem_reqs.size;
		memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		if (memAllocInfo.memoryTypeIndex == ~0u)
			memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &tempbufmemory));
		DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)tempbufmemory, "VKVID_GetRGBInfo buffer");
		VkAssert(vkBindBufferMemory(vk.device, tempbuffer, tempbufmemory, 0));


		set_image_layout(fence->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
				framebufferlayout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		set_image_layout(fence->cbuf, tempimage, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		{
			VkImageBlit iblt;
			iblt.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			iblt.srcSubresource.mipLevel = 0;
			iblt.srcSubresource.baseArrayLayer = 0;
			iblt.srcSubresource.layerCount = 1;
			iblt.srcOffsets[0].x = 0;
			iblt.srcOffsets[0].y = 0;
			iblt.srcOffsets[0].z = 0;
			iblt.srcOffsets[1].x = vid.pixelwidth;
			iblt.srcOffsets[1].y = vid.pixelheight;
			iblt.srcOffsets[1].z = 1;
			iblt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			iblt.dstSubresource.mipLevel = 0;
			iblt.dstSubresource.baseArrayLayer = 0;
			iblt.dstSubresource.layerCount = 1;
			iblt.dstOffsets[0].x = 0;
			iblt.dstOffsets[0].y = 0;
			iblt.dstOffsets[0].z = 0;
			iblt.dstOffsets[1].x = vid.pixelwidth;
			iblt.dstOffsets[1].y = vid.pixelheight;
			iblt.dstOffsets[1].z = 1;

			vkCmdBlitImage(fence->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, tempimage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &iblt, VK_FILTER_LINEAR);
		}
		set_image_layout(fence->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				framebufferlayout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		set_image_layout(fence->cbuf, tempimage, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		{
			VkBufferImageCopy icpy;
			icpy.bufferOffset = 0;
			icpy.bufferRowLength = 0;	//packed
			icpy.bufferImageHeight = 0;	//packed
			icpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			icpy.imageSubresource.mipLevel = 0;
			icpy.imageSubresource.baseArrayLayer = 0;
			icpy.imageSubresource.layerCount = 1;
			icpy.imageOffset.x = 0;
			icpy.imageOffset.y = 0;
			icpy.imageOffset.z = 0;
			icpy.imageExtent.width = ici.extent.width;
			icpy.imageExtent.height = ici.extent.height;
			icpy.imageExtent.depth = 1;

			vkCmdCopyImageToBuffer(fence->cbuf, tempimage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tempbuffer, 1, &icpy);
		}

		VK_FencedSync(fence);

		outdata = BZ_Malloc(4*ici.extent.width*ici.extent.height);
		if (ici.format == VK_FORMAT_B8G8R8_SRGB || ici.format == VK_FORMAT_B8G8R8_UNORM)
			*fmt = PTI_BGR8;
		else if (ici.format == VK_FORMAT_R8G8B8_SRGB || ici.format == VK_FORMAT_R8G8B8_UNORM)
			*fmt = PTI_RGB8;
		else if (ici.format == VK_FORMAT_R8G8B8A8_SRGB || ici.format == VK_FORMAT_R8G8B8A8_UNORM)
			*fmt = PTI_RGBA8;
		else
			*fmt = PTI_BGRA8;
		*bytestride = ici.extent.width*4;
		*truevidwidth = ici.extent.width;
		*truevidheight = ici.extent.height;

		VkAssert(vkMapMemory(vk.device, tempbufmemory, 0, 4*ici.extent.width*ici.extent.height, 0, &imgdata));
		memcpy(outdata, imgdata, 4*ici.extent.width*ici.extent.height);
		vkUnmapMemory(vk.device, tempbufmemory);

		vkDestroyImage(vk.device, tempimage, vkallocationcb);
		vkFreeMemory(vk.device, tempmemory, vkallocationcb);

		vkDestroyBuffer(vk.device, tempbuffer, vkallocationcb);
		vkFreeMemory(vk.device, tempbufmemory, vkallocationcb);

		return outdata;
	}
	return NULL;
}

static void VK_PaintScreen(void)
{
	qboolean nohud;
	qboolean noworld;

	
	vid.fbvwidth = vid.width;
	vid.fbvheight = vid.height;
	vid.fbpwidth = vid.pixelwidth;
	vid.fbpheight = vid.pixelheight;

	r_refdef.pxrect.x = 0;
	r_refdef.pxrect.y = 0;
	r_refdef.pxrect.width = vid.fbpwidth;
	r_refdef.pxrect.height = vid.fbpheight;
	r_refdef.pxrect.maxheight = vid.pixelheight;

	vid.numpages = vk.backbuf_count + 1;

	R2D_Font_Changed();

	VK_Set2D ();

	Shader_DoReload();

	if (scr_disabled_for_loading)
	{
		extern float scr_disabled_time;
		if (Sys_DoubleTime() - scr_disabled_time > 60 || !Key_Dest_Has(~kdm_game))
		{
			//FIXME: instead of reenabling the screen, we should just draw the relevent things skipping only the game.
			scr_disabled_for_loading = false;
		}
		else
		{
//			scr_drawloading = true;
			SCR_DrawLoading (true);
//			scr_drawloading = false;
			return;
		}
	}

/*	if (!scr_initialized || !con_initialized)
	{
		RSpeedEnd(RSPEED_TOTALREFRESH);
		return;                         // not initialized yet
	}
*/

#ifdef TEXTEDITOR
	if (editormodal)
	{
		Editor_Draw();
		V_UpdatePalette (false);
#if defined(_WIN32) && defined(GLQUAKE)
		Media_RecordFrame();
#endif
		R2D_BrightenScreen();

		if (key_dest_mask & kdm_console)
			Con_DrawConsole(vid.height/2, false);
		else
			Con_DrawConsole(0, false);
//		SCR_DrawCursor();
		return;
	}
#endif

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	noworld = false;
	nohud = false;

	if (topmenu && topmenu->isopaque)
		nohud = true;
#ifdef VM_CG
	else if (q3 && q3->cg.Redraw(cl.time))
		nohud = true;
#endif
#ifdef CSQC_DAT
	else if (CSQC_DrawView())
		nohud = true;
#endif
	else
	{
		if (r_worldentity.model && cls.state == ca_active)
			V_RenderView (nohud);
		else
		{
			noworld = true;
		}
	}

	scr_con_forcedraw = false;
	if (noworld)
	{
		//draw the levelshot or the conback fullscreen
		if (R2D_DrawLevelshot())
			;
		else if (scr_con_current != vid.height)
		{
#ifdef HAVE_LEGACY
			extern cvar_t dpcompat_console;
			if (dpcompat_console.ival)
			{
				R2D_ImageColours(0,0,0,1);
				R2D_FillBlock(0, 0, vid.width, vid.height);
				R2D_ImageColours(1,1,1,1);
			}
			else
#endif
				R2D_ConsoleBackground(0, vid.height, true);
		}
		else
			scr_con_forcedraw = true;

		nohud = true;
	}

	r_refdef.playerview = &cl.playerview[0];
	if (!vrui.enabled)
		SCR_DrawTwoDimensional(nohud);

	V_UpdatePalette (false);
	R2D_BrightenScreen();

#if defined(_WIN32) && defined(GLQUAKE)
	Media_RecordFrame();
#endif

	RSpeedShow();
}

VkCommandBuffer VK_AllocFrameCBuf(void)
{
	struct vkframe *frame = vk.frame;
	if (frame->numcbufs == frame->maxcbufs)
	{
		VkCommandBufferAllocateInfo cbai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

		frame->maxcbufs++;
		frame->cbufs = BZ_Realloc(frame->cbufs, sizeof(*frame->cbufs)*frame->maxcbufs);

		cbai.commandPool = vk.cmdpool;
		cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = frame->maxcbufs - frame->numcbufs;
		VkAssert(vkAllocateCommandBuffers(vk.device, &cbai, frame->cbufs+frame->numcbufs));
		DebugSetName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)frame->cbufs[frame->numcbufs], "VK_AllocFrameCBuf");
	}
	return frame->cbufs[frame->numcbufs++];
}

qboolean VK_SCR_GrabBackBuffer(void)
{
	VkSemaphore sem;
	RSpeedLocals();

	if (vk.frame)	//erk, we already have one...
		return true;


	RSpeedRemark();

	VK_FencedCheck();

	if (!vk.unusedframes)
	{
		struct vkframe *newframe = Z_Malloc(sizeof(*vk.frame));
		VKBE_InitFramePools(newframe);
		newframe->next = vk.unusedframes;
		vk.unusedframes = newframe;
	}

	while (vk.acquirenext == vk.acquirelast)
	{	//we're still waiting for the render thread to increment acquirelast.
		//shouldn't really happen, but can if the gpu is slow.
		if (vk.neednewswapchain)
		{	//the render thread is is likely to have died... don't loop until infinity.
#ifdef MULTITHREAD
			if (vk.submitthread)
			{
				//signal its condition, in case its sleeping, so we don't wait for infinity
				Sys_LockConditional(vk.submitcondition);
				Sys_ConditionSignal(vk.submitcondition);
				Sys_UnlockConditional(vk.submitcondition);

				//now wait+clean up the thread
				Sys_WaitOnThread(vk.submitthread);
				vk.submitthread = NULL;
			}
#endif
			return false;
		}
		Sys_Sleep(0);	//o.O
#ifdef _WIN32
		Sys_SendKeyEvents();
#endif
	}

	if (vk.acquirefences[vk.acquirenext%ACQUIRELIMIT] != VK_NULL_HANDLE)
	{
		//wait for the queued acquire to actually finish
		if (vk_busywait.ival)
		{	//busy wait, to try to get the highest fps possible
			for (;;)
			{
				switch(vkGetFenceStatus(vk.device, vk.acquirefences[vk.acquirenext%ACQUIRELIMIT]))
				{
				case VK_SUCCESS:
					break;	//hurrah
				case VK_NOT_READY:
					continue;	//keep going until its actually signaled. submission thread is probably just slow.
				case VK_TIMEOUT:
					continue;	//erk? this isn't a documented result here.
				case VK_ERROR_DEVICE_LOST:
					Sys_Error("Vulkan device lost");
				default:
					return false;
				}
				break;
			}
		}
		else
		{
			//friendly wait
			int failures = 0;
			for(;;)
			{
				VkResult err = vkWaitForFences(vk.device, 1, &vk.acquirefences[vk.acquirenext%ACQUIRELIMIT], VK_FALSE, 1000000000);

				if (err == VK_SUCCESS)
					break;
				else if (err == VK_TIMEOUT)
				{
					if (++failures == 5)
						Sys_Error("waiting for fence for over 5 seconds. Assuming bug.");
					continue;
				}
				else if (err == VK_ERROR_DEVICE_LOST)
					Sys_Error("Vulkan device lost");
				else if (err != VK_ERROR_OUT_OF_HOST_MEMORY && err != VK_ERROR_OUT_OF_DEVICE_MEMORY)
					Sys_Error("vkWaitForFences returned unspecified result: %s", VK_VKErrorToString(err));
				return false;
			}
		}
		VkAssert(vkResetFences(vk.device, 1, &vk.acquirefences[vk.acquirenext%ACQUIRELIMIT]));
	}
	vk.bufferidx = vk.acquirebufferidx[vk.acquirenext%ACQUIRELIMIT];

	sem = vk.acquiresemaphores[vk.acquirenext%ACQUIRELIMIT];
	vk.acquirenext++;

	//grab the first unused
	Sys_LockConditional(vk.submitcondition);
	vk.frame = vk.unusedframes;
	vk.unusedframes = vk.frame->next;
	vk.frame->next = NULL;
	Sys_UnlockConditional(vk.submitcondition);

	VkAssert(vkResetFences(vk.device, 1, &vk.frame->finishedfence));

	vk.frame->backbuf = &vk.backbufs[vk.bufferidx];
	vk.rendertarg = vk.frame->backbuf;

	vk.frame->numcbufs = 0;
	vk.rendertarg->cbuf = VK_AllocFrameCBuf();
	vk.frame->acquiresemaphore = sem;

	RSpeedEnd(RSPEED_SETUP);





	
	{
		VkCommandBufferBeginInfo begininf = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		VkCommandBufferInheritanceInfo inh = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
		begininf.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begininf.pInheritanceInfo = &inh;
		inh.renderPass = VK_NULL_HANDLE;	//unused
		inh.subpass = 0;					//unused
		inh.framebuffer = VK_NULL_HANDLE;	//unused
		inh.occlusionQueryEnable = VK_FALSE;
		inh.queryFlags = 0;
		inh.pipelineStatistics = 0;
		vkBeginCommandBuffer(vk.rendertarg->cbuf, &begininf);
	}

//	VK_DebugFramerate();

//	vkCmdWriteTimestamp(vk.frame->cbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, querypool, vk.bufferidx*2+0);

	if (!(vk.rendertarg->rpassflags & RP_PRESENTABLE))
	{
		VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imgbarrier.pNext = NULL;
		imgbarrier.srcAccessMask = 0;//VK_ACCESS_MEMORY_READ_BIT;
		imgbarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgbarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;//vk.rendertarg->colour.layout;	//'Alternately, oldLayout can be VK_IMAGE_LAYOUT_UNDEFINED, if the image's contents need not be preserved.'
		imgbarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgbarrier.image = vk.frame->backbuf->colour.image;
		imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgbarrier.subresourceRange.baseMipLevel = 0;
		imgbarrier.subresourceRange.levelCount = 1;
		imgbarrier.subresourceRange.baseArrayLayer = 0;
		imgbarrier.subresourceRange.layerCount = 1;
		imgbarrier.srcQueueFamilyIndex = vk.queuefam[1];
		imgbarrier.dstQueueFamilyIndex = vk.queuefam[0];
		if (vk.frame->backbuf->firstuse)
		{
			imgbarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		}
		vk.rendertarg->colour.layout = imgbarrier.newLayout;
		vkCmdPipelineBarrier(vk.rendertarg->cbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
	}
	{
		VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imgbarrier.pNext = NULL;
		imgbarrier.srcAccessMask = 0;
		imgbarrier.dstAccessMask = 0;//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imgbarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgbarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imgbarrier.image = vk.frame->backbuf->depth.image;
		imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgbarrier.subresourceRange.baseMipLevel = 0;
		imgbarrier.subresourceRange.levelCount = 1;
		imgbarrier.subresourceRange.baseArrayLayer = 0;
		imgbarrier.subresourceRange.layerCount = 1;
		imgbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkCmdPipelineBarrier(vk.rendertarg->cbuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
	}
	VKBE_RestartFrame();

	{
		int rp = vk.frame->backbuf->rpassflags;
		VkClearValue	clearvalues[3];
		extern cvar_t r_clear;
		VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

		//attachments are: screen[1], depth[msbits], (screen[msbits])

		clearvalues[0].color.float32[0] = !!(r_clear.ival & 1);
		clearvalues[0].color.float32[1] = !!(r_clear.ival & 2);
		clearvalues[0].color.float32[2] = !!(r_clear.ival & 4);
		clearvalues[0].color.float32[3] = 1;
		clearvalues[1].depthStencil.depth = 1.0;
		clearvalues[1].depthStencil.stencil = 0;

		if (rp & RP_MULTISAMPLE)
		{
			clearvalues[2].color.float32[0] = !!(r_clear.ival & 1);
			clearvalues[2].color.float32[1] = !!(r_clear.ival & 2);
			clearvalues[2].color.float32[2] = !!(r_clear.ival & 4);
			clearvalues[2].color.float32[3] = 1;
			rpbi.clearValueCount = 3;
		}
		else
			rpbi.clearValueCount = 2;

		if (r_clear.ival || vk.frame->backbuf->firstuse)
			rpbi.renderPass = VK_GetRenderPass(RP_FULLCLEAR|rp);
		else
			rpbi.renderPass = VK_GetRenderPass(RP_DEPTHCLEAR|rp);
		rpbi.framebuffer = vk.frame->backbuf->framebuffer;
		rpbi.renderArea.offset.x = 0;
		rpbi.renderArea.offset.y = 0;
		rpbi.renderArea.extent.width = vk.frame->backbuf->colour.width;
		rpbi.renderArea.extent.height = vk.frame->backbuf->colour.height;
		rpbi.pClearValues = clearvalues;
		vkCmdBeginRenderPass(vk.rendertarg->cbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

		vk.frame->backbuf->width = rpbi.renderArea.extent.width;
		vk.frame->backbuf->height = rpbi.renderArea.extent.height;

		rpbi.clearValueCount = 0;
		rpbi.pClearValues = NULL;
		rpbi.renderPass = VK_GetRenderPass(RP_RESUME|rp);
		vk.rendertarg->restartinfo = rpbi;
		vk.rendertarg->depthcleared = true;
	}
	vk.frame->backbuf->firstuse = false;
	return true;
}

struct vk_presented
{
	struct vk_fencework fw;
	struct vkframe *frame;
};
void VK_Presented(void *fw)
{
	struct vk_presented *pres = fw;
	struct vkframe *frame = pres->frame;
	pres->fw.fence = VK_NULL_HANDLE;	//don't allow that to be freed.

	while(frame->frameendjobs)
	{
		struct vk_frameend *job = frame->frameendjobs;
		frame->frameendjobs = job->next;
		job->FrameEnded(job+1);
		Z_Free(job);
	}

	frame->next = vk.unusedframes;
	vk.unusedframes = frame;
}

#if 0
void VK_DebugFramerate(void)
{
	static double lastupdatetime;
	static double lastsystemtime;
	double t;
	extern int fps_count;
	float lastfps;

	float frametime;

	t = Sys_DoubleTime();
	if ((t - lastupdatetime) >= 1.0)
	{
		lastfps = fps_count/(t - lastupdatetime);
		fps_count = 0;
		lastupdatetime = t;

		OutputDebugStringA(va("%g fps\n", lastfps));
	}
	frametime = t - lastsystemtime;
	lastsystemtime = t;
}
#endif

qboolean	VK_SCR_UpdateScreen			(void)
{
	VkImageLayout fblayout;

	VK_FencedCheck();

	//a few cvars need some extra work if they're changed
	if ((vk.allowsubmissionthread && vk_submissionthread.modified) || vid_vsync.modified || vk_waitfence.modified || vid_triplebuffer.modified || vid_srgb.modified || vid_multisample.modified)
		vk.neednewswapchain = true;

	if (vk.devicelost)
	{	//vkQueueSubmit returning vk_error_device_lost means we give up and try resetting everything.
		//if someone's installing new drivers then wait a little time before reloading everything, in the hope that any other dependant files got copied. or something.
		//fixme: don't allow this to be spammed...
		Sys_Sleep(5);
		Con_Printf("Device was lost. Restarting video\n");
		Cmd_ExecuteString("vid_restart", RESTRICT_LOCAL);
		return false;
	}

	if (vk.neednewswapchain && !vk.frame)
	{
#ifdef MULTITHREAD
		//kill the thread
		if (vk.submitthread)
		{
			Sys_LockConditional(vk.submitcondition);	//annoying, but required for it to be reliable with respect to other things.
			Sys_ConditionSignal(vk.submitcondition);
			Sys_UnlockConditional(vk.submitcondition);
			Sys_WaitOnThread(vk.submitthread);
			vk.submitthread = NULL;
		}
#endif
		//make sure any work is actually done BEFORE the swapchain gets destroyed
		while (vk.work)
		{
			Sys_LockConditional(vk.submitcondition);
			VK_Submit_DoWork();
			Sys_UnlockConditional(vk.submitcondition);
		}
		if (vk.dopresent)
			vk.dopresent(NULL);
		vkDeviceWaitIdle(vk.device);
		if (!VK_CreateSwapChain())
			return false;
		vk.neednewswapchain = false;

#ifdef MULTITHREAD
		if (vk.allowsubmissionthread && (vk_submissionthread.ival || !*vk_submissionthread.string))
		{
			vk.submitthread = Sys_CreateThread("vksubmission", VK_Submit_Thread, NULL, THREADP_HIGHEST, 0);
		}
#endif
	}

	if (!VK_SCR_GrabBackBuffer())
		return false;

	VKBE_Set2D(true);
	VKBE_SelectDLight(NULL, vec3_origin, NULL, 0);

	VK_PaintScreen();

	if (R2D_Flush)
		R2D_Flush();

	vkCmdEndRenderPass(vk.rendertarg->cbuf);

	fblayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	/*if (0)
	{
		vkscreencapture_t *capt = VK_AtFrameEnd(atframeend, sizeof(vkscreencapture_t));
		VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		VkBufferImageCopy region;
		imgbarrier.pNext = NULL;
		imgbarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgbarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgbarrier.oldLayout = fblayout;
		imgbarrier.newLayout = fblayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgbarrier.image = vk.frame->backbuf->colour.image;
		imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgbarrier.subresourceRange.baseMipLevel = 0;
		imgbarrier.subresourceRange.levelCount = 1;
		imgbarrier.subresourceRange.baseArrayLayer = 0;
		imgbarrier.subresourceRange.layerCount = 1;
		imgbarrier.srcQueueFamilyIndex = vk.queuefam[0];
		imgbarrier.dstQueueFamilyIndex = vk.queuefam[0];
		vkCmdPipelineBarrier(vk.frame->cbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);

		region.bufferOffset = 0;
		region.bufferRowLength = 0;		//tightly packed
		region.bufferImageHeight = 0;	//tightly packed
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = 0;
		region.imageOffset.y = 0;
		region.imageOffset.z = 0;
		region.imageExtent.width = capt->imagewidth = vk.frame->backbuf->colour.width;
		region.imageExtent.height = capt->imageheight = vk.frame->backbuf->colour.height;
		region.imageExtent.depth = 1;
		vkCmdCopyImageToBuffer(vk.frame->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
	}*/

	if (!(vk.frame->backbuf->rpassflags & RP_PRESENTABLE))
	{
		VkImageMemoryBarrier imgbarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imgbarrier.pNext = NULL;
		imgbarrier.srcAccessMask = /*VK_ACCESS_TRANSFER_READ_BIT|*/VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgbarrier.dstAccessMask = 0;
		imgbarrier.oldLayout = fblayout;
		imgbarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imgbarrier.image = vk.frame->backbuf->colour.image;
		imgbarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgbarrier.subresourceRange.baseMipLevel = 0;
		imgbarrier.subresourceRange.levelCount = 1;
		imgbarrier.subresourceRange.baseArrayLayer = 0;
		imgbarrier.subresourceRange.layerCount = 1;
		imgbarrier.srcQueueFamilyIndex = vk.queuefam[0];
		imgbarrier.dstQueueFamilyIndex = vk.queuefam[1];
		vkCmdPipelineBarrier(vk.rendertarg->cbuf,  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &imgbarrier);
		vk.rendertarg->colour.layout = imgbarrier.newLayout;
	}

//	vkCmdWriteTimestamp(vk.rendertarg->cbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, querypool, vk.bufferidx*2+1);
	vkEndCommandBuffer(vk.rendertarg->cbuf);

	VKBE_FlushDynamicBuffers();

	{
		struct vk_presented *fw = Z_Malloc(sizeof(*fw));
		fw->fw.Passed = VK_Presented;
		fw->fw.fence = vk.frame->finishedfence;
		fw->frame = vk.frame;
		//hand over any post-frame jobs to the frame in question.
		vk.frame->frameendjobs = vk.frameendjobs;
		vk.frameendjobs = NULL;

		VK_Submit_Work(vk.rendertarg->cbuf, vk.frame->acquiresemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, vk.frame->backbuf->presentsemaphore, vk.frame->finishedfence, vk.frame, &fw->fw);
	}

	//now would be a good time to do any compute work or lightmap updates...

	vk.frame = NULL;

	VK_FencedCheck();

	VID_SwapBuffers();

#ifdef TEXTEDITOR
	if (editormodal)
	{	//FIXME
		VK_SCR_GrabBackBuffer();
	}
#endif
	return true;
}

void	VKBE_RenderToTextureUpdate2d(qboolean destchanged)
{
}

static void VK_DestroyRenderPasses(void)
{
	int i;
	for (i = 0; i < countof(vk.renderpass); i++)
	{
		if (vk.renderpass[i] != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(vk.device, vk.renderpass[i], vkallocationcb);
			vk.renderpass[i] = VK_NULL_HANDLE;
		}
	}
}
VkRenderPass VK_GetRenderPass(int pass)
{
	int numattachments;
	static	VkAttachmentReference color_reference;
	static	VkAttachmentReference depth_reference;
	static	VkAttachmentReference resolve_reference;
	static	VkAttachmentDescription attachments[3] = {{0}};
	static	VkSubpassDescription subpass = {0};
	static 	VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

//two render passes are compatible for piplines when they match exactly except for:
//initial and final layouts in attachment descriptions.
//load and store operations in attachment descriptions.
//image layouts in attachment references.

	if (vk.multisamplebits == VK_SAMPLE_COUNT_1_BIT)
		pass &= ~RP_MULTISAMPLE;	//no difference

	if (vk.renderpass[pass] != VK_NULL_HANDLE)
		return vk.renderpass[pass];	//already built

	numattachments = 0;
	if ((pass&3)==RP_DEPTHONLY)
		color_reference.attachment = ~(uint32_t)0;	//no colour buffer...
	else
		color_reference.attachment = numattachments++;
	depth_reference.attachment = numattachments++;
	resolve_reference.attachment = ~(uint32_t)0;
	if ((pass & RP_MULTISAMPLE) && color_reference.attachment != ~(uint32_t)0)
	{	//if we're using multisample, then render to a third texture, with a resolve to the original colour texture.
		resolve_reference.attachment = color_reference.attachment;
		color_reference.attachment = numattachments++;
	}

	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (color_reference.attachment != ~(uint32_t)0)
	{
		if (pass&RP_FP16)
			attachments[color_reference.attachment].format = VK_FORMAT_R16G16B16A16_SFLOAT;
		else if (pass&RP_VR)
			attachments[color_reference.attachment].format = vk.backbufformat;	//FIXME
		else
			attachments[color_reference.attachment].format = vk.backbufformat;
		attachments[color_reference.attachment].samples = (pass & RP_MULTISAMPLE)?vk.multisamplebits:VK_SAMPLE_COUNT_1_BIT;
//		attachments[color_reference.attachment].loadOp = pass?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[color_reference.attachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[color_reference.attachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[color_reference.attachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[color_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[color_reference.attachment].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	if (depth_reference.attachment != ~(uint32_t)0)
	{
		attachments[depth_reference.attachment].format = vk.depthformat;
		attachments[depth_reference.attachment].samples = (pass & RP_MULTISAMPLE)?vk.multisamplebits:VK_SAMPLE_COUNT_1_BIT;
//		attachments[depth_reference.attachment].loadOp = pass?VK_ATTACHMENT_LOAD_OP_LOAD:VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[depth_reference.attachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;//VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[depth_reference.attachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[depth_reference.attachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[depth_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[depth_reference.attachment].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	if (resolve_reference.attachment != ~(uint32_t)0)
	{
		attachments[resolve_reference.attachment].format = vk.backbufformat;
		attachments[resolve_reference.attachment].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[resolve_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[resolve_reference.attachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[resolve_reference.attachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[resolve_reference.attachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[resolve_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[resolve_reference.attachment].finalLayout = (pass&RP_PRESENTABLE)?VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = (resolve_reference.attachment != ~(uint32_t)0)?&resolve_reference:NULL;
	subpass.pDepthStencilAttachment = &depth_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	rp_info.attachmentCount = numattachments;
	rp_info.pAttachments = attachments;
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = NULL;

	switch(pass&3)
	{
	case RP_RESUME:
		//nothing cleared, both are just re-loaded.
		attachments[color_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[depth_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		break;
	case RP_DEPTHCLEAR:
		//depth cleared, colour is whatever.
		attachments[depth_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[color_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[depth_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		break;
	case RP_FULLCLEAR:
		//both cleared
		attachments[color_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[depth_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[color_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[depth_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		break;
	case RP_DEPTHONLY:
		attachments[depth_reference.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//		attachments[color_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[depth_reference.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

		attachments[depth_reference.attachment].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAssert(vkCreateRenderPass(vk.device, &rp_info, vkallocationcb, &vk.renderpass[pass]));
	DebugSetName(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)vk.renderpass[pass], va("RP%i", pass));
	return vk.renderpass[pass];
}

void VK_DoPresent(struct vkframe *theframe)
{
	VkResult err;
	uint32_t framenum;
	VkPresentInfoKHR presinfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	if (!theframe)
		return;	//used to ensure that the queue is flushed at shutdown
	framenum = theframe->backbuf - vk.backbufs;
	presinfo.waitSemaphoreCount = 1;
	presinfo.pWaitSemaphores = &theframe->backbuf->presentsemaphore;
	presinfo.swapchainCount = 1;
	presinfo.pSwapchains = &vk.swapchain;
	presinfo.pImageIndices = &framenum;

	{
		RSpeedMark();
		err = vkQueuePresentKHR(vk.queue_present, &presinfo);
		RSpeedEnd(RSPEED_PRESENT);
	}
	{
		RSpeedMark();
#ifdef _DIII4A //karin: VK_SUBOPTIMAL_KHR as success
		if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR)
#else
		if (err)
#endif
		{
			if (err == VK_SUBOPTIMAL_KHR)
				Con_DPrintf("vkQueuePresentKHR: VK_SUBOPTIMAL_KHR\n");
			else if (err == VK_ERROR_OUT_OF_DATE_KHR)
				Con_DPrintf("vkQueuePresentKHR: VK_ERROR_OUT_OF_DATE_KHR\n");
			else
				Con_Printf("ERROR: vkQueuePresentKHR: %i\n", err);
			vk.neednewswapchain = true;
		}
		else
		{
			int r = vk.acquirelast%ACQUIRELIMIT;
			uint64_t timeout = (vk.acquirelast==vk.acquirenext)?UINT64_MAX:0;	//
			err = vkAcquireNextImageKHR(vk.device, vk.swapchain, timeout, vk.acquiresemaphores[r], vk.acquirefences[r], &vk.acquirebufferidx[r]);
			switch(err)
			{
			case VK_SUBOPTIMAL_KHR:	//success, but with a warning.
#if !defined(_DIII4A) //karin: VK_SUBOPTIMAL_KHR as success
				vk.neednewswapchain = true;
#endif
				vk.acquirelast++;
				break;
			case VK_SUCCESS:	//success
				vk.acquirelast++;
				break;

			//we gave the presentation engine an image, but its refusing to give us one back.
			//logically this means the implementation lied about its VkSurfaceCapabilitiesKHR::minImageCount
			case VK_TIMEOUT:	//'success', yet still no result
			case VK_NOT_READY:
				//no idea how to handle. let it slip?
				if (vk.acquirelast == vk.acquirenext)
					vk.neednewswapchain = true;	//slipped too much
				break;

			case VK_ERROR_OUT_OF_DATE_KHR:
				//unable to present, but we at least don't need to throw everything away.
				vk.neednewswapchain = true;
				break;
			case VK_ERROR_DEVICE_LOST:
			case VK_ERROR_OUT_OF_HOST_MEMORY:
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			case VK_ERROR_SURFACE_LOST_KHR:
				//something really bad happened.
				Con_Printf("ERROR: vkAcquireNextImageKHR: %s\n", VK_VKErrorToString(err));
				vk.neednewswapchain = true;
				vk.devicelost = true;
				break;
			default:
			//case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
				//we don't know why we're getting this. vendor problem.
				Con_Printf("ERROR: vkAcquireNextImageKHR: undocumented/extended %s\n", VK_VKErrorToString(err));
				vk.neednewswapchain = true;
				vk.devicelost = true;	//this might be an infinite loop... no idea how to handle it.
				break;
			}
		}
		RSpeedEnd(RSPEED_ACQUIRE);
	}
}

static void VK_Submit_DoWork(void)
{
	VkCommandBuffer cbuf[64];
	VkSemaphore wsem[64];
	VkPipelineStageFlags wsemstageflags[64];
	VkSemaphore ssem[64];

	VkQueue	subqueue = NULL;
	VkSubmitInfo subinfo[64];
	unsigned int subcount = 0;
	struct vkwork_s *work;
	struct vkframe *present = NULL;
	VkFence waitfence = VK_NULL_HANDLE;
	VkResult err;
	struct vk_fencework *fencedwork = NULL;
	qboolean errored = false;

	while(vk.work && !present && !waitfence && !fencedwork && subcount < countof(subinfo))
	{
		work = vk.work;
		if (subcount && subqueue != work->queue)
			break;
		subinfo[subcount].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subinfo[subcount].pNext = NULL;
		subinfo[subcount].waitSemaphoreCount = work->semwait?1:0;
		subinfo[subcount].pWaitSemaphores = &wsem[subcount];
		wsem[subcount] = work->semwait;
		subinfo[subcount].pWaitDstStageMask = &wsemstageflags[subcount];
		wsemstageflags[subcount] = work->semwaitstagemask;
		subinfo[subcount].commandBufferCount = work->cmdbuf?1:0;
		subinfo[subcount].pCommandBuffers = &cbuf[subcount];
		cbuf[subcount] = work->cmdbuf;
		subinfo[subcount].signalSemaphoreCount = work->semsignal?1:0;
		subinfo[subcount].pSignalSemaphores = &ssem[subcount];
		ssem[subcount] = work->semsignal;
		waitfence = work->fencesignal;
		fencedwork = work->fencedwork;
		subqueue = work->queue;

		subcount++;

		present = work->present;

		vk.work = work->next;
		Z_Free(work);
	}

	Sys_UnlockConditional(vk.submitcondition);	//don't block people giving us work while we're occupied
	if (subcount || waitfence)
	{
		RSpeedMark();
		err = vkQueueSubmit(subqueue, subcount, subinfo, waitfence);
		if (err)
		{
			if (!vk.devicelost)
				Con_Printf(CON_ERROR "ERROR: vkQueueSubmit: %s\n", VK_VKErrorToString(err));
			errored = vk.neednewswapchain = true;
			vk.devicelost |= (err==VK_ERROR_DEVICE_LOST);
		}
		RSpeedEnd(RSPEED_SUBMIT);
	}

	if (present && !errored)
	{
		vk.dopresent(present);
	}
	
	Sys_LockConditional(vk.submitcondition);

	if (fencedwork)
	{	//this is used for loading and cleaning up things after the gpu has consumed it.
		if (vk.fencework_last)
		{
			vk.fencework_last->next = fencedwork;
			vk.fencework_last = fencedwork;
		}
		else
			vk.fencework_last = vk.fencework = fencedwork;
	}
}

#ifdef MULTITHREAD
//oh look. a thread.
//nvidia's drivers seem to like doing a lot of blocking in queuesubmit and queuepresent(despite the whole QUEUE thing).
//so thread this work so the main thread doesn't have to block so much.
int VK_Submit_Thread(void *arg)
{
	Sys_LockConditional(vk.submitcondition);
	while(!vk.neednewswapchain)
	{
		if (!vk.work)
			Sys_ConditionWait(vk.submitcondition);

		VK_Submit_DoWork();
	}
	Sys_UnlockConditional(vk.submitcondition);
	return true;
}
#endif

void VK_Submit_Work(VkCommandBuffer cmdbuf, VkSemaphore semwait, VkPipelineStageFlags semwaitstagemask, VkSemaphore semsignal, VkFence fencesignal, struct vkframe *presentframe, struct vk_fencework *fencedwork)
{
	struct vkwork_s *work = Z_Malloc(sizeof(*work));
	struct vkwork_s **link;

	work->queue = vk.queue_render;
	work->cmdbuf = cmdbuf;
	work->semwait = semwait;
	work->semwaitstagemask = semwaitstagemask;
	work->semsignal = semsignal;
	work->fencesignal = fencesignal;
	work->present = presentframe;
	work->fencedwork = fencedwork;

	Sys_LockConditional(vk.submitcondition);

#ifdef MULTITHREAD
	if (vk.neednewswapchain && vk.submitthread)
	{	//if we're trying to kill the submission thread, don't post work to it - instead wait for it to die cleanly then do it ourselves.
		Sys_ConditionSignal(vk.submitcondition);
		Sys_UnlockConditional(vk.submitcondition);
		Sys_WaitOnThread(vk.submitthread);
		vk.submitthread = NULL;
		Sys_LockConditional(vk.submitcondition);	//annoying, but required for it to be reliable with respect to other things.
	}
#endif

	//add it on the end in a lazy way.
	for (link = &vk.work; *link; link = &(*link)->next)
		;
	*link = work;

#ifdef MULTITHREAD
	if (vk.submitthread)
		Sys_ConditionSignal(vk.submitcondition);
	else
#endif
		VK_Submit_DoWork();
	Sys_UnlockConditional(vk.submitcondition);
}

void VK_Submit_Sync(void)
{
	Sys_LockConditional(vk.submitcondition);
	//FIXME: 
	vkDeviceWaitIdle(vk.device); //just in case
	Sys_UnlockConditional(vk.submitcondition);
}

void VK_CheckTextureFormats(void)
{
	struct {
		unsigned int pti;
		VkFormat vulkan;
		unsigned int needextra;
	} texfmt[] =
	{
		{PTI_RGBA8,				VK_FORMAT_R8G8B8A8_UNORM,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_RGBX8,				VK_FORMAT_R8G8B8A8_UNORM,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BGRA8,				VK_FORMAT_B8G8R8A8_UNORM,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BGRX8,				VK_FORMAT_B8G8R8A8_UNORM,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},

		{PTI_RGB8,				VK_FORMAT_R8G8B8_UNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BGR8,				VK_FORMAT_B8G8R8_UNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},

		{PTI_RGBA8_SRGB,		VK_FORMAT_R8G8B8A8_SRGB,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_RGBX8_SRGB,		VK_FORMAT_R8G8B8A8_SRGB,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_BGRA8_SRGB,		VK_FORMAT_B8G8R8A8_SRGB,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_BGRX8_SRGB,		VK_FORMAT_B8G8R8A8_SRGB,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},

		{PTI_E5BGR9,			VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_B10G11R11F,		VK_FORMAT_B10G11R11_UFLOAT_PACK32,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_A2BGR10,			VK_FORMAT_A2B10G10R10_UNORM_PACK32,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_RGB565,			VK_FORMAT_R5G6B5_UNORM_PACK16,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_RGBA4444,			VK_FORMAT_R4G4B4A4_UNORM_PACK16,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
#ifdef VK_EXT_4444_formats
		{PTI_ARGB4444,			VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT,VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
#endif
		{PTI_RGBA5551,			VK_FORMAT_R5G5B5A1_UNORM_PACK16,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ARGB1555,			VK_FORMAT_A1R5G5B5_UNORM_PACK16,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_RGBA16F,			VK_FORMAT_R16G16B16A16_SFLOAT,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT},
		{PTI_RGBA32F,			VK_FORMAT_R32G32B32A32_SFLOAT,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT},
		{PTI_RGB32F,			VK_FORMAT_R32G32B32_SFLOAT,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_L8,				VK_FORMAT_R8_UNORM,					VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_L8A8,				VK_FORMAT_R8G8_UNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_L8_SRGB,			VK_FORMAT_R8_SRGB,					VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_R8,				VK_FORMAT_R8_UNORM,					VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_RG8,				VK_FORMAT_R8G8_UNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_R8_SNORM,			VK_FORMAT_R8_SNORM,					VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_RG8_SNORM,			VK_FORMAT_R8G8_SNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_R16,				VK_FORMAT_R16_UNORM,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_RGBA16,			VK_FORMAT_R16G16B16A16_UNORM,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_R16F,				VK_FORMAT_R16_SFLOAT,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
		{PTI_R32F,				VK_FORMAT_R32_SFLOAT,				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},

		{PTI_DEPTH16,			VK_FORMAT_D16_UNORM,				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
		{PTI_DEPTH24,			VK_FORMAT_X8_D24_UNORM_PACK32,		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
		{PTI_DEPTH32,			VK_FORMAT_D32_SFLOAT,				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
		{PTI_DEPTH24_8,			VK_FORMAT_D24_UNORM_S8_UINT,		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},

		{PTI_BC1_RGB,			VK_FORMAT_BC1_RGB_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC1_RGBA,			VK_FORMAT_BC1_RGBA_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC2_RGBA,			VK_FORMAT_BC2_UNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC3_RGBA,			VK_FORMAT_BC3_UNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC1_RGB_SRGB,		VK_FORMAT_BC1_RGB_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC1_RGBA_SRGB,		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC2_RGBA_SRGB,		VK_FORMAT_BC2_SRGB_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC3_RGBA_SRGB,		VK_FORMAT_BC3_SRGB_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC4_R,				VK_FORMAT_BC4_UNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC4_R_SNORM,		VK_FORMAT_BC4_SNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC5_RG,			VK_FORMAT_BC5_UNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC5_RG_SNORM,		VK_FORMAT_BC5_SNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC6_RGB_UFLOAT,	VK_FORMAT_BC6H_UFLOAT_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC6_RGB_SFLOAT,	VK_FORMAT_BC6H_SFLOAT_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC7_RGBA,			VK_FORMAT_BC7_UNORM_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_BC7_RGBA_SRGB,		VK_FORMAT_BC7_SRGB_BLOCK,			VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC1_RGB8,			VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},	//vulkan doesn't support etc1 (but that's okay, because etc2 is a superset).
		{PTI_ETC2_RGB8,			VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC2_RGB8A1,		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC2_RGB8A8,		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC2_RGB8_SRGB,	VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC2_RGB8A1_SRGB,	VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ETC2_RGB8A8_SRGB,	VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_EAC_R11,			VK_FORMAT_EAC_R11_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_EAC_R11_SNORM,		VK_FORMAT_EAC_R11_SNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_EAC_RG11,			VK_FORMAT_EAC_R11G11_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_EAC_RG11_SNORM,	VK_FORMAT_EAC_R11G11_SNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_4X4_LDR,		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X4_LDR,		VK_FORMAT_ASTC_5x4_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X5_LDR,		VK_FORMAT_ASTC_5x5_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X5_LDR,		VK_FORMAT_ASTC_6x5_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X6_LDR,		VK_FORMAT_ASTC_6x6_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X5_LDR,		VK_FORMAT_ASTC_8x5_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X6_LDR,		VK_FORMAT_ASTC_8x6_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X8_LDR,		VK_FORMAT_ASTC_8x8_UNORM_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X5_LDR,		VK_FORMAT_ASTC_10x5_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X6_LDR,		VK_FORMAT_ASTC_10x6_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X8_LDR,		VK_FORMAT_ASTC_10x8_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X10_LDR,	VK_FORMAT_ASTC_10x10_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X10_LDR,	VK_FORMAT_ASTC_12x10_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X12_LDR,	VK_FORMAT_ASTC_12x12_UNORM_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_4X4_SRGB,		VK_FORMAT_ASTC_4x4_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X4_SRGB,		VK_FORMAT_ASTC_5x4_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X5_SRGB,		VK_FORMAT_ASTC_5x5_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X5_SRGB,		VK_FORMAT_ASTC_6x5_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X6_SRGB,		VK_FORMAT_ASTC_6x6_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X5_SRGB,		VK_FORMAT_ASTC_8x5_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X6_SRGB,		VK_FORMAT_ASTC_8x6_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X8_SRGB,		VK_FORMAT_ASTC_8x8_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X5_SRGB,	VK_FORMAT_ASTC_10x5_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X6_SRGB,	VK_FORMAT_ASTC_10x6_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X8_SRGB,	VK_FORMAT_ASTC_10x8_SRGB_BLOCK,		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X10_SRGB,	VK_FORMAT_ASTC_10x10_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X10_SRGB,	VK_FORMAT_ASTC_12x10_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X12_SRGB,	VK_FORMAT_ASTC_12x12_SRGB_BLOCK,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},

#ifdef VK_EXT_texture_compression_astc_hdr
		{PTI_ASTC_4X4_HDR,		VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X4_HDR,		VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_5X5_HDR,		VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X5_HDR,		VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_6X6_HDR,		VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X5_HDR,		VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X6_HDR,		VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_8X8_HDR,		VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X5_HDR,		VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X6_HDR,		VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X8_HDR,		VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_10X10_HDR,	VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X10_HDR,	VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
		{PTI_ASTC_12X12_HDR,	VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT,	VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT},
#endif
	};
	unsigned int i;
	VkPhysicalDeviceProperties props;

	vkGetPhysicalDeviceProperties(vk.gpu, &props);
	vk.limits = props.limits;

	sh_config.texture2d_maxsize = props.limits.maxImageDimension2D;
	sh_config.texturecube_maxsize = props.limits.maxImageDimensionCube;
	sh_config.texture2darray_maxlayers = props.limits.maxImageArrayLayers;

	for (i = 0; i < countof(texfmt); i++)
	{
		unsigned int need = /*VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |*/ texfmt[i].needextra;
		VkFormatProperties fmt;
		vkGetPhysicalDeviceFormatProperties(vk.gpu, texfmt[i].vulkan, &fmt);

		if ((fmt.optimalTilingFeatures & need) == need)
			sh_config.texfmt[texfmt[i].pti] = true;
	}

	if (sh_config.texfmt[PTI_BC1_RGBA] && sh_config.texfmt[PTI_BC2_RGBA] && sh_config.texfmt[PTI_BC3_RGBA] && sh_config.texfmt[PTI_BC5_RG] && sh_config.texfmt[PTI_BC7_RGBA])
		sh_config.hw_bc = 3;
	if (sh_config.texfmt[PTI_ETC2_RGB8] && sh_config.texfmt[PTI_ETC2_RGB8A1] && sh_config.texfmt[PTI_ETC2_RGB8A8] && sh_config.texfmt[PTI_EAC_RG11])
		sh_config.hw_etc = 2;
	if (sh_config.texfmt[PTI_ASTC_4X4_LDR])
		sh_config.hw_astc = 1;	//the core vulkan formats refer to the ldr profile. hdr is a separate extension, which is still not properly specified..
	if (sh_config.texfmt[PTI_ASTC_4X4_HDR])
		sh_config.hw_astc = 2;	//the core vulkan formats refer to the ldr profile. hdr is a separate extension, which is still not properly specified..
}

//creates a vulkan instance with the additional extensions, and hands a copy of the instance to the caller.
qboolean VK_CreateInstance(vrsetup_t *info, char *vrexts, void *result)
{
	VkInstanceCreateInfo inst_info = *(VkInstanceCreateInfo*)info->userctx;
	VkResult err;
	const char *ext[64];
	unsigned int numext = inst_info.enabledExtensionCount;
	memcpy(ext, inst_info.ppEnabledExtensionNames, numext*sizeof(*ext));
	while (vrexts && numext < countof(ext))
	{
		ext[numext++] = vrexts;
		vrexts = strchr(vrexts, ' ');
		if (!vrexts)
			break;
		*vrexts++ = 0;
	}

	err = vkCreateInstance(&inst_info, vkallocationcb, &vk.instance);
	switch(err)
	{
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		Con_Printf("VK_ERROR_INCOMPATIBLE_DRIVER: please install an appropriate vulkan driver\n");
		return false;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		Con_Printf("VK_ERROR_EXTENSION_NOT_PRESENT: something on a system level is probably misconfigured\n");
		return false;
	case VK_ERROR_LAYER_NOT_PRESENT:
		Con_Printf("VK_ERROR_LAYER_NOT_PRESENT: requested layer is not known/usable\n");
		return false;
	default:
		Con_Printf("Unknown vulkan instance creation error: %x\n", err);
		return false;
	case VK_SUCCESS:
		break;
	}

	if (result)
		*(VkInstance*)result = vk.instance;
	return true;
}

qboolean VK_EnumerateDevices (void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc), const char *descprefix, PFN_vkGetInstanceProcAddr vk_GetInstanceProcAddr)
{
	VkInstance vk_instance;

	VkApplicationInfo app;
	VkInstanceCreateInfo inst_info;

	#if 0	//for quicky debugging...
		#define VKFunc(n) int vk##n;
			VKFuncs
		#undef VKFunc
	#endif

	#define VKFunc(n) PFN_vk##n vk_##n;
		VKFunc(CreateInstance)

		VKFunc(DestroyInstance)
		VKFunc(EnumeratePhysicalDevices)
		VKFunc(GetPhysicalDeviceProperties)
	#undef VKFunc

	//get second set of pointers... (instance-level)
#ifdef VK_NO_PROTOTYPES
	if (!vk_GetInstanceProcAddr)
		return false;
	#define VKFunc(n) vk_##n = (PFN_vk##n)vk_GetInstanceProcAddr(VK_NULL_HANDLE, "vk"#n);
		//VKFunc(EnumerateInstanceLayerProperties)
		//VKFunc(EnumerateInstanceExtensionProperties)
		VKFunc(CreateInstance)
	#undef VKFunc
#endif

	memset(&app, 0, sizeof(app));
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext = NULL;
	app.pApplicationName = FULLENGINENAME;
	app.applicationVersion = revision_number(false);
	app.pEngineName = "FTE Quake";
	app.engineVersion = VK_MAKE_VERSION(FTE_VER_MAJOR, FTE_VER_MINOR, 0);
	app.apiVersion = VK_API_VERSION_1_0;	//make sure it works...

	memset(&inst_info, 0, sizeof(inst_info));
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pApplicationInfo = &app;
	inst_info.enabledLayerCount = vklayercount;
	inst_info.ppEnabledLayerNames = vklayerlist;
	inst_info.enabledExtensionCount = 0;
	inst_info.ppEnabledExtensionNames = NULL;

	if (vk_CreateInstance(&inst_info, vkallocationcb, &vk_instance) != VK_SUCCESS)
		return false;

	//third set of functions...
#ifdef VK_NO_PROTOTYPES
	//vk_GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vk_GetInstanceProcAddr(vk_instance, "vkGetInstanceProcAddr");
	#define VKFunc(n) vk_##n = (PFN_vk##n)vk_GetInstanceProcAddr(vk_instance, "vk"#n);
		VKFunc(DestroyInstance)
		VKFunc(EnumeratePhysicalDevices)
		VKFunc(GetPhysicalDeviceProperties)
	#undef VKFunc
#endif

	//enumerate the gpus
	{
		uint32_t gpucount = 0, i;
		VkPhysicalDevice *devs;
		char gpuname[64];

		vk_EnumeratePhysicalDevices(vk_instance, &gpucount, NULL);
		if (!gpucount)
			return false;
		devs = malloc(sizeof(VkPhysicalDevice)*gpucount);
		vk_EnumeratePhysicalDevices(vk_instance, &gpucount, devs);
		for (i = 0; i < gpucount; i++)
		{
			VkPhysicalDeviceProperties props;
			vk_GetPhysicalDeviceProperties(devs[i], &props);

			Q_snprintfz(gpuname, sizeof(gpuname), "GPU%u", i);
			//FIXME: make sure its not a number or GPU#...
			callback(usercontext, gpuname, "", va("%s%s", descprefix, props.deviceName));
		}
		free(devs);
	}

	vk_DestroyInstance(vk_instance, vkallocationcb);
	return true;
}

//initialise the vulkan instance, context, device, etc.
qboolean VK_Init(rendererstate_t *info, const char **sysextnames, qboolean (*createSurface)(void), void (*dopresent)(struct vkframe *theframe))
{
	VkQueueFamilyProperties *queueprops;
	VkResult err;
	VkApplicationInfo app;
	VkInstanceCreateInfo inst_info;
	int gpuidx = 0;
	const char *extensions[8];
	uint32_t extensions_count = 0;

	qboolean	ignorequeuebugs = false;
	qboolean okay;
	vrsetup_t vrsetup = {sizeof(vrsetup)};

	//device extensions that want to enable
	//initialised in reverse order, so superseeded should name later extensions.
	struct
	{
		qboolean *flag;
		const char *name;
		cvar_t *var;
		qboolean def;				//default value when the cvar is empty.
		qboolean *superseeded;		//if this is set then the extension will not be enabled after all
		const char *warningtext;	//printed if the extension is requested but not supported by the device
		qboolean supported;
	} knowndevexts[] =
	{
		{&vk.khr_swapchain,					VK_KHR_SWAPCHAIN_EXTENSION_NAME,				NULL,							true, NULL, " Nothing will be drawn!"},
		{&vk.khr_get_memory_requirements2,	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,&vk_khr_get_memory_requirements2,true, NULL, NULL},
		{&vk.khr_dedicated_allocation,		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,		&vk_khr_dedicated_allocation,	true, NULL, NULL},
		{&vk.khr_push_descriptor,			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,			&vk_khr_push_descriptor,		true, NULL, NULL},
		{&vk.amd_rasterization_order,		VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,		&vk_amd_rasterization_order,	false, NULL, NULL},
#ifdef VK_KHR_fragment_shading_rate
		{&vk.khr_fragment_shading_rate,		VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,	&vK_khr_fragment_shading_rate,	true, NULL, NULL},
#endif
#ifdef VK_EXT_astc_decode_mode
		{&vk.ext_astc_decode_mode,			VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME,			&vk_ext_astc_decode_mode,		true,  NULL, NULL},
#endif
#ifdef VK_KHR_acceleration_structure
		{&vk.khr_deferred_host_operations,  VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,	&vk_khr_ray_query,				true,  NULL, NULL},	//dependancy of khr_acceleration_structure
		{&vk.khr_acceleration_structure,	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,	&vk_khr_ray_query,				true,  NULL, NULL},
#endif
#ifdef VK_KHR_ray_query
		{&vk.khr_ray_query,					VK_KHR_RAY_QUERY_EXTENSION_NAME,				&vk_khr_ray_query,				true,  NULL, NULL},
#endif
	};
	size_t e;

	for (e = 0; e < countof(knowndevexts); e++)
		*knowndevexts[e].flag = false;
	vk.neednewswapchain = true;
	vk.triplebuffer = info->triplebuffer;
	vk.vsync = info->wait;
	vk.dopresent = dopresent?dopresent:VK_DoPresent;
	memset(&sh_config, 0, sizeof(sh_config));


	//get second set of pointers... (instance-level)
#ifdef VK_NO_PROTOTYPES
	if (!vkGetInstanceProcAddr)
	{
		Con_Printf(CON_ERROR"vkGetInstanceProcAddr is null\n");
		return false;
	}
#define VKFunc(n) vk##n = (PFN_vk##n)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vk"#n);
	VKInstFuncs
#undef VKFunc
#endif

	//try and enable some instance extensions...
	{
		qboolean surfext = false;
		uint32_t count, i, j;
		VkExtensionProperties *ext;
#ifdef VK_EXT_debug_utils
		qboolean havedebugutils = false;
#endif
#ifdef VK_EXT_debug_report
		qboolean havedebugreport = false;
#endif
		if (VK_SUCCESS!=vkEnumerateInstanceExtensionProperties(NULL, &count, NULL))
			count = 0;
		ext = malloc(sizeof(*ext)*count);
		if (!ext || VK_SUCCESS!=vkEnumerateInstanceExtensionProperties(NULL, &count, ext))
			count = 0;
		for (i = 0; i < count && extensions_count < countof(extensions); i++)
		{
			Con_DLPrintf(2, " vki: %s\n", ext[i].extensionName);
#ifdef VK_EXT_debug_utils
			if (!strcmp(ext[i].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
				havedebugutils = true;
#endif
#ifdef VK_EXT_debug_report
			if (!strcmp(ext[i].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
				havedebugreport = true;
#endif
			if (!strcmp(ext[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
				extensions[extensions_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
			else if (sysextnames && !strcmp(ext[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME))
			{
				extensions[extensions_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
				surfext = true;
			}
			else if (sysextnames)
			{
				for (j = 0; sysextnames[j]; j++)
				{
					if (!strcmp(ext[i].extensionName, sysextnames[j]))
					{
						extensions[extensions_count++] = sysextnames[j];
						vk.khr_swapchain = true;
					}
				}
			}
		}
		free(ext);

		if (!vk_debug.ival)
			;
#ifdef VK_EXT_debug_utils
		else if (havedebugutils)
			extensions[extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif
#ifdef VK_EXT_debug_report
		else if (havedebugreport)
			extensions[extensions_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif

		if (sysextnames && (!vk.khr_swapchain || !surfext))
		{
			Con_TPrintf(CON_ERROR"Vulkan instance lacks driver support for %s\n", sysextnames[0]);
			return false;
		}
	}

#define ENGINEVERSION 1
	memset(&app, 0, sizeof(app));
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext = NULL;
	app.pApplicationName = FULLENGINENAME;
	app.applicationVersion = revision_number(false);
	app.pEngineName = "FTE Quake";
	app.engineVersion = VK_MAKE_VERSION(FTE_VER_MAJOR, FTE_VER_MINOR, 0);
	app.apiVersion = VK_API_VERSION_1_0;
	if (vkEnumerateInstanceVersion)
	{
		vkEnumerateInstanceVersion(&app.apiVersion);
#ifdef VK_API_VERSION_1_2
		if (app.apiVersion > VK_API_VERSION_1_2)
			app.apiVersion = VK_API_VERSION_1_2;
#else
		if (app.apiVersion > VK_API_VERSION_1_0)
			app.apiVersion = VK_API_VERSION_1_0;
#endif
	}

	memset(&inst_info, 0, sizeof(inst_info));
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pApplicationInfo = &app;
	inst_info.enabledLayerCount = vklayercount;
	inst_info.ppEnabledLayerNames = vklayerlist;
	inst_info.enabledExtensionCount = extensions_count;
	inst_info.ppEnabledExtensionNames = extensions;

	vrsetup.vrplatform = VR_VULKAN;
	vrsetup.userctx = &inst_info;
	vrsetup.createinstance = VK_CreateInstance;
	if (info->vr)
	{
		okay = info->vr->Prepare(&vrsetup);
		if (!okay)
		{
			info->vr->Shutdown();
			info->vr = NULL;
		}
	}
	else
		okay = false;
	if (!okay)
		okay = vrsetup.createinstance(&vrsetup, NULL, NULL);
	if (!okay)
	{
		Con_TPrintf(CON_ERROR"Unable to create vulkan instance\n");
		if (info->vr)
			info->vr->Shutdown();
		return false;
	}
	vid.vr = info->vr;

#ifdef MULTITHREAD
	vk.allowsubmissionthread = !vid.vr;
#endif

	//third set of functions...
#ifdef VK_NO_PROTOTYPES
	//vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetInstanceProcAddr");
#define VKFunc(n) vk##n = (PFN_vk##n)vkGetInstanceProcAddr(vk.instance, "vk"#n);
	VKInst2Funcs
#undef VKFunc
#endif

	//set up debug callbacks
	if (vk_debug.ival)
	{
#ifdef VK_EXT_debug_utils
		vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
		vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkCreateDebugUtilsMessengerEXT)
		{
			VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo;
			memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			dbgCreateInfo.pfnUserCallback = mydebugutilsmessagecallback;
			dbgCreateInfo.pUserData = NULL;
			dbgCreateInfo.messageSeverity =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT	|
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT	|
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT	|
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			dbgCreateInfo.messageType =	    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			vkCreateDebugUtilsMessengerEXT(vk.instance, &dbgCreateInfo, vkallocationcb, &vk_debugucallback);
		}
#endif
#ifdef VK_EXT_debug_report
		vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugReportCallbackEXT");
		if (vkCreateDebugReportCallbackEXT && vkDestroyDebugReportCallbackEXT)
		{
			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
			memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pfnCallback = mydebugreportcallback;
			dbgCreateInfo.pUserData = NULL;
			dbgCreateInfo.flags =	VK_DEBUG_REPORT_ERROR_BIT_EXT |
									VK_DEBUG_REPORT_WARNING_BIT_EXT	|
/*									VK_DEBUG_REPORT_INFORMATION_BIT_EXT	| */
									VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
									VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			vkCreateDebugReportCallbackEXT(vk.instance, &dbgCreateInfo, vkallocationcb, &vk_debugcallback);
		}
#endif
	}

	//create the platform-specific surface
	createSurface();

	//figure out which gpu we're going to use
	{
		uint32_t gpucount = 0, i;
		uint32_t bestpri = ~0u, pri;
		VkPhysicalDevice *devs;
		char *s = info->subrenderer;
		int wantdev = -1;
		if (*s)
		{
			if (!Q_strncasecmp(s, "GPU", 3))
				s += 3;
			wantdev = strtoul(s, &s, 0);
			if (*s)	//its a named device.
				wantdev = -1;
		}

		vkEnumeratePhysicalDevices(vk.instance, &gpucount, NULL);
		if (!gpucount)
		{
			Con_Printf(CON_ERROR"vulkan: no devices known!\n");
			return false;
		}
		devs = malloc(sizeof(VkPhysicalDevice)*gpucount);
		vkEnumeratePhysicalDevices(vk.instance, &gpucount, devs);
		for (i = 0; i < gpucount; i++)
		{
			VkPhysicalDeviceProperties props;
			uint32_t j, queue_count = 0;
			vkGetPhysicalDeviceProperties(devs[i], &props);
			vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &queue_count, NULL);

			if (vk.khr_swapchain)
			{
				for (j = 0; j < queue_count; j++)
				{
					VkBool32 supportsPresent = false;
					VkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(devs[i], j, vk.surface, &supportsPresent));
					if (supportsPresent)
						break;	//okay, this one should be usable
				}
				if (j == queue_count)
				{
					if ((wantdev >= 0 && i==wantdev) || (wantdev==-1 && *info->subrenderer && !Q_strcasecmp(props.deviceName, info->subrenderer)))
					{
						Con_Printf(CON_WARNING"vulkan: attempting to use device \"%s\" despite no device queues being able to present to window surface\n", props.deviceName);
						ignorequeuebugs = true;
					}
					else
					{
						//no queues can present to that surface, so I guess we can't use that device
						Con_DLPrintf((wantdev != i)?1:0, "vulkan: ignoring device \"%s\" as it can't present to window\n", props.deviceName);
						continue;
					}
				}
			}
			Con_DPrintf("Found Vulkan Device \"%s\"\n", props.deviceName);

			if (!vk.gpu)
			{
				gpuidx = i;
				vk.gpu = devs[i];
			}
			switch(props.deviceType)
			{
			default:
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				pri = 5;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				pri = 2;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				pri = 1;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				pri = 3;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				pri = 4;
				break;
			}
			if (vrsetup.vk.physicaldevice != VK_NULL_HANDLE)
			{	//if we're using vr, use the gpu our vr context requires.
				if (devs[i] == vrsetup.vk.physicaldevice)
					pri = 0;
			}
			else if (wantdev >= 0)
			{
				if (wantdev == i)
					pri = 0;
			}
			else
			{
				if (!Q_strcasecmp(props.deviceName, info->subrenderer))
					pri = 0;
			}

			if (pri < bestpri)
			{
				gpuidx = i;
				vk.gpu = devs[gpuidx];
				bestpri = pri;
			}
		}
		free(devs);

		if (!vk.gpu)
		{
			Con_Printf(CON_ERROR"vulkan: unable to pick a usable device\n");
			return false;
		}
	}

	{
		char *vendor, *type;
#ifdef VK_API_VERSION_1_2
		VkPhysicalDeviceVulkan12Properties props12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
		VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &props12};
#else
		struct {VkPhysicalDeviceProperties properties;} props;
#endif
		vkGetPhysicalDeviceProperties(vk.gpu, &props.properties);	//legacy

		vk.apiversion = props.properties.apiVersion;
		if (vk.apiversion > app.apiVersion)
			vk.apiversion = app.apiVersion;	//cap it to the instance version...

#ifdef VK_API_VERSION_1_2
		if (vk.apiversion >= VK_API_VERSION_1_2)
		{
			PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(vk.instance, "vkGetPhysicalDeviceProperties2");
			if (vkGetPhysicalDeviceProperties2)
				vkGetPhysicalDeviceProperties2(vk.gpu, &props);
		}
	
		if (*props12.driverName)
			vendor = props12.driverName;
		else
#endif
		switch(props.properties.vendorID)
		{
		//explicit registered vendors
		case 0x10001: vendor = "Vivante";		break;
		case 0x10002: vendor = "VeriSilicon";	break;
		case 0x10003: vendor = "Kazan";		break;
		case 0x10004: vendor = "Codeplay";		break;
		case /*VK_VENDOR_ID_MESA*/0x10005: vendor = "MESA";	break;

		//pci vendor ids
		//there's a lot of pci vendors, some even still exist, but not all of them actually have 3d hardware.
		//many of these probably won't even be used... Oh well.
		//anyway, here's some of the ones that are listed
		case 0x1002: vendor = "AMD";		break;
		case 0x10DE: vendor = "NVIDIA";		break;
		case 0x8086: vendor = "Intel";		break; //cute
		case 0x13B5: vendor = "ARM";		break;
		case 0x5143: vendor = "Qualcomm";	break;
		case 0x1AEE: vendor = "Imagination";break;
		case 0x1957: vendor = "Freescale";	break;

		//I really have no idea who makes mobile gpus nowadays, but lets make some guesses.
		case 0x1AE0: vendor = "Google";		break;
		case 0x5333: vendor = "S3";			break;
		case 0xA200: vendor = "NEC";		break;
		case 0x0A5C: vendor = "Broadcom";	break;
		case 0x1131: vendor = "NXP";		break;
		case 0x1099: vendor = "Samsung";	break;
		case 0x10C3: vendor = "Samsung";	break;
		case 0x11E2: vendor = "Samsung";	break;
		case 0x1249: vendor = "Samsung";	break;
		
		default:	vendor = va("VEND_%x", props.properties.vendorID); break;
		}

		switch(props.properties.deviceType)
		{
		default:
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:				type = "(other)"; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:	type = "integrated"; break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:		type = "discrete"; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:		type = "virtual"; break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:				type = "software"; break;
		}

#ifdef VK_API_VERSION_1_2
		if (*props12.driverInfo)
		{
			Con_TPrintf("Vulkan Driver Name: %s\n"
						"Vulkan Device (GPU%i): %s\n"
						"Vulkan Driver Info: %s\n",
						vendor,
						gpuidx, props.properties.deviceName,
						props12.driverInfo );
		}
		else
#endif
		{
			Con_TPrintf("Vulkan %u.%u.%u: GPU%i %s %s %s (%u.%u.%u)\n", VK_VERSION_MAJOR(props.properties.apiVersion), VK_VERSION_MINOR(props.properties.apiVersion), VK_VERSION_PATCH(props.properties.apiVersion),
				gpuidx, type, vendor, props.properties.deviceName,
				VK_VERSION_MAJOR(props.properties.driverVersion), VK_VERSION_MINOR(props.properties.driverVersion), VK_VERSION_PATCH(props.properties.driverVersion)
				);
		}
	}

	//figure out which of the device's queue's we're going to use
	{
		uint32_t queue_count, i;
		vkGetPhysicalDeviceQueueFamilyProperties(vk.gpu, &queue_count, NULL);
		queueprops = malloc(sizeof(VkQueueFamilyProperties)*queue_count);	//Oh how I wish I was able to use C99.
		vkGetPhysicalDeviceQueueFamilyProperties(vk.gpu, &queue_count, queueprops);

		vk.queuefam[0] = ~0u;
		vk.queuefam[1] = ~0u;
		vk.queuenum[0] = 0;
		vk.queuenum[1] = 0;

		/*
		//try to find a 'dedicated' present queue
		for (i = 0; i < queue_count; i++)
		{
			VkBool32 supportsPresent = FALSE;
			VkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(vk.gpu, i, vk.surface, &supportsPresent));

			if (supportsPresent && !(queueprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				vk.queuefam[1] = i;
				break;
			}
		}

		if (vk.queuefam[1] != ~0u)
		{	//try to find a good graphics queue
			for (i = 0; i < queue_count; i++)
			{
				if (queueprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					vk.queuefam[0] = i;
					break;
				}
			}
		}
		else*/
		{
			for (i = 0; i < queue_count; i++)
			{
				VkBool32 supportsPresent = false;
				if (!vk.khr_swapchain)
					supportsPresent = true;	//won't be used anyway.
				else
					VkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(vk.gpu, i, vk.surface, &supportsPresent));

				if ((queueprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
				{
					vk.queuefam[0] = i;
					vk.queuefam[1] = i;
					break;
				}
				else if (vk.queuefam[0] == ~0u && (queueprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
					vk.queuefam[0] = i;
				else if (vk.queuefam[1] == ~0u && supportsPresent)
					vk.queuefam[1] = i;
			}
		}


		if (vk.queuefam[0] == ~0u || vk.queuefam[1] == ~0u)
		{
			if (ignorequeuebugs && queue_count>0)
			{
				vk.queuefam[0] = vk.queuefam[1] = 0;
			}
			else
			{
				free(queueprops);
				Con_Printf(CON_ERROR"vulkan: unable to find suitable queues\n");
				return false;
			}
		}
	}

	{
		uint32_t extcount = 0, i;
		VkExtensionProperties *ext;
		vkEnumerateDeviceExtensionProperties(vk.gpu, NULL, &extcount, NULL);
		ext = malloc(sizeof(*ext)*extcount);
		vkEnumerateDeviceExtensionProperties(vk.gpu, NULL, &extcount, ext);
		for (i = 0; i < extcount; i++)
			Con_DLPrintf(2, " vkd: %s\n", ext[i].extensionName);
		while (extcount --> 0)
		{
			for (e = 0; e < countof(knowndevexts); e++)
			{
				if (!strcmp(ext[extcount].extensionName, knowndevexts[e].name))
				{
					if (knowndevexts[e].var)
						*knowndevexts[e].flag = !!knowndevexts[e].var->ival || (!*knowndevexts[e].var->string && knowndevexts[e].def);
					knowndevexts[e].supported = true;
				}
			}
		}
		free(ext);
	}

#ifdef VK_KHR_ray_query
	if ((vk.khr_ray_query && !vk.khr_acceleration_structure) || vk.apiversion < VK_API_VERSION_1_2)
		vk.khr_ray_query = false;	//doesn't make sense.
#endif
#ifdef VK_KHR_acceleration_structure
	if ((vk.khr_acceleration_structure && !vk.khr_ray_query) || vk.apiversion < VK_API_VERSION_1_2)
		vk.khr_acceleration_structure = false;	//not useful.
#endif
#ifdef VK_KHR_fragment_shading_rate
	if (vk.apiversion < VK_API_VERSION_1_2)	//too lazy to check its requesite extensions. vk12 is enough.
		vk.khr_fragment_shading_rate = false;
#endif

	{
		const char *devextensions[1+countof(knowndevexts)];
		size_t numdevextensions = 0;
		float queue_priorities[2] = {0.8, 1.0};
		VkDeviceQueueCreateInfo queueinf[2] = {{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO},{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}};
		VkDeviceCreateInfo devinf = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceFeatures avail;
		void *next = NULL;
#ifdef VK_KHR_fragment_shading_rate
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingrate = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
#endif
#ifdef VK_KHR_ray_query
		VkPhysicalDeviceRayQueryFeaturesKHR rayquery = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
#endif
#ifdef VK_KHR_acceleration_structure
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelstruct = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
#endif
#ifdef VK_API_VERSION_1_2
		VkPhysicalDeviceVulkan12Features vk12features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
#endif
		memset(&features, 0, sizeof(features));

		vkGetPhysicalDeviceFeatures(vk.gpu, &avail);

		//try to enable whatever we can use, if we can.
		features.robustBufferAccess			= avail.robustBufferAccess;
		features.textureCompressionBC		= avail.textureCompressionBC;
		features.textureCompressionETC2		= avail.textureCompressionETC2;
		features.textureCompressionASTC_LDR	= avail.textureCompressionASTC_LDR;
		features.samplerAnisotropy			= avail.samplerAnisotropy;
		features.geometryShader				= avail.geometryShader;
		features.tessellationShader			= avail.tessellationShader;

		//Add in the extensions we support
		for (e = 0; e < countof(knowndevexts); e++)
		{	//prints are to let the user know what's going on. only warn if its explicitly enabled
			if (knowndevexts[e].superseeded && *knowndevexts[e].superseeded)
			{
				Con_DPrintf("Superseeded %s.\n", knowndevexts[e].name);
				*knowndevexts[e].flag = false;
			}
			else if (*knowndevexts[e].flag)
			{
				Con_DPrintf("Using %s.\n", knowndevexts[e].name);
				devextensions[numdevextensions++] = knowndevexts[e].name;
			}
			else if (knowndevexts[e].var && knowndevexts[e].var->ival)
				Con_Printf(CON_WARNING"unable to enable %s extension.%s\n", knowndevexts[e].name, knowndevexts[e].warningtext?knowndevexts[e].warningtext:"");
			else if (knowndevexts[e].supported)
				Con_DPrintf("Ignoring %s.\n", knowndevexts[e].name);
			else
				Con_DPrintf("Unavailable %s.\n", knowndevexts[e].name);
		}

		queueinf[0].pNext = NULL;
		queueinf[0].queueFamilyIndex = vk.queuefam[0];
		queueinf[0].queueCount = 1;
		queueinf[0].pQueuePriorities = &queue_priorities[0];
		queueinf[1].pNext = NULL;
		queueinf[1].queueFamilyIndex = vk.queuefam[1];
		queueinf[1].queueCount = 1;
		queueinf[1].pQueuePriorities = &queue_priorities[1];

		if (vk.queuefam[0] == vk.queuefam[1])
		{
			devinf.queueCreateInfoCount = 1;

			if (queueprops[queueinf[0].queueFamilyIndex].queueCount >= 2 && vk_dualqueue.ival)
			{
				queueinf[0].queueCount = 2;
				vk.queuenum[1] = 1; 
				Con_DPrintf("Using duel queue\n");
			}
			else
			{
				queueinf[0].queueCount = 1;
				if (vk.khr_swapchain)
					vk.dopresent = VK_DoPresent;	//can't split submit+present onto different queues, so do these on a single thread.
				Con_DPrintf("Using single queue\n");
			}
		}
		else
		{
			devinf.queueCreateInfoCount = 2;
			Con_DPrintf("Using separate queue families\n");
		}

		free(queueprops);

		devinf.pQueueCreateInfos = queueinf;
		devinf.enabledLayerCount = vklayercount;
		devinf.ppEnabledLayerNames = vklayerlist;
		devinf.enabledExtensionCount = numdevextensions;
		devinf.ppEnabledExtensionNames = devextensions;
		devinf.pEnabledFeatures = &features;

#ifdef VK_KHR_fragment_shading_rate
		if (vk.khr_fragment_shading_rate)
		{
			shadingrate.pNext = next;
			next = &shadingrate;	//now linked
			shadingrate.pipelineFragmentShadingRate = true;
			shadingrate.primitiveFragmentShadingRate = false;
			shadingrate.attachmentFragmentShadingRate = false;
		}
#endif
#ifdef VK_KHR_ray_query
		if (vk.khr_ray_query)
		{
			rayquery.pNext = next;
			next = &rayquery;	//now linked
			rayquery.rayQuery = true;
		}
#endif
#ifdef VK_KHR_acceleration_structure
		if (vk.khr_acceleration_structure)
		{
			accelstruct.pNext = next;
			next = &accelstruct;	//now linked
			accelstruct.accelerationStructure = true;
			accelstruct.accelerationStructureCaptureReplay = false;
			accelstruct.accelerationStructureIndirectBuild = false;
			accelstruct.accelerationStructureHostCommands = false;
			accelstruct.descriptorBindingAccelerationStructureUpdateAfterBind = false;

			vk12features.bufferDeviceAddress = true;	//we also need this feature.
		}
#endif
#ifdef VK_API_VERSION_1_2
		if (vk.apiversion >= VK_API_VERSION_1_2)
		{
			vk12features.pNext = next;
			next = &vk12features;
		}
#endif
		devinf.pNext = next;

#if 0
		if (vkEnumeratePhysicalDeviceGroupsKHR && vk_afr.ival)
		{	
			//'Every physical device must be in exactly one device group'. So we can just use the first group that lists it and automatically get AFR.
			uint32_t gpugroups = 0;
			VkDeviceGroupDeviceCreateInfoKHX dgdci = {VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHR};

			VkPhysicalDeviceGroupPropertiesKHR *groups;
			vkEnumeratePhysicalDeviceGroupsKHR(vk.instance, &gpugroups, NULL);
			groups = malloc(sizeof(*groups)*gpugroups);
			vkEnumeratePhysicalDeviceGroupsKHR(vk.instance, &gpugroups, groups);
			for (i = 0; i < gpugroups; i++)
			{
				for (j = 0; j < groups[i].physicalDeviceCount; j++)
					if (groups[i].physicalDevices[j] == vk.gpu)
					{
						dgdci.physicalDeviceCount = groups[i].physicalDeviceCount;
						dgdci.pPhysicalDevices = groups[i].physicalDevices;
						break;
					}
			}
			
			if (dgdci.physicalDeviceCount > 1)
			{
				vk.subdevices = dgdci.physicalDeviceCount;
				dgdci.pNext = devinf.pNext;
				devinf.pNext = &dgdci;
			}
		
			err = vkCreateDevice(vk.gpu, &devinf, NULL, &vk.device);

			free(groups);
		}
		else
#endif
			err = vkCreateDevice(vk.gpu, &devinf, NULL, &vk.device);

		switch(err)
		{
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			Con_TPrintf(CON_ERROR"VK_ERROR_INCOMPATIBLE_DRIVER: please install an appropriate vulkan driver\n");
			return false;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
		case VK_ERROR_FEATURE_NOT_PRESENT:
		case VK_ERROR_INITIALIZATION_FAILED:
		case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			Con_Printf(CON_ERROR"%s: something on a system level is probably misconfigured\n", VK_VKErrorToString(err));
			return false;
		default:
			Con_Printf(CON_ERROR"Unknown vulkan device creation error: %x\n", err);
			return false;
		case VK_SUCCESS:
			break;
		}
	}

#ifdef VK_NO_PROTOTYPES
	vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetDeviceProcAddr");
#define VKFunc(n) vk##n = (PFN_vk##n)vkGetDeviceProcAddr(vk.device, "vk"#n);
	VKDevFuncs
#ifdef VK_KHR_acceleration_structure
	if (vk.khr_acceleration_structure) { VKAccelStructFuncs }
#endif
#undef VKFunc
#endif

	vkGetDeviceQueue(vk.device, vk.queuefam[0], vk.queuenum[0], &vk.queue_render);
	vkGetDeviceQueue(vk.device, vk.queuefam[1], vk.queuenum[1], &vk.queue_present);

	vrsetup.vk.instance = vk.instance;
	vrsetup.vk.device = vk.device;
	vrsetup.vk.physicaldevice = vk.gpu;
	vrsetup.vk.queuefamily = vk.queuefam[1];
	vrsetup.vk.queueindex = vk.queuenum[1];
	if (vid.vr)
	{
		if (!vid.vr->Init(&vrsetup, info))
		{
			vid.vr->Shutdown();
			vid.vr = NULL;
		}
	}

	vkGetPhysicalDeviceMemoryProperties(vk.gpu, &vk.memory_properties);

	{
		VkCommandPoolCreateInfo cpci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		cpci.queueFamilyIndex = vk.queuefam[0];
		cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkAssert(vkCreateCommandPool(vk.device, &cpci, vkallocationcb, &vk.cmdpool));
	}

	
	sh_config.progpath = "vulkan/%s.fvb";
	sh_config.blobpath = NULL;	//just use general pipeline cache instead.
	sh_config.shadernamefmt = NULL;//"_vulkan";

	sh_config.progs_supported = true;
	sh_config.progs_required = true;
	sh_config.minver = -1;
	sh_config.maxver = -1;

	sh_config.texture_allow_block_padding = true;
	sh_config.texture_non_power_of_two = true;	//is this always true?
	sh_config.texture_non_power_of_two_pic = true;	//probably true...
	sh_config.npot_rounddown = false;
	sh_config.tex_env_combine = false;		//fixme: figure out what this means...
	sh_config.nv_tex_env_combine4 = false;	//fixme: figure out what this means...
	sh_config.env_add = false;				//fixme: figure out what this means...

	sh_config.can_mipcap = true;
	sh_config.havecubemaps = true;

	VK_CheckTextureFormats();


	sh_config.pDeleteProg = NULL;
	sh_config.pLoadBlob = NULL;
	sh_config.pCreateProgram = NULL;
	sh_config.pValidateProgram = NULL;
	sh_config.pProgAutoFields = NULL;

	if (info->depthbits == 16 && sh_config.texfmt[PTI_DEPTH16])
		vk.depthformat = VK_FORMAT_D16_UNORM;
	else if (info->depthbits == 32 && sh_config.texfmt[PTI_DEPTH32])
		vk.depthformat = VK_FORMAT_D32_SFLOAT;
//	else if (info->depthbits == 32 && sh_config.texfmt[PTI_DEPTH32_8])
//		vk.depthformat = VK_FORMAT_D32_SFLOAT_S8_UINT;
	else if (info->depthbits == 24 && sh_config.texfmt[PTI_DEPTH24_8])
		vk.depthformat = VK_FORMAT_D24_UNORM_S8_UINT;
	else if (info->depthbits == 24 && sh_config.texfmt[PTI_DEPTH24])
		vk.depthformat = VK_FORMAT_X8_D24_UNORM_PACK32;

	else if (sh_config.texfmt[PTI_DEPTH24])
		vk.depthformat = VK_FORMAT_X8_D24_UNORM_PACK32;
	else if (sh_config.texfmt[PTI_DEPTH24_8])
		vk.depthformat = VK_FORMAT_D24_UNORM_S8_UINT;
	else if (sh_config.texfmt[PTI_DEPTH32])	//nvidia: "Don’t use 32-bit floating point depth formats, due to the performance cost, unless improved precision is actually required"
		vk.depthformat = VK_FORMAT_D32_SFLOAT;
	else	//16bit depth is guarenteed in vulkan
		vk.depthformat = VK_FORMAT_D16_UNORM;

#ifdef MULTITHREAD
	vk.submitcondition = Sys_CreateConditional();
#endif

	{
		VkPipelineCacheCreateInfo pci = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
		qofs_t size = 0;
		pci.pInitialData = FS_MallocFile("vulkan.pcache", FS_ROOT, &size);
		pci.initialDataSize = size;
		VkAssert(vkCreatePipelineCache(vk.device, &pci, vkallocationcb, &vk.pipelinecache));
		FS_FreeFile((void*)pci.pInitialData);
	}

	if (VK_CreateSwapChain())
	{
		vk.neednewswapchain = false;

#ifdef MULTITHREAD
		if (vk.allowsubmissionthread && (vk_submissionthread.ival || !*vk_submissionthread.string))
		{
			vk.submitthread = Sys_CreateThread("vksubmission", VK_Submit_Thread, NULL, THREADP_HIGHEST, 0);
		}
#endif
	}
	if (info->srgb > 0 && (vid.flags & VID_SRGB_FB))
		vid.flags |= VID_SRGBAWARE;

	Q_snprintfz(info->subrenderer, sizeof(info->subrenderer), "GPU%i", gpuidx);

	if (!vk.khr_fragment_shading_rate)
		Cvar_LockUnsupportedRendererCvar(&r_halfrate, "0");
#ifdef VK_KHR_ray_query //karin: vk
	if (!vk.khr_ray_query)
#endif
		Cvar_LockUnsupportedRendererCvar(&r_shadow_raytrace, "0");
	return true;
}
void VK_Shutdown(void)
{
	uint32_t i;

	VK_DestroySwapChain();

	for (i = 0; i < countof(postproc); i++)
		VKBE_RT_Gen(&postproc[i], NULL, 0, 0, false, RT_IMAGEFLAGS);
	VKBE_RT_Gen_Cube(&vk_rt_cubemap, 0, false);
	VK_R_BloomShutdown();

	if (vk.cmdpool)
		vkDestroyCommandPool(vk.device, vk.cmdpool, vkallocationcb);
	VK_DestroyRenderPasses();

	if (vk.pipelinecache)
	{
		size_t size;
		if (VK_SUCCESS == vkGetPipelineCacheData(vk.device, vk.pipelinecache, &size, NULL))
		{
			void *ptr = Z_Malloc(size);	//valgrind says nvidia isn't initialising this.
			if (VK_SUCCESS == vkGetPipelineCacheData(vk.device, vk.pipelinecache, &size, ptr))
				FS_WriteFile("vulkan.pcache", ptr, size, FS_ROOT);
			Z_Free(ptr);
		}
		vkDestroyPipelineCache(vk.device, vk.pipelinecache, vkallocationcb);
		vk.pipelinecache = VK_NULL_HANDLE;
	}

	while(vk.mempools)
	{
		void *l;
		vkFreeMemory(vk.device, vk.mempools->memory, vkallocationcb);
		l = vk.mempools;
		vk.mempools = vk.mempools->next;
		Z_Free(l);
	}

	if (vk.device)
		vkDestroyDevice(vk.device, vkallocationcb);
#ifdef VK_EXT_debug_utils
	if (vk_debugucallback)
	{
		vkDestroyDebugUtilsMessengerEXT(vk.instance, vk_debugucallback, vkallocationcb);
		vk_debugucallback = VK_NULL_HANDLE;
	}
#endif
#ifdef VK_EXT_debug_report
	if (vk_debugcallback)
	{
		vkDestroyDebugReportCallbackEXT(vk.instance, vk_debugcallback, vkallocationcb);
		vk_debugcallback = VK_NULL_HANDLE;
	}
#endif

	if (vk.surface)
		vkDestroySurfaceKHR(vk.instance, vk.surface, vkallocationcb);
	if (vk.instance)
		vkDestroyInstance(vk.instance, vkallocationcb);
#ifdef MULTITHREAD
	if (vk.submitcondition)
		Sys_DestroyConditional(vk.submitcondition);
#endif

	memset(&vk, 0, sizeof(vk));

#ifdef VK_NO_PROTOTYPES
	#define VKFunc(n) vk##n = NULL;
	VKFuncs
	#undef VKFunc
#endif

}
#endif
