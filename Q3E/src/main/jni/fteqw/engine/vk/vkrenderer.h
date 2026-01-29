//#include "glquake.h"

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define VK_USE_PLATFORM_WIN32_KHR
	#define VKInstWin32Funcs VKFunc(CreateWin32SurfaceKHR)
#elif defined(ANDROID)
	#define VK_USE_PLATFORM_ANDROID_KHR
	#define VKInstXLibFuncs VKFunc(CreateAndroidSurfaceKHR)
#elif defined(__linux__)
	#ifndef NO_X11
		#define VK_USE_PLATFORM_XLIB_KHR
		#define VKInstXLibFuncs VKFunc(CreateXlibSurfaceKHR)

		#define VK_USE_PLATFORM_XCB_KHR
		#define VKInstXCBFuncs VKFunc(CreateXcbSurfaceKHR)
	#endif
	#ifdef WAYLANDQUAKE
		#define VK_USE_PLATFORM_WAYLAND_KHR
		#define VKInstWaylandFuncs VKFunc(CreateWaylandSurfaceKHR)
	#endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#define VK_USE_PLATFORM_XLIB_KHR
	#define VKInstXLibFuncs VKFunc(CreateXlibSurfaceKHR)

	#define VK_USE_PLATFORM_XCB_KHR
	#define VKInstXCBFuncs VKFunc(CreateXcbSurfaceKHR)
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#if defined(_MSC_VER) && !defined(UINT64_MAX)
#define UINT64_MAX _UI64_MAX
#ifndef _UI64_MAX
#define _UI64_MAX 0xffffffffffffffffui64
#endif
#endif

#ifndef VKInstWin32Funcs
#define VKInstWin32Funcs
#endif
#ifndef VKInstXLibFuncs
#define VKInstXLibFuncs
#endif
#ifndef VKInstXCBFuncs
#define VKInstXCBFuncs
#endif
#ifndef VKInstWaylandFuncs
#define VKInstWaylandFuncs
#endif
#define VKInstArchFuncs VKInstWin32Funcs VKInstXLibFuncs VKInstXCBFuncs VKInstWaylandFuncs

#ifdef VK_EXT_debug_utils
#define VKDebugFuncs	\
	VKFunc(SetDebugUtilsObjectNameEXT)
#else
#define VKDebugFuncs
#endif

//funcs needed for creating an instance
#define VKInstFuncs \
	VKFunc(EnumerateInstanceLayerProperties)		\
	VKFunc(EnumerateInstanceExtensionProperties)	\
	VKFunc(EnumerateInstanceVersion)				\
	VKFunc(CreateInstance)

//funcs specific to an instance
#define VKInst2Funcs \
	VKFunc(EnumeratePhysicalDevices)				\
	VKFunc(EnumerateDeviceExtensionProperties)		\
	VKFunc(GetPhysicalDeviceProperties)				\
	VKFunc(GetPhysicalDeviceQueueFamilyProperties)	\
	VKFunc(GetPhysicalDeviceSurfaceSupportKHR)		\
	VKFunc(GetPhysicalDeviceSurfaceFormatsKHR)		\
	VKFunc(GetPhysicalDeviceSurfacePresentModesKHR)	\
	VKFunc(GetPhysicalDeviceSurfaceCapabilitiesKHR)	\
	VKFunc(GetPhysicalDeviceMemoryProperties)		\
	VKFunc(GetPhysicalDeviceFormatProperties)		\
	VKFunc(GetPhysicalDeviceFeatures)				\
	VKFunc(DestroySurfaceKHR)						\
	VKFunc(CreateDevice)							\
	VKFunc(DestroyInstance)							\
	VKDebugFuncs									\
	VKInstArchFuncs

//funcs specific to a device
#define VKDevFuncs \
	VKFunc(AcquireNextImageKHR)			\
	VKFunc(QueuePresentKHR)				\
	VKFunc(CreateSwapchainKHR)			\
	VKFunc(GetSwapchainImagesKHR)		\
	VKFunc(DestroySwapchainKHR)			\
	VKFunc(CmdBeginRenderPass)			\
	VKFunc(CmdEndRenderPass)			\
	VKFunc(CmdBindPipeline)				\
	VKFunc(CmdDrawIndexedIndirect)		\
	VKFunc(CmdDraw)						\
	VKFunc(CmdDrawIndexed)				\
	VKFunc(CmdSetViewport)				\
	VKFunc(CmdSetScissor)				\
	VKFunc(CmdBindDescriptorSets)		\
	VKFunc(CmdBindIndexBuffer)			\
	VKFunc(CmdBindVertexBuffers)		\
	VKFunc(CmdPushConstants)			\
	VKFunc(CmdPushDescriptorSetKHR)		\
	VKFunc(CmdClearAttachments)			\
	VKFunc(CmdClearColorImage)			\
	VKFunc(CmdClearDepthStencilImage)	\
	VKFunc(CmdCopyImage)				\
	VKFunc(CmdCopyBuffer)				\
	VKFunc(CmdCopyImageToBuffer)		\
	VKFunc(CmdCopyBufferToImage)		\
	VKFunc(CmdBlitImage)				\
	VKFunc(CmdPipelineBarrier)			\
	VKFunc(CmdSetEvent)					\
	VKFunc(CmdResetEvent)				\
	VKFunc(CmdWaitEvents)				\
	VKFunc(CreateDescriptorSetLayout)	\
	VKFunc(DestroyDescriptorSetLayout)	\
	VKFunc(CreatePipelineLayout)		\
	VKFunc(DestroyPipelineLayout)		\
	VKFunc(CreateShaderModule)			\
	VKFunc(DestroyShaderModule)			\
	VKFunc(CreateGraphicsPipelines)		\
	VKFunc(DestroyPipeline)				\
	VKFunc(CreatePipelineCache)			\
	VKFunc(GetPipelineCacheData)		\
	VKFunc(DestroyPipelineCache)		\
	VKFunc(QueueSubmit)					\
	VKFunc(QueueWaitIdle)				\
	VKFunc(DeviceWaitIdle)				\
	VKFunc(BeginCommandBuffer)			\
	VKFunc(ResetCommandBuffer)			\
	VKFunc(EndCommandBuffer)			\
	VKFunc(DestroyDevice)				\
	VKFunc(GetDeviceQueue)				\
	VKFunc(GetBufferMemoryRequirements)	\
	VKFunc(GetImageMemoryRequirements)	\
	VKFunc(GetImageMemoryRequirements2KHR)	\
	VKFunc(GetImageSubresourceLayout)	\
	VKFunc(CreateFramebuffer)			\
	VKFunc(DestroyFramebuffer)			\
	VKFunc(CreateCommandPool)			\
	VKFunc(ResetCommandPool)			\
	VKFunc(DestroyCommandPool)			\
	VKFunc(CreateDescriptorPool)		\
	VKFunc(ResetDescriptorPool)			\
	VKFunc(DestroyDescriptorPool)		\
	VKFunc(AllocateDescriptorSets)		\
	VKFunc(CreateSampler)				\
	VKFunc(DestroySampler)				\
	VKFunc(CreateImage)					\
	VKFunc(DestroyImage)				\
	VKFunc(CreateBuffer)				\
	VKFunc(DestroyBuffer)				\
	VKFunc(AllocateMemory)				\
	VKFunc(FreeMemory)					\
	VKFunc(BindBufferMemory)			\
	VKFunc(BindImageMemory)				\
	VKFunc(MapMemory)					\
	VKFunc(FlushMappedMemoryRanges)		\
	VKFunc(UnmapMemory)					\
	VKFunc(UpdateDescriptorSets)		\
	VKFunc(AllocateCommandBuffers)		\
	VKFunc(FreeCommandBuffers)			\
	VKFunc(CreateRenderPass)			\
	VKFunc(DestroyRenderPass)			\
	VKFunc(CreateSemaphore)				\
	VKFunc(DestroySemaphore)			\
	VKFunc(CreateFence)					\
	VKFunc(GetFenceStatus)				\
	VKFunc(WaitForFences)				\
	VKFunc(ResetFences)					\
	VKFunc(DestroyFence)				\
	VKFunc(CreateImageView)				\
	VKFunc(DestroyImageView)

//funcs for ray query's acceleration structures
#ifdef VK_KHR_acceleration_structure
#define VKAccelStructFuncs \
	VKFunc(GetBufferDeviceAddress)/*1.2*/				\
	VKFunc(GetAccelerationStructureBuildSizesKHR)		\
	VKFunc(CreateAccelerationStructureKHR)				\
	VKFunc(GetAccelerationStructureDeviceAddressKHR)	\
	VKFunc(DestroyAccelerationStructureKHR)				\
	VKFunc(CmdBuildAccelerationStructuresKHR)
#else
#define VKAccelStructFuncs
#endif

//all vulkan funcs
#define VKFuncs \
	VKInstFuncs		\
	VKInst2Funcs	\
	VKDevFuncs		\
	VKAccelStructFuncs \
	VKFunc(GetInstanceProcAddr)\
	VKFunc(GetDeviceProcAddr)


#ifdef VK_NO_PROTOTYPES
	#define VKFunc(n) extern PFN_vk##n vk##n;
	VKFuncs
	#undef VKFunc
#else
//	#define VKFunc(n) static const PFN_vk##n vk##n = vk##n;
//	VKFuncs
//	#undef VKFunc
#endif

#define vkallocationcb NULL
#ifdef _DEBUG
#define VkAssert(f) do {VkResult err = f; if (err) Sys_Error("%s == %s", #f, VK_VKErrorToString(err)); } while(0)
#define VkWarnAssert(f) do {VkResult err = f; if (err) Con_Printf("%s == %s\n", #f, VK_VKErrorToString(err)); } while(0)
#else
#define VkAssert(f) f
#define VkWarnAssert(f) f
#endif

typedef struct
{
	struct vk_mempool_s *pool;
	VkDeviceMemory memory;
	size_t size;
	size_t offset;
} vk_poolmem_t;

typedef struct vk_image_s
{
	VkImage image;
	vk_poolmem_t mem;
	VkImageView view;
	VkSampler sampler;
	VkImageLayout layout;

	VkFormat vkformat;
	uint32_t width;
	uint32_t height;
	uint32_t layers;
	uint32_t mipcount;
	uint32_t encoding;
	uint32_t type;	//PTI_2D/3D/CUBE
} vk_image_t;
enum dynbuf_e
{
	DB_VBO,
	DB_EBO,
	DB_UBO,
	DB_STAGING,
#ifdef VK_KHR_acceleration_structure
	DB_ACCELERATIONSTRUCT,
	DB_ACCELERATIONSCRATCH,
	DB_ACCELERATIONMESHDATA,
	DB_ACCELERATIONINSTANCE,
#endif
	DB_MAX
};
struct vk_rendertarg
{
	VkCommandBuffer cbuf;	//cbuf allocated for this render target.
	VkFramebuffer framebuffer;
	vk_image_t colour, depth, mscolour;

	image_t q_colour, q_depth, q_mscolour;	//extra sillyness...

	uint32_t width;
	uint32_t height;

	uint32_t rpassflags;
	qboolean depthcleared;	//starting a new gameview needs cleared depth relative to other views, but the first probably won't.

	VkRenderPassBeginInfo restartinfo;
	VkSemaphore presentsemaphore;
	qboolean firstuse;
	qboolean externalimage;

	struct vk_rendertarg *prevtarg;
};
struct vk_rendertarg_cube
{
	uint32_t size;
	image_t q_colour, q_depth;	//extra sillyness...
	vk_image_t colour, depth;
	struct vk_rendertarg face[6];
};

#define VQ_RENDER 0
#define VQ_PRESENT 1
#define VQ_ALTRENDER 2
#define VQ_ALTRENDER_COUNT 16
#define VQ_COUNT 3
extern struct vulkaninfo_s
{
	unsigned short	triplebuffer;
	qboolean		vsync;
	qboolean		allowsubmissionthread;

	qboolean		khr_swapchain;					//aka: not headless. we're actually rendering stuff!
	qboolean		khr_get_memory_requirements2;	//slightly richer info
	qboolean		khr_dedicated_allocation;		//standardised version of the above where the driver decides whether a resource is worth a dedicated allocation.
	qboolean		khr_push_descriptor;			//more efficient descriptor streaming
	qboolean		amd_rasterization_order;		//allows primitives to draw in any order
#ifdef VK_EXT_astc_decode_mode
	qboolean		ext_astc_decode_mode;			//small perf boost
#endif
#ifdef VK_KHR_fragment_shading_rate
	qboolean		khr_fragment_shading_rate;		//small perf boost. probably more useful for battery.
#endif
#ifdef VK_KHR_ray_query
	qboolean		khr_ray_query;
#endif
#ifdef VK_KHR_acceleration_structure
	qboolean		khr_acceleration_structure;
	qboolean		khr_deferred_host_operations;	//need to enable it, we don't make use of it though.
#endif

	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice gpu;
	uint32_t apiversion;	//the device api, capped by instance version, capped by our own version... sigh
	VkSurfaceKHR surface;
	uint32_t queuefam[VQ_COUNT];
	uint32_t queuenum[VQ_COUNT];
	VkQueue queue_render;
	VkQueue queue_present;
	VkQueue queue_alt[1];
	VkPhysicalDeviceMemoryProperties memory_properties;
	VkCommandPool cmdpool;
	VkPhysicalDeviceLimits limits;

	//we have a ringbuffer for acquires
#define ACQUIRELIMIT 8
	VkSemaphore acquiresemaphores[ACQUIRELIMIT];
	VkFence acquirefences[ACQUIRELIMIT];
	uint32_t acquirebufferidx[ACQUIRELIMIT];
	unsigned int acquirenext;			//first usable buffer, but we still need to wait on its fence (accessed on main thread).
	volatile unsigned int acquirelast;	//last buffer that we have successfully asked to aquire (set inside the submission thread).
	//acquirenext <= acquirelast, acquirelast-acquirenext<=ACQUIRELIMIT

	VkPipelineCache pipelinecache;

	struct vk_fencework 
	{
		VkFence fence;
		struct vk_fencework *next;
		void (*Passed) (void*);
		VkCommandBuffer cbuf;
	} *fencework, *fencework_last;	//callback for each fence as its passed. mostly for loading code or freeing memory.

	int filtermip[3];
	int filterpic[3];
	int mipcap[2];
	float lodbias;
	float max_anistophy;	//limits.maxSamplerAnistrophy

	struct vk_mempool_s
	{
		struct vk_mempool_s *next;

		uint32_t memtype;
		VkDeviceMemory memory;

		//FIXME: replace with an ordered list of free blocks.
		VkDeviceSize gaps;
		VkDeviceSize memoryoffset;
		VkDeviceSize memorysize;
	} *mempools;

	struct descpool
	{
		VkDescriptorPool pool;
		int availsets;
		int totalsets;
		struct descpool *next;
	} *descpool;
	struct dynbuffer
	{
		size_t flushed;	//size already copied to the gpu
		size_t offset;	//size written by the cpu (that might not yet be flushed)
		size_t size;	//maximum buffer size
		size_t align;
		qboolean stagingcoherent;
		VkBuffer stagingbuf;
		VkDeviceMemory stagingmemory;
		VkBuffer devicebuf;
		VkDeviceMemory devicememory;
		VkBuffer renderbuf;	//either staging or device. this is the buffer that we tell vulkan about
		void *ptr;

		struct dynbuffer *next;
	} *dynbuf[DB_MAX];
	struct vk_rendertarg *backbufs;
	struct vk_rendertarg *rendertarg;
	struct vkframe {
		struct vkframe *next;
		struct dynbuffer *dynbufs[DB_MAX];
		struct descpool *descpools;
		VkSemaphore acquiresemaphore;
		VkCommandBuffer *cbufs;
		size_t			numcbufs;
		size_t			maxcbufs;
		VkFence finishedfence;
		struct vk_frameend {
			struct vk_frameend *next;
			void (*FrameEnded) (void*);
		} *frameendjobs;

		struct vk_rendertarg *backbuf;
	} *frame, *unusedframes;
	struct vk_frameend *frameendjobs;
	uint32_t backbuf_count;

#define RP_RESUME		0
#define RP_DEPTHCLEAR	1	//
#define RP_FULLCLEAR	2
#define RP_DEPTHONLY	3	//shadowmaps (clears depth)
#define RP_MULTISAMPLE	(1u<<2)
#define RP_PRESENTABLE	(1u<<3)
#define RP_FP16			(1u<<4)
#define RP_VR			(1u<<5)	//potentially a different colour format.
	VkRenderPass renderpass[1u<<6];
	VkSwapchainKHR swapchain;
	uint32_t bufferidx;

	VkSampleCountFlagBits multisamplebits;
	VkFormat depthformat;
	VkFormat backbufformat;
	qboolean srgbcapable;

	qboolean neednewswapchain;	//something changed that invalidates the old one.
	qboolean devicelost;		//we seriously fucked up somewhere. or the gpu is shite.

	struct vksamplers_s
	{
		VkSampler samp;
		unsigned int usages;	//refcounted.
		unsigned int flags;
		VkSamplerCreateInfo props;
		struct vksamplers_s *next;
		struct vksamplers_s **link;
	} *samplers;

	struct vkwork_s
	{
		struct vkwork_s *next;
		VkQueue queue;
		VkCommandBuffer cmdbuf;
		VkSemaphore semwait;
		VkPipelineStageFlags semwaitstagemask;
		VkSemaphore semsignal;
		VkFence fencesignal;

		struct vk_fencework *fencedwork;
		struct vkframe *present;
	} *work;
	void *submitthread;
	void *submitcondition;
	void (*dopresent)(struct vkframe *theframe);

	texid_t sourcecolour;
	texid_t sourcedepth;

	shader_t *scenepp_waterwarp;
	shader_t *scenepp_antialias;
	shader_t *scenepp_rescale;
} vk;

struct pipeline_s
{
	struct pipeline_s *next;
	unsigned int permu:16;	//matches the permutation (masked by permutations that are supposed to be supported)
	unsigned int flags:16;	//matches the shader flags (cull etc)
	unsigned int blendbits; //matches blend state.
	VkPipeline pipeline;
};

uint32_t vk_find_memory_try(uint32_t typeBits, VkFlags requirements_mask);
uint32_t vk_find_memory_require(uint32_t typeBits, VkFlags requirements_mask);

void VK_DoPresent(struct vkframe *theframe);

qboolean VK_EnumerateDevices (void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc), const char *descprefix, PFN_vkGetInstanceProcAddr vk_GetInstanceProcAddr);
qboolean VK_Init(rendererstate_t *info, const char *const*sysextname, unsigned int numsysext, qboolean (*createSurface)(void), void (*dopresent)(struct vkframe *theframe));
void VK_Shutdown(void);

void VK_R_BloomBlend (texid_t source, int x, int y, int w, int h);
void VK_R_BloomShutdown(void);
qboolean R_CanBloom(void);

struct programshared_s;
struct programpermu_s;
qboolean VK_LoadGLSL(struct programshared_s *prog, struct programpermu_s *permu, int ver, const char **precompilerconstants, const char *vert, const char *tcs, const char *tes, const char *geom, const char *frag, qboolean noerrors, vfsfile_t *blobfile);

VkCommandBuffer VK_AllocFrameCBuf(void);
void VK_Submit_Work(VkCommandBuffer cmdbuf, VkSemaphore semwait, VkPipelineStageFlags semwaitstagemask, VkSemaphore semsignal, VkFence fencesignal, struct vkframe *presentframe, struct vk_fencework *fencedwork);

void VKBE_Init(void);
void VKBE_InitFramePools(struct vkframe *frame);
void VKBE_RestartFrame(void);
void VKBE_FlushDynamicBuffers(void);
void VKBE_Set2D(qboolean twodee);
void VKBE_ShutdownFramePools(struct vkframe *frame);
void VKBE_Shutdown(void);
void VKBE_SelectMode(backendmode_t mode);
void VKBE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **mesh, vbo_t *vbo, texnums_t *texnums, unsigned int beflags);
void VKBE_DrawMesh_Single(shader_t *shader, mesh_t *meshchain, vbo_t *vbo, unsigned int beflags);
void VKBE_SubmitBatch(batch_t *batch);
batch_t *VKBE_GetTempBatch(void);
void VKBE_GenBrushModelVBO(model_t *mod);
void VKBE_ClearVBO(vbo_t *vbo, qboolean dataonly);
void VKBE_UploadAllLightmaps(void);
void VKBE_DrawWorld (batch_t **worldbatches);
qboolean VKBE_LightCullModel(vec3_t org, model_t *model);
void VKBE_SelectEntity(entity_t *ent);
qboolean VKBE_SelectDLight(dlight_t *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode);
void VKBE_VBO_Begin(vbobctx_t *ctx, size_t maxsize);
void VKBE_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray);
void VKBE_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem);
void VKBE_VBO_Destroy(vboarray_t *vearray, void *mem);
void VKBE_Scissor(srect_t *rect);
void VKBE_BaseEntTextures(const qbyte *scenepvs, const int *sceneareas);

struct vk_shadowbuffer;
struct vk_shadowbuffer *VKBE_GenerateShadowBuffer(vecV_t *verts, int numverts, index_t *indicies, int numindicies, qboolean istemp);
void VKBE_DestroyShadowBuffer(struct vk_shadowbuffer *buf);
void VKBE_RenderShadowBuffer(struct vk_shadowbuffer *buf);
void VKBE_SetupForShadowMap(dlight_t *dl, int texwidth, int texheight, float shadowscale);
qboolean VKBE_BeginShadowmap(qboolean isspot, uint32_t width, uint32_t height);
void VKBE_BeginShadowmapFace(void);
void VKBE_DoneShadows(void);

void VKBE_RT_Gen_Cube(struct vk_rendertarg_cube *targ, uint32_t size, qboolean clear);
void VKBE_RT_Gen(struct vk_rendertarg *targ, vk_image_t *colour, uint32_t width, uint32_t height, qboolean clear, unsigned int flags);
void VKBE_RT_Begin(struct vk_rendertarg *targ);
void VKBE_RT_End(struct vk_rendertarg *targ);
void VKBE_RT_Destroy(struct vk_rendertarg *targ);

char *VK_VKErrorToString(VkResult err);	//helper for converting vulkan error codes to strings, if we get something unexpected.

qboolean VK_AllocatePoolMemory(uint32_t pooltype, VkDeviceSize memsize, VkDeviceSize poolalignment, vk_poolmem_t *mem);
void VK_ReleasePoolMemory(vk_poolmem_t *mem);
qboolean VK_AllocateImageMemory(VkImage image, qboolean dedicated, vk_poolmem_t *mem);	//dedicated should normally be TRUE for render targets
qboolean VK_AllocateBindImageMemory(vk_image_t *image, qboolean dedicated);	//dedicated should normally be TRUE for render targets
struct stagingbuf
{
	VkBuffer buf;
	VkBuffer retbuf;
	vk_poolmem_t mem;
	size_t size;
	VkBufferUsageFlags usage;
};
vk_image_t VK_CreateTexture2DArray(uint32_t width, uint32_t height, uint32_t layers, uint32_t mips, uploadfmt_t encoding, unsigned int type, qboolean rendertarget, const char *debugname);
void set_image_layout(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkAccessFlags srcaccess, VkPipelineStageFlagBits srcstagemask, VkImageLayout new_image_layout, VkAccessFlags dstaccess, VkPipelineStageFlagBits dststagemask);
void VK_CreateSampler(unsigned int flags, vk_image_t *img);
void VK_CreateSamplerInfo(VkSamplerCreateInfo *info, vk_image_t *img);
void *VKBE_CreateStagingBuffer(struct stagingbuf *n, size_t size, VkBufferUsageFlags usage);
VkBuffer VKBE_FinishStaging(struct stagingbuf *n, vk_poolmem_t *memptr);
void *VK_FencedBegin(void (*passed)(void *work), size_t worksize);
void VK_FencedSubmit(void *work);
void VK_FencedCheck(void);
void *VK_AtFrameEnd(void (*passed)(void *work), void *data, size_t worksize);



void	VK_Draw_Init(void);
void	VK_Draw_Shutdown(void);

void	VK_UpdateFiltering			(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis);
qboolean VK_LoadTextureMips			(texid_t tex, const struct pendingtextureinfo *mips);
void    VK_DestroyTexture			(texid_t tex);
void	VK_DestroyVkTexture			(vk_image_t *img);

void	VK_R_Init					(void);
void	VK_R_DeInit					(void);
void	VK_R_RenderView				(void);

char	*VKVID_GetRGBInfo			(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt);

qboolean	VK_SCR_UpdateScreen			(void);

void	VKBE_RenderToTextureUpdate2d(qboolean destchanged);
VkRenderPass VK_GetRenderPass(int pass);

//improved rgb get that calls the callback when the data is actually available. used for video capture.
void VKVID_QueueGetRGBData			(void (*gotrgbdata) (void *rgbdata, qintptr_t bytestride, size_t width, size_t height, enum uploadfmt fmt));
