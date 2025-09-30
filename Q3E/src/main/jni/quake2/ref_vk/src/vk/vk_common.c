/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file implements the operating system binding of Vk to QVk function
 * pointers.  When doing a port of Quake2 you must implement the following
 * two functions:
 *
 * QVk_Init() - loads libraries, assigns function pointers, etc.
 * QVk_Shutdown() - unloads libraries, NULLs function pointers
 *
 * =======================================================================
 *
 */

#include <float.h>
#include "header/local.h"

#ifdef __ANDROID__ //karin: ANativeWindow
static ANativeWindow *vk_window;
#else
static SDL_Window *vk_window;
#endif

// Vulkan instance, surface and memory allocator
VkInstance vk_instance  = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

// Vulkan device
qvkdevice_t vk_device = {
	.physical = VK_NULL_HANDLE,
	.logical = VK_NULL_HANDLE,
	.gfxQueue = VK_NULL_HANDLE,
	.presentQueue = VK_NULL_HANDLE,
	.transferQueue = VK_NULL_HANDLE,
	.gfxFamilyIndex = -1,
	.presentFamilyIndex = -1,
	.transferFamilyIndex = -1,
	.screenshotSupported = false
};

// Vulkan swapchain
qvkswapchain_t vk_swapchain = {
	.sc = VK_NULL_HANDLE,
	.format = VK_FORMAT_UNDEFINED,
	.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
	.extent = { 0, 0 },
	.images = NULL,
	.imageCount = 0
};

// Vulkan renderpasses
qvkrenderpass_t vk_renderpasses[RP_COUNT] = {
	// RP_WORLD
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	},
	// RP_UI
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	},
	// RP_WORLD_WARP
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	}
};

// Vulkan pools
VkCommandPool vk_commandPool[NUM_CMDBUFFERS] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
VkCommandPool vk_transferCommandPool = VK_NULL_HANDLE;
VkDescriptorPool vk_descriptorPool = VK_NULL_HANDLE;
static VkCommandPool vk_stagingCommandPool[NUM_DYNBUFFERS] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
// Vulkan image views
static VkImageView *vk_imageviews = NULL;
// Vulkan framebuffers
static VkFramebuffer *vk_framebuffers[RP_COUNT];
// color buffer containing main game/world view
qvktexture_t vk_colorbuffer = QVVKTEXTURE_INIT;
// color buffer with postprocessed game view
qvktexture_t vk_colorbufferWarp = QVVKTEXTURE_INIT;
// depth buffer
qvktexture_t vk_depthbuffer = QVVKTEXTURE_INIT;
// depth buffer for UI renderpass
static qvktexture_t vk_ui_depthbuffer = QVVKTEXTURE_INIT;
// render target for MSAA resolve
static qvktexture_t vk_msaaColorbuffer = QVVKTEXTURE_INIT;
// viewport and scissor
VkViewport vk_viewport = { .0f, .0f, .0f, .0f, .0f, .0f };
VkRect2D vk_scissor = { { 0, 0 }, { 0, 0 } };

// Vulkan command buffers
static VkCommandBuffer *vk_commandbuffers = NULL;
// command buffer double buffering fences
static VkFence vk_fences[NUM_CMDBUFFERS];
// semaphore: signal when next image is available for rendering
static VkSemaphore vk_imageAvailableSemaphores[NUM_CMDBUFFERS];
// semaphore: signal when rendering to current command buffer is complete
static VkSemaphore vk_renderFinishedSemaphores[NUM_CMDBUFFERS];
// tracker variables
VkCommandBuffer vk_activeCmdbuffer = VK_NULL_HANDLE;
// index of active command buffer
int vk_activeBufferIdx = 0;
// index of currently acquired image
static uint32_t vk_imageIndex = 0;
// index of currently used staging buffer
static int vk_activeStagingBuffer = 0;
// started rendering frame?
qboolean vk_frameStarted = false;
// the swap chain needs to be rebuilt.
qboolean vk_recreateSwapchainNeeded = false;
// is QVk initialized?
qboolean vk_initialized = false;

// render pipelines
qvkpipeline_t vk_drawTexQuadPipeline[RP_COUNT]    = {
	QVKPIPELINE_INIT, QVKPIPELINE_INIT, QVKPIPELINE_INIT };
qvkpipeline_t vk_drawColorQuadPipeline[RP_COUNT]  = {
	QVKPIPELINE_INIT, QVKPIPELINE_INIT, QVKPIPELINE_INIT };
qvkpipeline_t vk_drawModelPipelineFan[RP_COUNT]   = {
	QVKPIPELINE_INIT, QVKPIPELINE_INIT, QVKPIPELINE_INIT };
qvkpipeline_t vk_drawNoDepthModelPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawLefthandModelPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawNullModelPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawParticlesPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPointParticlesPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawSpritePipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyLmapPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyWarpPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolySolidWarpPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawBeamPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawSkyboxPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawDLightPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_showTrisPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_shadowsPipelineStrip = QVKPIPELINE_INIT;
qvkpipeline_t vk_shadowsPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_worldWarpPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_postprocessPipeline = QVKPIPELINE_INIT;

// samplers
static VkSampler vk_samplers[NUM_SAMPLERS];

// Vulkan function pointers
PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;
PFN_vkSetDebugUtilsObjectNameEXT qvkSetDebugUtilsObjectNameEXT;
PFN_vkSetDebugUtilsObjectTagEXT qvkSetDebugUtilsObjectTagEXT;
PFN_vkCmdBeginDebugUtilsLabelEXT qvkCmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT qvkCmdEndDebugUtilsLabelEXT;
PFN_vkCmdInsertDebugUtilsLabelEXT qvkInsertDebugUtilsLabelEXT;
PFN_vkCreateDebugReportCallbackEXT qvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT qvkDestroyDebugReportCallbackEXT;

#define VK_INPUTBIND_DESC(s) { \
	.binding = 0, \
	.stride = s, \
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX \
};

#define VK_INPUTATTR_DESC(l, f, o) { \
	.binding = 0, \
	.location = l, \
	.format = f, \
	.offset = o \
}

#define VK_VERTEXINPUT_CINF(b, a) { \
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, \
	.pNext = NULL, \
	.flags = 0, \
	.vertexBindingDescriptionCount = 1, \
	.pVertexBindingDescriptions = &b, \
	.vertexAttributeDescriptionCount = sizeof(a) / sizeof(a[0]), \
	.pVertexAttributeDescriptions = a \
}

#define VK_VERTINFO(name, bindSize, ...) \
	VkVertexInputAttributeDescription attrDesc##name[] = { __VA_ARGS__ }; \
	VkVertexInputBindingDescription name##bindingDesc = VK_INPUTBIND_DESC(bindSize); \
	VkPipelineVertexInputStateCreateInfo vertInfo##name = VK_VERTEXINPUT_CINF(name##bindingDesc, attrDesc##name);

#define VK_NULL_VERTEXINPUT_CINF { \
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, \
	.pNext = NULL, \
	.flags = 0, \
	.vertexBindingDescriptionCount = 0, \
	.pVertexBindingDescriptions = NULL, \
	.vertexAttributeDescriptionCount = 0, \
	.pVertexAttributeDescriptions = NULL \
}

enum {
	SHADER_VERT_INDEX = 0,
	SHADER_FRAG_INDEX = 1,
	SHADER_INDEX_SIZE = 2
};

#define VK_LOAD_VERTFRAG_SHADERS(shaders, namevert, namefrag) \
	DestroyShaderModule(shaders); \
	shaders[SHADER_VERT_INDEX] = QVk_CreateShader(namevert##_vert_spv, namevert##_vert_size, VK_SHADER_STAGE_VERTEX_BIT); \
	shaders[SHADER_FRAG_INDEX] = QVk_CreateShader(namefrag##_frag_spv, namefrag##_frag_size, VK_SHADER_STAGE_FRAGMENT_BIT); \
	QVk_DebugSetObjectName((uint64_t)shaders[SHADER_VERT_INDEX].module, VK_OBJECT_TYPE_SHADER_MODULE, "Shader Module: "#namevert".vert"); \
	QVk_DebugSetObjectName((uint64_t)shaders[SHADER_FRAG_INDEX].module, VK_OBJECT_TYPE_SHADER_MODULE, "Shader Module: "#namefrag".frag");

// global static buffers (reused, never changing)
static qvkbuffer_t vk_texRectVbo;
static qvkbuffer_t vk_colorRectVbo;
static qvkbuffer_t vk_rectIbo;

// global dynamic buffers (double buffered)
static qvkbuffer_t vk_dynVertexBuffers[NUM_DYNBUFFERS];
static qvkbuffer_t vk_dynIndexBuffers[NUM_DYNBUFFERS];
static qvkbuffer_t vk_dynUniformBuffers[NUM_DYNBUFFERS];
static VkDescriptorSet vk_uboDescriptorSets[NUM_DYNBUFFERS];
static qvkstagingbuffer_t vk_stagingBuffers[NUM_DYNBUFFERS];
static int vk_activeDynBufferIdx = 0;
static int vk_activeSwapBufferIdx = 0;

// index buffer for triangle fan/strip emulation - all because Metal/MoltenVK don't support them
static VkBuffer *vk_triangleFanIbo = NULL;
static VkBuffer *vk_triangleStripIbo = NULL;
static uint32_t  vk_triangleFanIboUsage = 0;

// swap buffers used if primary dynamic buffers get full
#define NUM_SWAPBUFFER_SLOTS 4
static int vk_swapBuffersCnt[NUM_SWAPBUFFER_SLOTS];
static int vk_swapDescSetsCnt[NUM_SWAPBUFFER_SLOTS];
static qvkbuffer_t *vk_swapBuffers[NUM_SWAPBUFFER_SLOTS];
static VkDescriptorSet *vk_swapDescriptorSets[NUM_SWAPBUFFER_SLOTS];

// by how much will the dynamic buffers be resized if we run out of space?
#define BUFFER_RESIZE_FACTOR 2.f
// size in bytes used for uniform descriptor update
#define UNIFORM_ALLOC_SIZE 1024
// start values for dynamic buffer sizes - bound to change if the application runs out of space (sizes in bytes)
#define VERTEX_BUFFER_SIZE (1024 * 1024)
#define INDEX_BUFFER_SIZE (2 * 1024)
#define UNIFORM_BUFFER_SIZE (2048 * 1024)
// staging buffer is constant in size but has a max limit beyond which it will be submitted
#define STAGING_BUFFER_MAXSIZE (8192 * 1024)
// initial index count in triangle fan buffer - assuming 200 indices (200*3 = 600 triangles) per object
#define TRIANGLE_INDEX_CNT 200

// Vulkan common descriptor sets for UBO, primary texture sampler and optional lightmap texture
static VkDescriptorSetLayout vk_uboDescSetLayout;
VkDescriptorSetLayout vk_samplerDescSetLayout;
static VkDescriptorSetLayout vk_samplerLightmapDescSetLayout;

static const char *renderpassObjectNames[] = {
	"RP_WORLD",
	"RP_UI",
	"RP_WORLD_WARP"
};

VkFormat QVk_FindDepthFormat()
{
	VkFormat depthFormats[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (int i = 0; i < 5; ++i)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(vk_device.physical, depthFormats[i], &formatProps);

		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			return depthFormats[i];
	}

	return VK_FORMAT_D16_UNORM;
}

static const VkSampleCountFlagBits msaaModes[] = {
	VK_SAMPLE_COUNT_64_BIT,
	VK_SAMPLE_COUNT_32_BIT,
	VK_SAMPLE_COUNT_16_BIT,
	VK_SAMPLE_COUNT_8_BIT,
	VK_SAMPLE_COUNT_4_BIT,
	VK_SAMPLE_COUNT_2_BIT,
	VK_SAMPLE_COUNT_1_BIT
};

// internal helper
static VkSampleCountFlagBits GetSampleCount(int msaa, VkSampleCountFlagBits supportedMsaa)
{
	int step = 0, value = 64;

	while ((msaa < value && value > 1) ||
		   ((supportedMsaa & msaaModes[step]) != msaaModes[step]))
	{
		value >>= 1;
		step ++;
	}

	R_Printf(PRINT_ALL, "...MSAAx%d is used\n", value);

	return msaaModes[step];
}

// internal helper
static void DestroyImageViews()
{
	if(!vk_imageviews)
		return;

	for (int i = 0; i < vk_swapchain.imageCount; i++)
	{
		vkDestroyImageView(vk_device.logical, vk_imageviews[i], NULL);
	}
	free(vk_imageviews);
	vk_imageviews = NULL;
}

// internal helper
static VkResult CreateImageViews()
{
	VkResult res = VK_SUCCESS;
	vk_imageviews = (VkImageView *)malloc(vk_swapchain.imageCount * sizeof(VkImageView));

	for (size_t i = 0; i < vk_swapchain.imageCount; ++i)
	{
		res = QVk_CreateImageView(&vk_swapchain.images[i],
			VK_IMAGE_ASPECT_COLOR_BIT, &vk_imageviews[i], vk_swapchain.format, 1);
		QVk_DebugSetObjectName((uint64_t)vk_swapchain.images[i],
			VK_OBJECT_TYPE_IMAGE, va("Swap Chain Image #" YQ2_COM_PRIdS, i));
		QVk_DebugSetObjectName((uint64_t)vk_imageviews[i],
			VK_OBJECT_TYPE_IMAGE_VIEW, va("Swap Chain Image View #" YQ2_COM_PRIdS, i));

		if (res != VK_SUCCESS)
		{
			DestroyImageViews();
			return res;
		}
	}

	return res;
}

// internal helper
static void DestroyFramebuffers()
{
	for (int f = 0; f < RP_COUNT; f++)
	{
		if (vk_framebuffers[f])
		{
			for (int i = 0; i < vk_swapchain.imageCount; ++i)
			{
				vkDestroyFramebuffer(vk_device.logical, vk_framebuffers[f][i], NULL);
			}

			free(vk_framebuffers[f]);
			vk_framebuffers[f] = NULL;
		}
	}
}

// internal helper
static VkResult CreateFramebuffers()
{
	for(int i = 0; i < RP_COUNT; ++i)
		vk_framebuffers[i] = (VkFramebuffer *)malloc(vk_swapchain.imageCount * sizeof(VkFramebuffer));

	VkFramebufferCreateInfo fbCreateInfos[] = {
		// RP_WORLD: main world view framebuffer
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = vk_renderpasses[RP_WORLD].rp,
			.attachmentCount = (vk_renderpasses[RP_WORLD].sampleCount != VK_SAMPLE_COUNT_1_BIT) ? 3 : 2,
			.width = vk_swapchain.extent.width,
			.height = vk_swapchain.extent.height,
			.layers = 1
		},
		// RP_UI: UI framebuffer
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = vk_renderpasses[RP_UI].rp,
			.attachmentCount = 3,
			.width = vk_swapchain.extent.width,
			.height = vk_swapchain.extent.height,
			.layers = 1
		},
		// RP_WORLD_WARP: warped main world view (postprocessing) framebuffer
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = vk_renderpasses[RP_WORLD_WARP].rp,
			.attachmentCount = 2,
			.width = vk_swapchain.extent.width,
			.height = vk_swapchain.extent.height,
			.layers = 1
		}
	};

	VkImageView worldAttachments[] = { vk_colorbuffer.imageView, vk_depthbuffer.imageView, vk_msaaColorbuffer.imageView };
	VkImageView warpAttachments[]  = { vk_colorbuffer.imageView, vk_colorbufferWarp.imageView };

	fbCreateInfos[RP_WORLD].pAttachments = worldAttachments;
	fbCreateInfos[RP_WORLD_WARP].pAttachments = warpAttachments;

	for (size_t i = 0; i < vk_swapchain.imageCount; ++i)
	{
		VkImageView uiAttachments[] = { vk_colorbufferWarp.imageView, vk_ui_depthbuffer.imageView, vk_imageviews[i] };
		fbCreateInfos[RP_UI].pAttachments = uiAttachments;

		for (int j = 0; j < RP_COUNT; ++j)
		{
			VkResult res = vkCreateFramebuffer(vk_device.logical, &fbCreateInfos[j], NULL, &vk_framebuffers[j][i]);
			QVk_DebugSetObjectName((uint64_t)vk_framebuffers[j][i],
				VK_OBJECT_TYPE_FRAMEBUFFER, va("Framebuffer #" YQ2_COM_PRIdS "for Render Pass %s",
					i, renderpassObjectNames[j]));

			if (res != VK_SUCCESS)
			{
				R_Printf(PRINT_ALL, "%s(): framebuffer #%d create error: %s\n", __func__, j, QVk_GetError(res));
				DestroyFramebuffers();
				return res;
			}
		}
	}

	return VK_SUCCESS;
}

// internal helper
static VkResult CreateRenderpasses()
{
	qboolean msaaEnabled = vk_renderpasses[RP_WORLD].sampleCount != VK_SAMPLE_COUNT_1_BIT;

	/*
	 * world view setup
	 */
	// The color attachment is loaded from the previous frame and stored
	// after the frame is drawn to mask geometry errors in the skybox
	// that may leave some pixels without coverage.
	VkAttachmentDescription worldAttachments[] = {
		// Single-sample color attachment.
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = (msaaEnabled ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD),
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		},
		// depth attachment
		{
			.flags = 0,
			.format = QVk_FindDepthFormat(),
			.samples = vk_renderpasses[RP_WORLD].sampleCount,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		// MSAA attachment
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = vk_renderpasses[RP_WORLD].sampleCount,
			.loadOp = (msaaEnabled ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD),
			.storeOp = (msaaEnabled ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE),
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference worldAttachmentRefs[] = {
		// color
		{
			.attachment = msaaEnabled ? 2 : 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		// depth
		{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		// MSAA resolve
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	};

	// primary renderpass writes to color, depth and optional MSAA resolve
	VkSubpassDescription worldSubpassDesc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &worldAttachmentRefs[0],
		.pResolveAttachments = msaaEnabled ? &worldAttachmentRefs[2] : NULL,
		.pDepthStencilAttachment = &worldAttachmentRefs[1],
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	/*
	 * world warp setup
	 */
	VkAttachmentDescription warpAttachments[] = {
		// color attachment - input from RP_WORLD renderpass
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		},
		// color attachment output - warped/postprocessed image that ends up in RP_UI
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		}
	};

	VkAttachmentReference warpAttachmentRef = {
		// output color
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	// world view postprocess writes to a separate color buffer
	VkSubpassDescription warpSubpassDesc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &warpAttachmentRef,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	/*
	 * UI setup
	 */
	VkAttachmentDescription uiAttachments[] = {
		// color attachment
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		},
		// depth attachment - because of player model preview in settings screen
		{
			.flags = 0,
			.format = QVk_FindDepthFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		// swapchain presentation
		{
			.flags = 0,
			.format = vk_swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	// UI renderpass writes to depth (for player model in setup screen) and outputs to swapchain
	VkAttachmentReference uiAttachmentRefs[] = {
		// depth
		{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		// swapchain output
		{
			.attachment = 2,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	};

	VkSubpassDescription uiSubpassDesc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &uiAttachmentRefs[1],
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = &uiAttachmentRefs[0],
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	/*
	 * create the render passes
	 */
	// we're using 3 render passes which depend on each other (main color -> warp/postprocessing -> ui)
	VkSubpassDependency subpassDeps[2] = {
		{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dstAccessMask = (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT),
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		},
		{
		.srcSubpass = 0,
		.dstSubpass = VK_SUBPASS_EXTERNAL,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
	};

	VkRenderPassCreateInfo rpCreateInfos[] = {
		// offscreen world rendering to color buffer (RP_WORLD)
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.attachmentCount = msaaEnabled ? 3 : 2,
			.pAttachments = worldAttachments,
			.subpassCount = 1,
			.pSubpasses = &worldSubpassDesc,
			.dependencyCount = 2,
			.pDependencies = subpassDeps
		},
		// UI rendering (RP_UI)
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.attachmentCount = 3,
			.pAttachments = uiAttachments,
			.subpassCount = 1,
			.pSubpasses = &uiSubpassDesc,
			.dependencyCount = 2,
			.pDependencies = subpassDeps
		},
		// world warp/postprocessing render pass (RP_WORLD_WARP)
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.attachmentCount = 2,
			.pAttachments = warpAttachments,
			.subpassCount = 1,
			.pSubpasses = &warpSubpassDesc,
			.dependencyCount = 2,
			.pDependencies = subpassDeps
		}
	};

	for (int i = 0; i < RP_COUNT; ++i)
	{
		VkResult res = vkCreateRenderPass(vk_device.logical, &rpCreateInfos[i], NULL, &vk_renderpasses[i].rp);
		if (res != VK_SUCCESS)
		{
			R_Printf(PRINT_ALL, "%s(): renderpass #%d create error: %s\n", __func__, i, QVk_GetError(res));
			return res;
		}
		QVk_DebugSetObjectName((uint64_t)vk_renderpasses[i].rp, VK_OBJECT_TYPE_RENDER_PASS,
			va("Render Pass: %s",  renderpassObjectNames[i]));
	}

	return VK_SUCCESS;
}

// internal helper
static void CreateDrawBuffers()
{
	QVk_CreateDepthBuffer(vk_renderpasses[RP_WORLD].sampleCount,
		&vk_depthbuffer);
	R_Printf(PRINT_ALL, "...created world depth buffer\n");
	QVk_CreateDepthBuffer(VK_SAMPLE_COUNT_1_BIT, &vk_ui_depthbuffer);
	R_Printf(PRINT_ALL, "...created UI depth buffer\n");
	QVk_CreateColorBuffer(VK_SAMPLE_COUNT_1_BIT, &vk_colorbuffer,
		VK_IMAGE_USAGE_SAMPLED_BIT);
	R_Printf(PRINT_ALL, "...created world color buffer\n");
	QVk_CreateColorBuffer(VK_SAMPLE_COUNT_1_BIT, &vk_colorbufferWarp,
		VK_IMAGE_USAGE_SAMPLED_BIT);
	R_Printf(PRINT_ALL, "...created world postpocess color buffer\n");

	if (vk_renderpasses[RP_WORLD].sampleCount > 1)
	{
		QVk_CreateColorBuffer(vk_renderpasses[RP_WORLD].sampleCount, &vk_msaaColorbuffer,
			VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		R_Printf(PRINT_ALL, "...created MSAAx%d color buffer\n",
			vk_renderpasses[RP_WORLD].sampleCount);
	}

	QVk_DebugSetObjectName((uint64_t)vk_depthbuffer.resource.image,
		VK_OBJECT_TYPE_IMAGE, "Depth Buffer: World");
	QVk_DebugSetObjectName((uint64_t)vk_depthbuffer.imageView,
		VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: World Depth Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_depthbuffer.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: World Depth Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_ui_depthbuffer.resource.image,
		VK_OBJECT_TYPE_IMAGE, "Depth Buffer: UI");
	QVk_DebugSetObjectName((uint64_t)vk_ui_depthbuffer.imageView,
		VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: UI Depth Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_ui_depthbuffer.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: UI Depth Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_colorbuffer.resource.image,
		VK_OBJECT_TYPE_IMAGE, "Color Buffer: World");
	QVk_DebugSetObjectName((uint64_t)vk_colorbuffer.imageView,
		VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: World Color Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_colorbuffer.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: World Color Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_colorbufferWarp.resource.image,
		VK_OBJECT_TYPE_IMAGE, "Color Buffer: Warp Postprocess");
	QVk_DebugSetObjectName((uint64_t)vk_colorbufferWarp.imageView,
		VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: Warp Postprocess Color Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_colorbufferWarp.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: Warp Postprocess Color Buffer");

	if (vk_renderpasses[RP_WORLD].sampleCount > 1)
	{
		QVk_DebugSetObjectName((uint64_t)vk_msaaColorbuffer.resource.image,
			VK_OBJECT_TYPE_IMAGE, "Color Buffer: MSAA");
		QVk_DebugSetObjectName((uint64_t)vk_msaaColorbuffer.imageView,
			VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: MSAA Color Buffer");
		QVk_DebugSetObjectName((uint64_t)vk_msaaColorbuffer.resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: MSAA Color Buffer");
	}
}

// internal helper
static void DestroyDrawBuffer(qvktexture_t *drawBuffer)
{
	if (drawBuffer->imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk_device.logical, drawBuffer->imageView, NULL);
		drawBuffer->imageView = VK_NULL_HANDLE;
	}

	if (drawBuffer->resource.image != VK_NULL_HANDLE)
	{
		image_destroy(&drawBuffer->resource);
	}
}

// internal helper
static void DestroyDrawBuffers()
{
	DestroyDrawBuffer(&vk_depthbuffer);
	DestroyDrawBuffer(&vk_ui_depthbuffer);
	DestroyDrawBuffer(&vk_colorbuffer);
	DestroyDrawBuffer(&vk_colorbufferWarp);
	DestroyDrawBuffer(&vk_msaaColorbuffer);
}

// internal helper
static void CreateDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &layoutBinding
	};

	// uniform buffer object layout
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_uboDescSetLayout));
	// sampler layout
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_samplerDescSetLayout));
	// secondary sampler: lightmaps
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_samplerLightmapDescSetLayout));

	QVk_DebugSetObjectName((uint64_t)vk_uboDescSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "Descriptor Set Layout: UBO");
	QVk_DebugSetObjectName((uint64_t)vk_samplerDescSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "Descriptor Set Layout: Sampler");
	QVk_DebugSetObjectName((uint64_t)vk_samplerLightmapDescSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "Descriptor Set Layout: Sampler + Lightmap");
}

// internal helper
static void CreateSamplersHelper(VkSampler *samplers, VkSamplerAddressMode addressMode)
{
	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1.f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = 1.f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	assert((vk_device.properties.limits.maxSamplerAnisotropy > 1.f) && "maxSamplerAnisotropy is 1");
	if (vk_device.features.samplerAnisotropy && vk_aniso->value > 0.f)
	{
		const float maxAniso = Q_min(Q_max(vk_aniso->value, 1.f),
			vk_device.properties.limits.maxSamplerAnisotropy);
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = maxAniso;
	}

	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &samplers[S_NEAREST]));
	QVk_DebugSetObjectName((uint64_t)samplers[S_NEAREST], VK_OBJECT_TYPE_SAMPLER, "Sampler: S_NEAREST");

	{
		VkSamplerCreateInfo nuSamplerInfo = samplerInfo;

		// unnormalizedCoordinates set to VK_TRUE forces other parameters to have restricted values.
		nuSamplerInfo.minLod = 0.f;
		nuSamplerInfo.maxLod = 0.f;
		nuSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		nuSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		nuSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		nuSamplerInfo.unnormalizedCoordinates = VK_TRUE;
		nuSamplerInfo.anisotropyEnable = VK_FALSE;
		nuSamplerInfo.maxAnisotropy = 1.f;

		VK_VERIFY(vkCreateSampler(vk_device.logical, &nuSamplerInfo, NULL, &samplers[S_NEAREST_UNNORMALIZED]));
		QVk_DebugSetObjectName((uint64_t)samplers[S_NEAREST_UNNORMALIZED], VK_OBJECT_TYPE_SAMPLER, "Sampler: S_NEAREST_UNNORMALIZED");
	}

	samplerInfo.maxLod = FLT_MAX;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &samplers[S_MIPMAP_NEAREST]));
	QVk_DebugSetObjectName((uint64_t)samplers[S_MIPMAP_NEAREST], VK_OBJECT_TYPE_SAMPLER, "Sampler: S_MIPMAP_NEAREST");

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &samplers[S_MIPMAP_LINEAR]));
	QVk_DebugSetObjectName((uint64_t)samplers[S_MIPMAP_LINEAR], VK_OBJECT_TYPE_SAMPLER, "Sampler: S_MIPMAP_LINEAR");

	samplerInfo.maxLod = 1.f;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &samplers[S_LINEAR]));
	QVk_DebugSetObjectName((uint64_t)samplers[S_LINEAR], VK_OBJECT_TYPE_SAMPLER, "Sampler: S_LINEAR");
}

// internal helper
static void CreateSamplers()
{
	CreateSamplersHelper(vk_samplers, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	CreateSamplersHelper(vk_samplers + S_SAMPLER_CNT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

// internal helper
static void DestroySamplers()
{
	int i;
	for (i = 0; i < NUM_SAMPLERS; ++i)
	{
		if (vk_samplers[i] != VK_NULL_HANDLE)
			vkDestroySampler(vk_device.logical, vk_samplers[i], NULL);

		vk_samplers[i] = VK_NULL_HANDLE;
	}
}

// internal helper
static void CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		// UBO
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 16
		},
		// sampler
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_VKTEXTURES + 1
		}
	};

	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = MAX_VKTEXTURES + 32,
		.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
		.pPoolSizes = poolSizes,
	};

	VK_VERIFY(vkCreateDescriptorPool(vk_device.logical, &poolInfo, NULL, &vk_descriptorPool));
	QVk_DebugSetObjectName((uint64_t)vk_descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "Descriptor Pool: Sampler + UBO");
}

// internal helper
static void CreateUboDescriptorSet(VkDescriptorSet *descSet, VkBuffer buffer)
{
	VkDescriptorSetAllocateInfo dsAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = vk_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vk_uboDescSetLayout
	};

	VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, descSet));

	VkDescriptorBufferInfo bufferInfo = {
		.buffer = buffer,
		.offset = 0,
		.range = UNIFORM_ALLOC_SIZE
	};

	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = NULL,
		.dstSet = *descSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.pImageInfo = NULL,
		.pBufferInfo = &bufferInfo,
		.pTexelBufferView = NULL,
	};

	vkUpdateDescriptorSets(vk_device.logical, 1, &descriptorWrite, 0, NULL);
}

// internal helper
static void CreateDynamicBuffers()
{
	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		QVk_CreateVertexBuffer(NULL, vk_config.vertex_buffer_size, &vk_dynVertexBuffers[i],
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		QVk_CreateIndexBuffer(NULL, vk_config.index_buffer_size, &vk_dynIndexBuffers[i],
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		VK_VERIFY(QVk_CreateUniformBuffer(vk_config.uniform_buffer_size,
			&vk_dynUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
		// keep dynamic buffers persistently mapped
		vk_dynVertexBuffers[i].pMappedData = buffer_map(&vk_dynVertexBuffers[i].resource);
		vk_dynIndexBuffers[i].pMappedData = buffer_map(&vk_dynIndexBuffers[i].resource);
		vk_dynUniformBuffers[i].pMappedData = buffer_map(&vk_dynUniformBuffers[i].resource);
		// create descriptor set for the uniform buffer
		CreateUboDescriptorSet(&vk_uboDescriptorSets[i], vk_dynUniformBuffers[i].resource.buffer);

		QVk_DebugSetObjectName((uint64_t)vk_uboDescriptorSets[i],
			VK_OBJECT_TYPE_DESCRIPTOR_SET, va("Dynamic UBO Descriptor Set #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynVertexBuffers[i].resource.buffer,
			VK_OBJECT_TYPE_BUFFER, va("Dynamic Vertex Buffer #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynVertexBuffers[i].resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Vertex Buffer #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynIndexBuffers[i].resource.buffer,
			VK_OBJECT_TYPE_BUFFER, va("Dynamic Index Buffer #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynIndexBuffers[i].resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Index Buffer #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynUniformBuffers[i].resource.buffer,
			VK_OBJECT_TYPE_BUFFER, va("Dynamic Uniform Buffer #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_dynUniformBuffers[i].resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Uniform Buffer #%d", i));
	}
}

// internal helper
static void ReleaseSwapBuffers()
{
	vk_activeSwapBufferIdx = (vk_activeSwapBufferIdx + 1) % NUM_SWAPBUFFER_SLOTS;
	int releaseBufferIdx   = (vk_activeSwapBufferIdx + 1) % NUM_SWAPBUFFER_SLOTS;

	if (vk_swapBuffersCnt[releaseBufferIdx] > 0)
	{
		for (int i = 0; i < vk_swapBuffersCnt[releaseBufferIdx]; i++)
			QVk_FreeBuffer(&vk_swapBuffers[releaseBufferIdx][i]);

		free(vk_swapBuffers[releaseBufferIdx]);
		vk_swapBuffers[releaseBufferIdx] = NULL;
		vk_swapBuffersCnt[releaseBufferIdx] = 0;
	}

	if (vk_swapDescSetsCnt[releaseBufferIdx] > 0)
	{
		vkFreeDescriptorSets(vk_device.logical, vk_descriptorPool, vk_swapDescSetsCnt[releaseBufferIdx], vk_swapDescriptorSets[releaseBufferIdx]);

		free(vk_swapDescriptorSets[releaseBufferIdx]);
		vk_swapDescriptorSets[releaseBufferIdx] = NULL;
		vk_swapDescSetsCnt[releaseBufferIdx] = 0;
	}
}

// internal helper
static int NextPow2(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

// internal helper
static uint8_t *QVk_GetIndexBuffer(VkDeviceSize size, VkDeviceSize *dstOffset, int currentBufferIdx);
static void RebuildTriangleIndexBuffer()
{
	int idx = 0;
	VkDeviceSize dstOffset = 0;
	VkDeviceSize bufferSize = 3 * vk_config.triangle_index_count * sizeof(uint16_t);
	uint16_t *iboData = NULL;
	uint16_t *fanData = malloc(bufferSize);
	uint16_t *stripData = malloc(bufferSize);

	// fill the index buffer so that we can emulate triangle fans via triangle lists
	for (int i = 0; i < vk_config.triangle_index_count; ++i)
	{
		fanData[idx++] = 0;
		fanData[idx++] = i + 1;
		fanData[idx++] = i + 2;
	}

	// fill the index buffer so that we can emulate triangle strips via triangle lists
	idx = 0;
	for (int i = 2; i < (vk_config.triangle_index_count + 2); ++i)
	{
		if ((i%2) == 0)
		{
			stripData[idx++] = i - 2;
			stripData[idx++] = i - 1;
			stripData[idx++] = i;
		}
		else
		{
			stripData[idx++] = i;
			stripData[idx++] = i - 1;
			stripData[idx++] = i - 2;
		}
	}

	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		VK_VERIFY(buffer_invalidate(&vk_dynIndexBuffers[i].resource));

		iboData = (uint16_t *)QVk_GetIndexBuffer(bufferSize, &dstOffset, i);
		if ((i%2) == 0)
		{
			memcpy(iboData, fanData, bufferSize);
		}
		else
		{
			memcpy(iboData, stripData, bufferSize);
		}

		VK_VERIFY(buffer_flush(&vk_dynIndexBuffers[i].resource));
	}

	vk_triangleFanIbo = &vk_dynIndexBuffers[0].resource.buffer;
	vk_triangleStripIbo = &vk_dynIndexBuffers[1].resource.buffer;
	vk_triangleFanIboUsage = ((bufferSize % 4) == 0) ? bufferSize : (bufferSize + 4 - (bufferSize % 4));

	free(fanData);
	free(stripData);
}

static void CreateStagingBuffer(VkDeviceSize size, qvkstagingbuffer_t *dstBuffer, int i)
{
	VkFenceCreateInfo fCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0
	};

	VK_VERIFY(QVk_CreateStagingBuffer(size,
		dstBuffer,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
	dstBuffer->pMappedData = buffer_map(&dstBuffer->resource);
	dstBuffer->submitted = false;

	VK_VERIFY(vkCreateFence(vk_device.logical, &fCreateInfo,
		NULL, &dstBuffer->fence));

	dstBuffer->cmdBuffer = QVk_CreateCommandBuffer(&vk_stagingCommandPool[i],
		VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VK_VERIFY(QVk_BeginCommand(&dstBuffer->cmdBuffer));

	QVk_DebugSetObjectName((uint64_t)dstBuffer->fence,
		VK_OBJECT_TYPE_FENCE, va("Fence: Staging Buffer #%d", i));
	QVk_DebugSetObjectName((uint64_t)dstBuffer->resource.buffer,
		VK_OBJECT_TYPE_BUFFER, va("Staging Buffer #%d", i));
	QVk_DebugSetObjectName((uint64_t)dstBuffer->resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Staging Buffer #%d", i));
	QVk_DebugSetObjectName((uintptr_t)dstBuffer->cmdBuffer,
		VK_OBJECT_TYPE_COMMAND_BUFFER, va("Command Buffer: Staging Buffer #%d", i));
}

// internal helper
static void CreateStagingBuffers()
{
	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		VK_VERIFY(QVk_CreateCommandPool(&vk_stagingCommandPool[i],
			vk_device.gfxFamilyIndex));
		QVk_DebugSetObjectName((uint64_t)vk_stagingCommandPool[i],
			VK_OBJECT_TYPE_COMMAND_POOL, va("Command Pool #%d: Staging", i));

		CreateStagingBuffer(STAGING_BUFFER_MAXSIZE, &vk_stagingBuffers[i], i);
	}
}

// Records a memory barrier in the given command buffer.
void Qvk_MemoryBarrier(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
	const VkMemoryBarrier memBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
	};
	vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0u, 1u, &memBarrier, 0u, NULL, 0u, NULL);
}

// internal helper
static void SubmitStagingBuffer(int index)
{
	if (vk_stagingBuffers[index].submitted)
	{
		// buffer is alredy submitted
		return;
	}

	Qvk_MemoryBarrier(vk_stagingBuffers[index].cmdBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		(VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT));

	VK_VERIFY(vkEndCommandBuffer(vk_stagingBuffers[index].cmdBuffer));

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_stagingBuffers[index].cmdBuffer
	};

	VK_VERIFY(vkQueueSubmit(vk_device.gfxQueue, 1, &submitInfo, vk_stagingBuffers[index].fence));

	vk_stagingBuffers[index].submitted = true;
	vk_activeStagingBuffer = (vk_activeStagingBuffer + 1) % NUM_DYNBUFFERS;
}

// internal helper
static void CreateStaticBuffers()
{
	const float texVerts[] = {	-1., -1., 0., 0.,
								 1.,  1., 1., 1.,
								-1.,  1., 0., 1.,
								 1., -1., 1., 0. };

	const float colorVerts[] = { -1., -1.,
								  1.,  1.,
								 -1.,  1.,
								  1., -1. };

	const uint32_t indices[] = { 0, 1, 2, 0, 3, 1 };

	QVk_CreateVertexBuffer(texVerts, sizeof(texVerts),
		&vk_texRectVbo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	QVk_CreateVertexBuffer(colorVerts, sizeof(colorVerts),
		&vk_colorRectVbo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	QVk_CreateIndexBuffer(indices, sizeof(indices),
		&vk_rectIbo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

	QVk_DebugSetObjectName((uint64_t)vk_texRectVbo.resource.buffer,
		VK_OBJECT_TYPE_BUFFER, "Static Buffer: Textured Rectangle VBO");
	QVk_DebugSetObjectName((uint64_t)vk_texRectVbo.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: Textured Rectangle VBO");
	QVk_DebugSetObjectName((uint64_t)vk_colorRectVbo.resource.buffer,
		VK_OBJECT_TYPE_BUFFER, "Static Buffer: Colored Rectangle VBO");
	QVk_DebugSetObjectName((uint64_t)vk_colorRectVbo.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: Colored Rectangle VBO");
	QVk_DebugSetObjectName((uint64_t)vk_rectIbo.resource.buffer,
		VK_OBJECT_TYPE_BUFFER, "Static Buffer: Rectangle IBO");
	QVk_DebugSetObjectName((uint64_t)vk_rectIbo.resource.memory,
		VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: Rectangle IBO");
}

static void
DestroyShaderModule(qvkshader_t *shaders)
{
	// final shader cleanup
	for (int i = 0; i < SHADER_INDEX_SIZE; ++i)
	{
		if (shaders[i].module)
		{
			vkDestroyShaderModule(vk_device.logical, shaders[i].module, NULL);
			memset(&shaders[i], 0, sizeof(qvkshader_t));
		}
	}
}

// internal helper
static void CreatePipelines()
{
	// shared pipeline vertex input state create infos
	VK_VERTINFO(RG, sizeof(float) * 2, VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32_SFLOAT, 0));

	VK_VERTINFO(RGB, sizeof(float) * 3, VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0));

	VK_VERTINFO(RG_RG, sizeof(float) * 4,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32_SFLOAT, 0),
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2));

	VK_VERTINFO(RGB_RG, sizeof(float) * 5,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RGB, sizeof(float) * 6,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RGBA,  sizeof(float) * 7,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RG_RG, sizeof(float) * 7,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3),
												VK_INPUTATTR_DESC(2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 5));

	VK_VERTINFO(RGB_RGBA_RG, sizeof(float) * 9,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),
												VK_INPUTATTR_DESC(2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 7));
	// no vertices passed to the pipeline (postprocessing)
	VkPipelineVertexInputStateCreateInfo vertInfoNull = VK_NULL_VERTEXINPUT_CINF;

	// shared descriptor set layouts
	VkDescriptorSetLayout samplerUboDsLayouts[] = { vk_samplerDescSetLayout, vk_uboDescSetLayout };
	VkDescriptorSetLayout samplerUboLmapDsLayouts[] = { vk_samplerDescSetLayout, vk_uboDescSetLayout, vk_samplerLightmapDescSetLayout };

	// shader array (vertex and fragment, no compute... yet)
	qvkshader_t shaders[SHADER_INDEX_SIZE] = {0};

	// textured quad pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, basic, basic);
	for (int i = 0; i < RP_COUNT; ++i)
	{
		vk_drawTexQuadPipeline[i].depthTestEnable = VK_FALSE;
		QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRG_RG, &vk_drawTexQuadPipeline[i], &vk_renderpasses[i], shaders, 2);
		QVk_DebugSetObjectName((uint64_t)vk_drawTexQuadPipeline[i].layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			va("Pipeline Layout: textured quad (%s)", renderpassObjectNames[i]));
		QVk_DebugSetObjectName((uint64_t)vk_drawTexQuadPipeline[i].pl, VK_OBJECT_TYPE_PIPELINE,
			va("Pipeline: textured quad (%s)", renderpassObjectNames[i]));
	}

	// draw particles pipeline (using a texture)
	VK_LOAD_VERTFRAG_SHADERS(shaders, particle, basic);
	vk_drawParticlesPipeline.depthWriteEnable = VK_TRUE;
	vk_drawParticlesPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoRGB_RGBA_RG, &vk_drawParticlesPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawParticlesPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: textured particles");
	QVk_DebugSetObjectName((uint64_t)vk_drawParticlesPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: textured particles");

	// draw particles pipeline (using point list)
	VK_LOAD_VERTFRAG_SHADERS(shaders, point_particle, point_particle);
	vk_drawPointParticlesPipeline.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	vk_drawPointParticlesPipeline.depthWriteEnable = VK_TRUE;
	vk_drawPointParticlesPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGBA, &vk_drawPointParticlesPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawPointParticlesPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: point particles");
	QVk_DebugSetObjectName((uint64_t)vk_drawPointParticlesPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: point particles");

	// colored quad pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, basic_color_quad, basic_color_quad);
	for (int i = 0; i < RP_COUNT; ++i)
	{
		vk_drawColorQuadPipeline[i].depthTestEnable = VK_FALSE;
		vk_drawColorQuadPipeline[i].blendOpts.blendEnable = VK_TRUE;
		QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRG, &vk_drawColorQuadPipeline[i], &vk_renderpasses[i], shaders, 2);
		QVk_DebugSetObjectName((uint64_t)vk_drawColorQuadPipeline[i].layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			va("Pipeline Layout: colored quad (%s)", renderpassObjectNames[i]));
		QVk_DebugSetObjectName((uint64_t)vk_drawColorQuadPipeline[i].pl, VK_OBJECT_TYPE_PIPELINE,
			va("Pipeline: colored quad (%s)", renderpassObjectNames[i]));
	}

	// untextured null model
	VK_LOAD_VERTFRAG_SHADERS(shaders, nullmodel, basic_color_quad);
	vk_drawNullModelPipeline.cullMode = VK_CULL_MODE_NONE;
	vk_drawNullModelPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_drawNullModelPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawNullModelPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: null model");
	QVk_DebugSetObjectName((uint64_t)vk_drawNullModelPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: null model");

	// textured model
	VK_LOAD_VERTFRAG_SHADERS(shaders, model, model);
	for (int i = 0; i < RP_COUNT; ++i)
	{
		vk_drawModelPipelineFan[i].topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawModelPipelineFan[i], &vk_renderpasses[i], shaders, 2);
		QVk_DebugSetObjectName((uint64_t)vk_drawModelPipelineFan[i].layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
			va("Pipeline Layout: draw model: fan (%s)", renderpassObjectNames[i]));
		QVk_DebugSetObjectName((uint64_t)vk_drawModelPipelineFan[i].pl, VK_OBJECT_TYPE_PIPELINE,
			va("Pipeline: draw model: fan (%s)", renderpassObjectNames[i]));
	}

	// dedicated model pipelines for translucent objects with depth write disabled
	vk_drawNoDepthModelPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawNoDepthModelPipelineFan.depthWriteEnable = VK_FALSE;
	vk_drawNoDepthModelPipelineFan.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawNoDepthModelPipelineFan, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawNoDepthModelPipelineFan.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: translucent model: fan");
	QVk_DebugSetObjectName((uint64_t)vk_drawNoDepthModelPipelineFan.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: translucent model: fan");

	// dedicated model pipelines for when left-handed weapon model is drawn
	vk_drawLefthandModelPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawLefthandModelPipelineFan.cullMode = VK_CULL_MODE_FRONT_BIT;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawLefthandModelPipelineFan, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawLefthandModelPipelineFan.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: left-handed model: fan");
	QVk_DebugSetObjectName((uint64_t)vk_drawLefthandModelPipelineFan.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: left-handed model: fan");

	// draw sprite pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, sprite, basic);
	vk_drawSpritePipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoRGB_RG, &vk_drawSpritePipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawSpritePipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: sprite");
	QVk_DebugSetObjectName((uint64_t)vk_drawSpritePipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: sprite");

	// draw polygon pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon, basic);
	vk_drawPolyPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawPolyPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RG, &vk_drawPolyPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: polygon");
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: polygon");

	// draw lightmapped polygon
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon_lmap, polygon_lmap);
	vk_drawPolyLmapPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(samplerUboLmapDsLayouts, 3, &vertInfoRGB_RG_RG, &vk_drawPolyLmapPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyLmapPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: lightmapped polygon");
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyLmapPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: lightmapped polygon");

	// draw polygon with warp effect (liquid) pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon_warp, basic);
	vk_drawPolyWarpPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawPolyWarpPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboLmapDsLayouts, 2, &vertInfoRGB_RG, &vk_drawPolyWarpPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyWarpPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: warped polygon (liquids)");
	QVk_DebugSetObjectName((uint64_t)vk_drawPolyWarpPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: warped polygon (liquids)");

	// draw solid polygon with warp effect (liquid) pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon_warp, basic);
	vk_drawPolySolidWarpPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(samplerUboLmapDsLayouts, 2, &vertInfoRGB_RG,
		&vk_drawPolySolidWarpPipeline, &vk_renderpasses[RP_WORLD],
		shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawPolySolidWarpPipeline.layout,
		VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: warped solid polygon (liquids)");
	QVk_DebugSetObjectName((uint64_t)vk_drawPolySolidWarpPipeline.pl,
		VK_OBJECT_TYPE_PIPELINE, "Pipeline: warped solid polygon (liquids)");

	// draw beam pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, beam, basic_color_quad);
	vk_drawBeamPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	vk_drawBeamPipeline.depthWriteEnable = VK_FALSE;
	vk_drawBeamPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB, &vk_drawBeamPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawBeamPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: beam");
	QVk_DebugSetObjectName((uint64_t)vk_drawBeamPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: beam");

	// draw skybox pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, skybox, basic);
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RG, &vk_drawSkyboxPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawSkyboxPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: skybox");
	QVk_DebugSetObjectName((uint64_t)vk_drawSkyboxPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: skybox");

	// draw dynamic light pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, d_light, basic_color_quad);
	vk_drawDLightPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawDLightPipeline.depthWriteEnable = VK_FALSE;
	vk_drawDLightPipeline.cullMode = VK_CULL_MODE_FRONT_BIT;
	vk_drawDLightPipeline.blendOpts.blendEnable = VK_TRUE;
	vk_drawDLightPipeline.blendOpts.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_drawDLightPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_drawDLightPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: dynamic light");
	QVk_DebugSetObjectName((uint64_t)vk_drawDLightPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: dynamic light");

	/* r_showtris render pipeline */
	VK_LOAD_VERTFRAG_SHADERS(shaders, d_light, basic_color_quad);
	vk_showTrisPipeline.cullMode = VK_CULL_MODE_NONE;
	vk_showTrisPipeline.depthTestEnable = VK_FALSE;
	vk_showTrisPipeline.depthWriteEnable = VK_FALSE;
	vk_showTrisPipeline.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_showTrisPipeline, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_showTrisPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: show triangles");
	QVk_DebugSetObjectName((uint64_t)vk_showTrisPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: show triangles");

	/* vk_shadows render pipeline */
	VK_LOAD_VERTFRAG_SHADERS(shaders, shadows, basic_color_quad);
	vk_shadowsPipelineFan.blendOpts.blendEnable = VK_TRUE;
	vk_shadowsPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB, &vk_shadowsPipelineFan, &vk_renderpasses[RP_WORLD], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_shadowsPipelineFan.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: draw shadows: fan");
	QVk_DebugSetObjectName((uint64_t)vk_shadowsPipelineFan.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: draw shadows: fan");

	/* underwater world warp pipeline (postprocess) */
	VK_LOAD_VERTFRAG_SHADERS(shaders, world_warp, world_warp);
	vk_worldWarpPipeline.depthTestEnable = VK_FALSE;
	vk_worldWarpPipeline.depthWriteEnable = VK_FALSE;
	vk_worldWarpPipeline.cullMode = VK_CULL_MODE_NONE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoNull, &vk_worldWarpPipeline, &vk_renderpasses[RP_WORLD_WARP], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_worldWarpPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: underwater view warp");
	QVk_DebugSetObjectName((uint64_t)vk_worldWarpPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: underwater view warp");

	/* postprocessing pipeline */
	VK_LOAD_VERTFRAG_SHADERS(shaders, postprocess, postprocess);
	vk_postprocessPipeline.depthTestEnable = VK_FALSE;
	vk_postprocessPipeline.depthWriteEnable = VK_FALSE;
	vk_postprocessPipeline.cullMode = VK_CULL_MODE_NONE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoNull, &vk_postprocessPipeline, &vk_renderpasses[RP_UI], shaders, 2);
	QVk_DebugSetObjectName((uint64_t)vk_postprocessPipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Pipeline Layout: world postprocess");
	QVk_DebugSetObjectName((uint64_t)vk_postprocessPipeline.pl, VK_OBJECT_TYPE_PIPELINE, "Pipeline: world postprocess");

	DestroyShaderModule(shaders);
}

static void DestroyStagingBuffer(qvkstagingbuffer_t *dstBuffer)
{
	if (dstBuffer->resource.buffer != VK_NULL_HANDLE)
	{
		// wait only if something is submitted
		if (dstBuffer->submitted)
		{
			VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &dstBuffer->fence,
				VK_TRUE, UINT64_MAX));
		}

		buffer_unmap(&dstBuffer->resource);
		QVk_FreeStagingBuffer(dstBuffer);
		vkDestroyFence(vk_device.logical, dstBuffer->fence, NULL);
	}
}

/*
** QVk_Shutdown
**
** Destroy all Vulkan related resources.
*/
void QVk_Shutdown( void )
{
	if (!vk_initialized)
	{
		return;
	}

	if (vk_instance != VK_NULL_HANDLE)
	{
		R_Printf(PRINT_ALL, "Shutting down Vulkan\n");

		for (int i = 0; i < RP_COUNT; ++i)
		{
			QVk_DestroyPipeline(&vk_drawColorQuadPipeline[i]);
			QVk_DestroyPipeline(&vk_drawModelPipelineFan[i]);
			QVk_DestroyPipeline(&vk_drawTexQuadPipeline[i]);
		}
		QVk_DestroyPipeline(&vk_drawNullModelPipeline);
		QVk_DestroyPipeline(&vk_drawNoDepthModelPipelineFan);
		QVk_DestroyPipeline(&vk_drawLefthandModelPipelineFan);
		QVk_DestroyPipeline(&vk_drawParticlesPipeline);
		QVk_DestroyPipeline(&vk_drawPointParticlesPipeline);
		QVk_DestroyPipeline(&vk_drawSpritePipeline);
		QVk_DestroyPipeline(&vk_drawPolyPipeline);
		QVk_DestroyPipeline(&vk_drawPolyLmapPipeline);
		QVk_DestroyPipeline(&vk_drawPolyWarpPipeline);
		QVk_DestroyPipeline(&vk_drawPolySolidWarpPipeline);
		QVk_DestroyPipeline(&vk_drawBeamPipeline);
		QVk_DestroyPipeline(&vk_drawSkyboxPipeline);
		QVk_DestroyPipeline(&vk_drawDLightPipeline);
		QVk_DestroyPipeline(&vk_showTrisPipeline);
		QVk_DestroyPipeline(&vk_shadowsPipelineFan);
		QVk_DestroyPipeline(&vk_worldWarpPipeline);
		QVk_DestroyPipeline(&vk_postprocessPipeline);
		QVk_FreeBuffer(&vk_texRectVbo);
		QVk_FreeBuffer(&vk_colorRectVbo);
		QVk_FreeBuffer(&vk_rectIbo);
		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			if (vk_dynUniformBuffers[i].resource.buffer != VK_NULL_HANDLE)
			{
				buffer_unmap(&vk_dynUniformBuffers[i].resource);
				QVk_FreeBuffer(&vk_dynUniformBuffers[i]);
			}
			if (vk_dynIndexBuffers[i].resource.buffer != VK_NULL_HANDLE)
			{
				buffer_unmap(&vk_dynIndexBuffers[i].resource);
				QVk_FreeBuffer(&vk_dynIndexBuffers[i]);
			}
			if (vk_dynVertexBuffers[i].resource.buffer != VK_NULL_HANDLE)
			{
				buffer_unmap(&vk_dynVertexBuffers[i].resource);
				QVk_FreeBuffer(&vk_dynVertexBuffers[i]);
			}
			DestroyStagingBuffer(&vk_stagingBuffers[i]);
			if (vk_stagingCommandPool[i] != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(vk_device.logical, vk_stagingCommandPool[i], NULL);
				vk_stagingCommandPool[i] = VK_NULL_HANDLE;
			}
		}
		if (vk_descriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(vk_device.logical, vk_descriptorPool, NULL);
		if (vk_uboDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_uboDescSetLayout, NULL);
		if (vk_samplerDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_samplerDescSetLayout, NULL);
		if (vk_samplerLightmapDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_samplerLightmapDescSetLayout, NULL);
		for (int i = 0; i < RP_COUNT; i++)
		{
			if (vk_renderpasses[i].rp != VK_NULL_HANDLE)
				vkDestroyRenderPass(vk_device.logical, vk_renderpasses[i].rp, NULL);
			vk_renderpasses[i].rp = VK_NULL_HANDLE;
		}

		for (int i = 0; i < NUM_CMDBUFFERS; i++)
		{
			if (vk_commandPool[i] != VK_NULL_HANDLE)
			{
				vkFreeCommandBuffers(vk_device.logical, vk_commandPool[i], 1, &vk_commandbuffers[i]);
				vkDestroyCommandPool(vk_device.logical, vk_commandPool[i], NULL);
				vk_commandPool[i] = VK_NULL_HANDLE;
			}
		}
		if (vk_commandbuffers != NULL)
		{
			free(vk_commandbuffers);
			vk_commandbuffers = NULL;
		}
		if (vk_transferCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_transferCommandPool, NULL);
		DestroySamplers();
		DestroyFramebuffers();
		DestroyImageViews();
		DestroyDrawBuffers();
		if (vk_swapchain.sc != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(vk_device.logical, vk_swapchain.sc, NULL);
			free(vk_swapchain.images);
			vk_swapchain.sc = VK_NULL_HANDLE;
			vk_swapchain.images = NULL;
			vk_swapchain.imageCount = 0;
		}
		if (vk_device.logical != VK_NULL_HANDLE)
		{
			for (int i = 0; i < NUM_CMDBUFFERS; ++i)
			{
				vkDestroySemaphore(vk_device.logical, vk_imageAvailableSemaphores[i], NULL);
				vkDestroySemaphore(vk_device.logical, vk_renderFinishedSemaphores[i], NULL);
				vkDestroyFence(vk_device.logical, vk_fences[i], NULL);
			}
		}
		// free all memory
		vulkan_memory_delete();

		if (vk_device.logical != VK_NULL_HANDLE)
			vkDestroyDevice(vk_device.logical, NULL);
		if(vk_surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
		QVk_DestroyValidationLayers();

		vkDestroyInstance(vk_instance, NULL);
		vk_instance = VK_NULL_HANDLE;
		vk_activeCmdbuffer = VK_NULL_HANDLE;
		vk_descriptorPool = VK_NULL_HANDLE;
		vk_uboDescSetLayout = VK_NULL_HANDLE;
		vk_samplerDescSetLayout = VK_NULL_HANDLE;
		vk_samplerLightmapDescSetLayout = VK_NULL_HANDLE;
		vk_transferCommandPool = VK_NULL_HANDLE;
		vk_activeBufferIdx = 0;
		vk_imageIndex = 0;
	}
}

#ifdef __ANDROID__ //karin: ANativeWindow
void QVk_SetWindow(ANativeWindow *window)
#else
void QVk_SetWindow(SDL_Window *window)
#endif
{
	vk_window = window;
}

/*
 * Fills the actual size of the drawable into width and height.
 */
void QVk_GetDrawableSize(int *width, int *height)
{
#ifdef __ANDROID__ //karin: get setting size, not real size
    extern void Android_Vulkan_GetDrawableSize(int* width, int* height);
	Android_Vulkan_GetDrawableSize(width, height);
#else
#ifdef USE_SDL3
	SDL_GetWindowSizeInPixels(vk_window, width, height);
#else
	SDL_GL_GetDrawableSize(vk_window, width, height);
#endif
#endif
}

void QVk_WaitAndShutdownAll (void)
{
	if (!vk_initialized)
	{
		return;
	}

	if (vk_device.logical != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(vk_device.logical);
	}

	Mod_FreeAll();
	Mod_FreeModelsKnown();
	Vk_ShutdownImages();
	Mesh_Free();
	QVk_Shutdown();

	vk_frameStarted = false;
	vk_initialized = false;
}

void QVk_Restart(void)
{
	QVk_WaitAndShutdownAll();
	if (!QVk_Init())
		Com_Error(ERR_FATAL, "Unable to restart Vulkan renderer");
	QVk_PostInit();
	ri.Vid_RequestRestart(RESTART_PARTIAL);
}

void QVk_PostInit(void)
{
	Mesh_Init();
	Vk_InitImages();
	Mod_Init();
	RE_InitParticleTexture();
	Draw_InitLocal();
}

/*
** QVk_Init
**
** This is responsible for initializing Vulkan.
**
*/
qboolean QVk_Init(void)
{
	PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
	uint32_t instanceVersion = VK_API_VERSION_1_0;

	if (vkEnumerateInstanceVersion)
	{
		VK_VERIFY(vkEnumerateInstanceVersion(&instanceVersion));
	}

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Quake 2",
		.applicationVersion = VK_MAKE_VERSION(3, 21, 0),
		.pEngineName = "id Tech 2",
		.engineVersion = VK_MAKE_VERSION(2, 0, 0),
		.apiVersion = instanceVersion
	};

	uint32_t extCount;
	char **wantedExtensions;
	memset(vk_config.supported_present_modes, 0, sizeof(vk_config.supported_present_modes));
	memset(vk_config.extensions, 0, sizeof(vk_config.extensions));
	memset(vk_config.layers, 0, sizeof(vk_config.layers));
	vk_config.vk_version = instanceVersion;
	vk_config.vertex_buffer_usage  = 0;
	vk_config.vertex_buffer_max_usage = 0;
	vk_config.vertex_buffer_size   = VERTEX_BUFFER_SIZE;
	vk_config.index_buffer_usage   = 0;
	vk_config.index_buffer_max_usage = 0;
	vk_config.index_buffer_size    = INDEX_BUFFER_SIZE;
	vk_config.uniform_buffer_usage = 0;
	vk_config.uniform_buffer_max_usage = 0;
	vk_config.uniform_buffer_size  = UNIFORM_BUFFER_SIZE;
	vk_config.triangle_index_usage = 0;
	vk_config.triangle_index_max_usage = 0;
	vk_config.triangle_index_count = TRIANGLE_INDEX_CNT;

#ifdef __ANDROID__ //karin: require Android surface extensions
    extern int Android_Vulkan_GetInstanceExtensions(unsigned int *count, const char **exts);
	if (!Android_Vulkan_GetInstanceExtensions(&extCount, NULL))
	{
		R_Printf(PRINT_ALL, "%s() Android_Vulkan_GetInstanceExtensions failed",
				__func__);
		return false;
	}
#else
#ifdef USE_SDL3
	if (!SDL_Vulkan_GetInstanceExtensions(&extCount))
#else
	if (!SDL_Vulkan_GetInstanceExtensions(vk_window, &extCount, NULL))
#endif
	{
		R_Printf(PRINT_ALL, "%s() SDL_Vulkan_GetInstanceExtensions failed: %s",
				__func__, SDL_GetError());
		return false;
	}
#endif

	// add space for validation layer
	if (r_validation->value > 0)
		extCount++;

#if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) && defined(__APPLE__)
	extCount++;
#endif

#ifdef __ANDROID__ //karin: require Android surface extensions
	wantedExtensions = malloc(extCount * sizeof(char *));
	if (!Android_Vulkan_GetInstanceExtensions(&extCount, (const char **)wantedExtensions))
	{
		R_Printf(PRINT_ALL, "%s() Android_Vulkan_GetInstanceExtensions failed",
				__func__);
		free(wantedExtensions);
		return false;
	}
#else
#ifdef USE_SDL3
	if ((wantedExtensions = (char **)SDL_Vulkan_GetInstanceExtensions(&extCount)) == NULL)
	{
		R_Printf(PRINT_ALL, "%s() SDL_Vulkan_GetInstanceExtensions failed: %s",
				__func__, SDL_GetError());
		return false;
	}
#else
	wantedExtensions = malloc(extCount * sizeof(char *));
	if (!SDL_Vulkan_GetInstanceExtensions(vk_window, &extCount, (const char **)wantedExtensions))
	{
		R_Printf(PRINT_ALL, "%s() SDL_Vulkan_GetInstanceExtensions failed: %s",
				__func__, SDL_GetError());
		free(wantedExtensions);
		return false;
	}
#endif
#endif

	// restore extensions count
	if (r_validation->value > 0)
	{
		extCount++;
		wantedExtensions[extCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}

#if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) && defined(__APPLE__)
	extCount++;
	wantedExtensions[extCount - 1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
#endif

	R_Printf(PRINT_ALL, "Enabled extensions: ");
	for (int i = 0; i < extCount; i++)
	{
		R_Printf(PRINT_ALL, "%s ", wantedExtensions[i]);
		vk_config.extensions[i] = wantedExtensions[i];
	}
	R_Printf(PRINT_ALL, "\n");

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.pApplicationInfo = &appInfo,
#if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) && defined(__APPLE__)
		.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = extCount,
		.ppEnabledExtensionNames = (const char* const*)wantedExtensions
	};

#if defined(__APPLE__)
	MVKConfiguration vk_molten_config;
	size_t vk_molten_len = sizeof(MVKConfiguration);
	memset(&vk_molten_config, 0, vk_molten_len);
	// Preferably getting the existing configuration to fill the most vital
	// data instead of "smartly" setting by hand.
	// We do not pass vk_instance here as we change before its creation
	// anyway, the api ignore it.
	if (qvkGetMoltenVKConfigurationMVK(VK_NULL_HANDLE, &vk_molten_config, &vk_molten_len) != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not fetch the MoltenVK configuration\n", __func__);
		return false;
	}

	R_Printf(PRINT_ALL, "%s(): Molten fast math %d\n", __func__, vk_molten_config.fastMathEnabled);
	R_Printf(PRINT_ALL, "%s(): Molten Metal buffers %d\n", __func__, vk_molten_config.useMetalArgumentBuffers);

	VkBool32 fastMath = vk_molten_fastmath->value > 0 ? VK_TRUE : VK_FALSE;
	VkBool32 metalBuffers = vk_molten_metalbuffers->value > 0 ? VK_TRUE : VK_FALSE;
	// unsure of the lost device resuming option value yet might need further testing
	// also a watermark is available, more fit for demo but might be invading tough

	if (vk_molten_config.fastMathEnabled != fastMath || vk_molten_config.useMetalArgumentBuffers != metalBuffers) {
		vk_molten_config.fastMathEnabled = fastMath;
		vk_molten_config.useMetalArgumentBuffers = metalBuffers;
		if (qvkSetMoltenVKConfigurationMVK(VK_NULL_HANDLE, &vk_molten_config, &vk_molten_len) != VK_SUCCESS)
		{
			R_Printf(PRINT_ALL, "%s(): Could not update the MoltenVK configuration\n", __func__);
		}
	}
#endif

// introduced in SDK 1.1.121
#if VK_HEADER_VERSION > 114
	VkValidationFeatureEnableEXT validationFeaturesEnable[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
	VkValidationFeaturesEXT validationFeatures = {
		.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.pNext = NULL,
		.enabledValidationFeatureCount = sizeof(validationFeaturesEnable) / sizeof(validationFeaturesEnable[0]),
		.pEnabledValidationFeatures = validationFeaturesEnable,
		.disabledValidationFeatureCount = 0,
		.pDisabledValidationFeatures = NULL
	};

	if (r_validation->value > 1)
	{
		createInfo.pNext = &validationFeatures;
	}
#endif

// introduced in SDK 1.1.106
#if VK_HEADER_VERSION > 101
	const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
#else
	const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
#endif

	if (r_validation->value > 0)
	{
		createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
		for (int i = 0; i < createInfo.enabledLayerCount; i++)
		{
			vk_config.layers[i] = validationLayers[i];
		}
	}

	VkResult res = vkCreateInstance(&createInfo, NULL, &vk_instance);

	if (res == VK_ERROR_LAYER_NOT_PRESENT && r_validation->value > 0) {
		// we give a "last try" if the validation layer fails
		// before falling back to a GL renderer
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = 0;
		createInfo.pNext = NULL;
		memset(vk_config.layers, 0, sizeof(vk_config.layers));
		ri.Cvar_Set("r_validation", "0");
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan instance, disabling r_validation\n", __func__);
		res = vkCreateInstance(&createInfo, NULL, &vk_instance);
	}

#ifndef USE_SDL3
	free(wantedExtensions);
#endif

	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan instance: %s\n", __func__, QVk_GetError(res));
		return false;
	}

	volkLoadInstance(vk_instance);
	R_Printf(PRINT_ALL, "...created Vulkan instance\n");
	if (r_validation->value > 0)
	{
		// initialize function pointers
		qvkCreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
		qvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");
		qvkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vk_instance, "vkSetDebugUtilsObjectNameEXT");
		qvkSetDebugUtilsObjectTagEXT  = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(vk_instance, "vkSetDebugUtilsObjectTagEXT");
		qvkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(vk_instance, "vkCmdBeginDebugUtilsLabelEXT");
		qvkCmdEndDebugUtilsLabelEXT   = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(vk_instance, "vkCmdEndDebugUtilsLabelEXT");
		qvkInsertDebugUtilsLabelEXT   = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(vk_instance, "vkCmdInsertDebugUtilsLabelEXT");
		qvkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugReportCallbackEXT");
		qvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugReportCallbackEXT");
	}
	else
	{
		qvkCreateDebugUtilsMessengerEXT  = NULL;
		qvkDestroyDebugUtilsMessengerEXT = NULL;
		qvkSetDebugUtilsObjectNameEXT = NULL;
		qvkSetDebugUtilsObjectTagEXT = NULL;
		qvkCmdBeginDebugUtilsLabelEXT = NULL;
		qvkCmdEndDebugUtilsLabelEXT = NULL;
		qvkInsertDebugUtilsLabelEXT = NULL;
		qvkCreateDebugReportCallbackEXT = NULL;
		qvkDestroyDebugReportCallbackEXT = NULL;
	}

	if (r_validation->value > 0)
		QVk_CreateValidationLayers();

	if (!Vkimp_CreateSurface(vk_window))
	{
		return false;
	}
	R_Printf(PRINT_ALL, "...created Vulkan surface\n");

	// create Vulkan device - see if the user prefers any specific device if there's more than one GPU in the system
	if (!QVk_CreateDevice((int)vk_device_idx->value))
	{
		return false;
	}
	QVk_DebugSetObjectName((uintptr_t)vk_device.physical, VK_OBJECT_TYPE_PHYSICAL_DEVICE, va("Physical Device: %s", vk_config.vendor_name));

	if (r_validation->value > 0)
		vulkan_memory_types_show();

	// setup swapchain
	res = QVk_CreateSwapchain();
	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan swapchain: %s\n", __func__, QVk_GetError(res));
		return false;
	}
	R_Printf(PRINT_ALL, "...created Vulkan swapchain\n");

	// set viewport and scissor
	if (vid_fullscreen->value == 2)
	{
		// Center viewport in "keep resolution mode".
		vk_viewport.x = Q_max(
			0.f, (float)(vk_swapchain.extent.width - (uint32_t)(vid.width)) / 2.0f);
		vk_viewport.y = Q_max(
			0.f, (float)(vk_swapchain.extent.height - (uint32_t)(vid.height)) / 2.0f);
	}
	else
	{
		vk_viewport.x = 0.f;
		vk_viewport.y = 0.f;
	}
	vk_viewport.minDepth = 0.f;
	vk_viewport.maxDepth = 1.f;
	vk_viewport.width = Q_min(
		(float)vid.width, (float)(vk_swapchain.extent.width) - vk_viewport.x);
	vk_viewport.height = Q_min(
		(float)vid.height, (float)(vk_swapchain.extent.height) - vk_viewport.y);
	vk_scissor.offset.x = 0;
	vk_scissor.offset.y = 0;
	vk_scissor.extent = vk_swapchain.extent;

	// setup fences and semaphores
	VkFenceCreateInfo fCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	VkSemaphoreCreateInfo sCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};
	for (int i = 0; i < NUM_CMDBUFFERS; ++i)
	{
		VK_VERIFY(vkCreateFence(vk_device.logical, &fCreateInfo, NULL, &vk_fences[i]));
		VK_VERIFY(vkCreateSemaphore(vk_device.logical, &sCreateInfo, NULL, &vk_imageAvailableSemaphores[i]));
		VK_VERIFY(vkCreateSemaphore(vk_device.logical, &sCreateInfo, NULL, &vk_renderFinishedSemaphores[i]));

		QVk_DebugSetObjectName((uint64_t)vk_fences[i],
			VK_OBJECT_TYPE_FENCE, va("Fence #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_imageAvailableSemaphores[i],
			VK_OBJECT_TYPE_SEMAPHORE, va("Semaphore: image available #%d", i));
		QVk_DebugSetObjectName((uint64_t)vk_renderFinishedSemaphores[i],
			VK_OBJECT_TYPE_SEMAPHORE, va("Semaphore: render finished #%d", i));
	}
	R_Printf(PRINT_ALL, "...created synchronization objects\n");

	// setup render passes
	for (int i = 0; i < RP_COUNT; ++i)
	{
		vk_renderpasses[i].colorLoadOp = r_clear->value ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	VkSampleCountFlagBits msaaMode = GetSampleCount((int)vk_msaa->value,
			vk_device.properties.limits.framebufferColorSampleCounts);

	// MSAA setting will be only relevant for the primary world render pass
	vk_renderpasses[RP_WORLD].sampleCount = msaaMode;

	res = CreateRenderpasses();
	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan render passes: %s\n", __func__, QVk_GetError(res));
		return false;
	}
	R_Printf(PRINT_ALL, "...created %d Vulkan render passes\n", RP_COUNT);

	// setup command pools
	for (int i = 0; i < NUM_CMDBUFFERS; i++)
	{
		res = QVk_CreateCommandPool(&vk_commandPool[i], vk_device.gfxFamilyIndex);
		if (res != VK_SUCCESS)
		{
			R_Printf(PRINT_ALL, "%s(): Could not create Vulkan command pool #%d for graphics: %s\n", __func__, i, QVk_GetError(res));
			return false;
		}
		QVk_DebugSetObjectName((uint64_t)vk_commandPool[i],
			VK_OBJECT_TYPE_COMMAND_POOL, va("Command Pool #%d: Graphics", i));
	}

	res = QVk_CreateCommandPool(&vk_transferCommandPool, vk_device.transferFamilyIndex);
	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan command pool for transfer: %s\n", __func__, QVk_GetError(res));
		return false;
	}

	QVk_DebugSetObjectName((uint64_t)vk_transferCommandPool,
		VK_OBJECT_TYPE_COMMAND_POOL, "Command Pool: Transfer");
	R_Printf(PRINT_ALL, "...created Vulkan command pools\n");

	// setup draw buffers
	CreateDrawBuffers();

	// setup image views
	res = CreateImageViews();
	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan image views: %s\n", __func__, QVk_GetError(res));
		return false;
	}
	R_Printf(PRINT_ALL, "...created %d Vulkan image view(s)\n", vk_swapchain.imageCount);

	// setup framebuffers
	res = CreateFramebuffers();
	if (res != VK_SUCCESS)
	{
		R_Printf(PRINT_ALL, "%s(): Could not create Vulkan framebuffers: %s\n", __func__, QVk_GetError(res));
		return false;
	}
	R_Printf(PRINT_ALL, "...created %d Vulkan framebuffers\n", vk_swapchain.imageCount);

	// setup command buffers (double buffering)
	vk_commandbuffers = (VkCommandBuffer *)malloc(NUM_CMDBUFFERS * sizeof(VkCommandBuffer));

	for (int i = 0; i < NUM_CMDBUFFERS; i++)
	{
		VkCommandBufferAllocateInfo cbInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = NULL,
			.commandPool = vk_commandPool[i],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		res = vkAllocateCommandBuffers(vk_device.logical, &cbInfo, &vk_commandbuffers[i]);
		if (res != VK_SUCCESS)
		{
			R_Printf(PRINT_ALL, "%s(): Could not create Vulkan commandbuffers: %s\n", __func__, QVk_GetError(res));
			free(vk_commandbuffers);
			vk_commandbuffers = NULL;
			return false;
		}
	}
	R_Printf(PRINT_ALL, "...created %d Vulkan commandbuffers\n", NUM_CMDBUFFERS);

	// initialize tracker variables
	vk_activeCmdbuffer = vk_commandbuffers[vk_activeBufferIdx];

	CreateDescriptorSetLayouts();
	CreateDescriptorPool();
	// create static vertex/index buffers reused in the games
	CreateStaticBuffers();
	// create vertex, index and uniform buffer pools
	CreateDynamicBuffers();
	// create staging buffers
	CreateStagingBuffers();
	// assign a dynamic index buffer for triangle fan emulation
	RebuildTriangleIndexBuffer();
	CreatePipelines();
	CreateSamplers();

	// main and world warp color buffers will be sampled for postprocessing effects, so they need descriptors and samplers
	VkDescriptorSetAllocateInfo dsAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = vk_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vk_samplerDescSetLayout
	};

	VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, &vk_colorbuffer.descriptorSet));
	QVk_UpdateTextureSampler(&vk_colorbuffer, S_NEAREST_UNNORMALIZED, false);
	VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, &vk_colorbufferWarp.descriptorSet));
	QVk_UpdateTextureSampler(&vk_colorbufferWarp, S_NEAREST_UNNORMALIZED, false);

	QVk_DebugSetObjectName((uint64_t)vk_colorbuffer.descriptorSet,
		VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descriptor Set: World Color Buffer");
	QVk_DebugSetObjectName((uint64_t)vk_colorbufferWarp.descriptorSet,
		VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descriptor Set: Warp Postprocess Color Buffer");

	vk_frameStarted = false;
	vk_initialized = true;
	return true;
}

VkResult QVk_BeginFrame(const VkViewport* viewport, const VkRect2D* scissor)
{
	// reset tracking variables
	vk_state.current_pipeline = VK_NULL_HANDLE;
	vk_state.current_renderpass = RP_COUNT;
	vk_config.vertex_buffer_usage  = 0;
	// triangle fan index buffer data will not be cleared between frames unless the buffer itself is too small
	vk_config.index_buffer_usage   = vk_triangleFanIboUsage;
	vk_config.uniform_buffer_usage = 0;
	vk_config.triangle_index_usage = 0;

	ReleaseSwapBuffers();

	VkResult result = vkAcquireNextImageKHR(vk_device.logical, vk_swapchain.sc, 500000000, vk_imageAvailableSemaphores[vk_activeBufferIdx], VK_NULL_HANDLE, &vk_imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR
#if !defined(__ANDROID__) //karin: do not recreate swapchain when VK_SUBOPTIMAL_KHR
	|| result == VK_SUBOPTIMAL_KHR
#endif
	|| result == VK_ERROR_SURFACE_LOST_KHR || result == VK_TIMEOUT)
	{
		vk_recreateSwapchainNeeded = true;

		// for VK_OUT_OF_DATE_KHR and VK_SUBOPTIMAL_KHR it'd be fine to just rebuild the swapchain but let's take the easy way out and restart Vulkan.
		R_Printf(PRINT_ALL, "%s(): received %s after vkAcquireNextImageKHR - restarting video!\n", __func__, QVk_GetError(result));
		return result;
	}
	else if (result != VK_SUCCESS
#ifdef __ANDROID__ //karin: VK_SUBOPTIMAL_KHR as success
		&& result != VK_SUBOPTIMAL_KHR
#endif
	)
	{
		Sys_Error("%s(): unexpected error after vkAcquireNextImageKHR: %s", __func__, QVk_GetError(result));
	}

	vk_activeCmdbuffer = vk_commandbuffers[vk_activeBufferIdx];

	// swap dynamic buffers
	vk_activeDynBufferIdx = (vk_activeDynBufferIdx + 1) % NUM_DYNBUFFERS;
	vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset = 0;
	vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset = 0;
	// triangle fan index data is placed in the beginning of the buffer
	vk_dynIndexBuffers[vk_activeDynBufferIdx].currentOffset = vk_triangleFanIboUsage;
	VK_VERIFY(buffer_invalidate(&vk_dynUniformBuffers[vk_activeDynBufferIdx].resource));
	VK_VERIFY(buffer_invalidate(&vk_dynVertexBuffers[vk_activeDynBufferIdx].resource));
	VK_VERIFY(buffer_invalidate(&vk_dynIndexBuffers[vk_activeDynBufferIdx].resource));

	VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &vk_fences[vk_activeBufferIdx], VK_TRUE, UINT32_MAX));
	VK_VERIFY(vkResetFences(vk_device.logical, 1, &vk_fences[vk_activeBufferIdx]));

	// setup command buffers and render pass for drawing
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	// Command buffers are implicitly reset by vkBeginCommandBuffer if VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT is set - this is expensive.
	// It's more efficient to reset entire pool instead and it also fixes the VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT performance warning.
	// see also: https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/command_buffer_usage/command_buffer_usage_tutorial.md
	vkResetCommandPool(vk_device.logical, vk_commandPool[vk_activeBufferIdx], 0);
	VK_VERIFY(vkBeginCommandBuffer(vk_commandbuffers[vk_activeBufferIdx], &beginInfo));

	vkCmdSetViewport(vk_commandbuffers[vk_activeBufferIdx], 0, 1, viewport);
	vkCmdSetScissor(vk_commandbuffers[vk_activeBufferIdx], 0, 1, scissor);

	vk_frameStarted = true;
	return VK_SUCCESS;
}

VkResult QVk_EndFrame(qboolean force)
{
	// continue only if QVk_BeginFrame() had been previously issued
	if (!vk_frameStarted)
	{
		return VK_SUCCESS;
	}

	// this may happen if Sys_Error is issued mid-frame, so we need to properly advance the draw pipeline
	if (force)
	{
		if(!RE_EndWorldRenderpass())
			// buffers is not initialized
			return VK_NOT_READY;
	}

	// submit
	QVk_SubmitStagingBuffers();
	VK_VERIFY(buffer_flush(&vk_dynUniformBuffers[vk_activeDynBufferIdx].resource));
	VK_VERIFY(buffer_flush(&vk_dynVertexBuffers[vk_activeDynBufferIdx].resource));
	VK_VERIFY(buffer_flush(&vk_dynIndexBuffers[vk_activeDynBufferIdx].resource));

	vkCmdEndRenderPass(vk_commandbuffers[vk_activeBufferIdx]);
	QVk_DebugLabelEnd(&vk_commandbuffers[vk_activeBufferIdx]);
	VK_VERIFY(vkEndCommandBuffer(vk_commandbuffers[vk_activeBufferIdx]));

	VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_imageAvailableSemaphores[vk_activeBufferIdx],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vk_renderFinishedSemaphores[vk_activeBufferIdx],
		.pWaitDstStageMask = &waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_commandbuffers[vk_activeBufferIdx]
	};

	VK_VERIFY(vkQueueSubmit(vk_device.gfxQueue, 1, &submitInfo, vk_fences[vk_activeBufferIdx]));

	// present
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_renderFinishedSemaphores[vk_activeBufferIdx],
		.swapchainCount = 1,
		.pSwapchains = &vk_swapchain.sc,
		.pImageIndices = &vk_imageIndex,
		.pResults = NULL
	};

	VkResult renderResult = vkQueuePresentKHR(vk_device.presentQueue, &presentInfo);

	// for VK_OUT_OF_DATE_KHR and VK_SUBOPTIMAL_KHR it'd be fine to just rebuild the swapchain but let's take the easy way out and restart video system
	if (renderResult == VK_ERROR_OUT_OF_DATE_KHR
#if !defined(__ANDROID__) //karin: do not recreate swapchain when VK_SUBOPTIMAL_KHR
	|| renderResult == VK_SUBOPTIMAL_KHR
#endif
	|| renderResult == VK_ERROR_SURFACE_LOST_KHR)
	{
		R_Printf(PRINT_ALL, "%s(): received %s after vkQueuePresentKHR - will restart video!\n", __func__, QVk_GetError(renderResult));
		vk_recreateSwapchainNeeded = true;
	}
	else if (renderResult != VK_SUCCESS
 #ifdef __ANDROID__ //karin: VK_SUBOPTIMAL_KHR as success
 				&& renderResult != VK_SUBOPTIMAL_KHR
 #endif
			 )
	{
		Sys_Error("%s(): unexpected error after vkQueuePresentKHR: %s", __func__, QVk_GetError(renderResult));
	}

	vk_activeBufferIdx = (vk_activeBufferIdx + 1) % NUM_CMDBUFFERS;

	vk_frameStarted = false;
	return renderResult;
}

void QVk_BeginRenderpass(qvkrenderpasstype_t rpType)
{
	const VkClearValue clearColors[3] = {
		{.color = {.float32 = { 1.f, .0f, .5f, 1.f } } },
		{.depthStencil = { 1.f, 0 } },
		{.color = {.float32 = { 1.f, .0f, .5f, 1.f } } },
	};

	const VkClearValue warpClearColors[2] = {
		clearColors[0],
		{.color = {.float32 = { 0.f, .0f, .0f, 1.f } } },
	};

	const VkClearValue uiClearColors[3] = {
		clearColors[0],
		clearColors[1],
		{.color = {.float32 = { 0.f, .0f, .0f, 1.f } } },
	};

	VkRenderPassBeginInfo renderBeginInfo[] = {
		// RP_WORLD
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = vk_renderpasses[RP_WORLD].rp,
			.framebuffer = vk_framebuffers[RP_WORLD][vk_imageIndex],
			.renderArea.offset = { 0, 0 },
			.renderArea.extent = vk_swapchain.extent,
			.clearValueCount = vk_renderpasses[RP_WORLD].sampleCount != VK_SAMPLE_COUNT_1_BIT ? 3 : 2,
			.pClearValues = clearColors
		},
		// RP_UI
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = vk_renderpasses[RP_UI].rp,
			.framebuffer = vk_framebuffers[RP_UI][vk_imageIndex],
			.renderArea.offset = { 0, 0 },
			.renderArea.extent = vk_swapchain.extent,
			.clearValueCount = 3,
			.pClearValues = uiClearColors
		},
		// RP_WORLD_WARP
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = vk_renderpasses[RP_WORLD_WARP].rp,
			.framebuffer = vk_framebuffers[RP_WORLD_WARP][vk_imageIndex],
			.renderArea.offset = { 0, 0 },
			.renderArea.extent = vk_swapchain.extent,
			.clearValueCount = 2,
			.pClearValues = warpClearColors
		}
	};

	if (rpType == RP_WORLD) {
		QVk_DebugLabelBegin(&vk_commandbuffers[vk_activeBufferIdx], "Draw World", 0.f, 1.f, 0.f);
	}
	if (rpType == RP_UI) {
		QVk_DebugLabelEnd(&vk_commandbuffers[vk_activeBufferIdx]);
		QVk_DebugLabelBegin(&vk_commandbuffers[vk_activeBufferIdx], "Draw UI", 1.f, 1.f, 0.f);
	}
	if (rpType == RP_WORLD_WARP) {
		QVk_DebugLabelEnd(&vk_commandbuffers[vk_activeBufferIdx]);
		QVk_DebugLabelBegin(&vk_commandbuffers[vk_activeBufferIdx], "Draw View Warp", 1.f, 0.f, .5f);
	}

	vkCmdBeginRenderPass(vk_commandbuffers[vk_activeBufferIdx], &renderBeginInfo[rpType], VK_SUBPASS_CONTENTS_INLINE);
	vk_state.current_renderpass = rpType;
}

qboolean QVk_RecreateSwapchain()
{
	VkResult result = VK_SUCCESS;
	vkDeviceWaitIdle( vk_device.logical );

	DestroyFramebuffers();
	DestroyImageViews();

	if (!QVk_CheckExtent())
		return false;

	VK_VERIFY(result = QVk_CreateSwapchain());

	if (result != VK_SUCCESS)
		return false;

	vk_viewport.width = (float)vid.width;
	vk_viewport.height = (float)vid.height;
	vk_scissor.extent = vk_swapchain.extent;

	DestroyDrawBuffers();
	CreateDrawBuffers();

	VK_VERIFY(result = CreateImageViews());

	if (result != VK_SUCCESS)
		return false;

	VK_VERIFY(result = CreateFramebuffers());

	if (result != VK_SUCCESS)
		return false;

	QVk_UpdateTextureSampler(&vk_colorbuffer, S_NEAREST_UNNORMALIZED, false);
	QVk_UpdateTextureSampler(&vk_colorbufferWarp, S_NEAREST_UNNORMALIZED, false);

	vk_recreateSwapchainNeeded = false;
	return true;
}

uint8_t *QVk_GetVertexBuffer(VkDeviceSize size, VkBuffer *dstBuffer, VkDeviceSize *dstOffset)
{
	if (vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset + size > vk_config.vertex_buffer_size)
	{
		vk_config.vertex_buffer_size = Q_max(
			vk_config.vertex_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		R_Printf(PRINT_ALL, "Resizing dynamic vertex buffer to %ukB\n", vk_config.vertex_buffer_size / 1024);
		int swapBufferOffset = vk_swapBuffersCnt[vk_activeSwapBufferIdx];
		vk_swapBuffersCnt[vk_activeSwapBufferIdx] += NUM_DYNBUFFERS;

		if (vk_swapBuffers[vk_activeSwapBufferIdx] == NULL)
		{
			vk_swapBuffers[vk_activeSwapBufferIdx] = malloc(sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);
		}
		else
		{
			vk_swapBuffers[vk_activeSwapBufferIdx] = realloc(vk_swapBuffers[vk_activeSwapBufferIdx], sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);
		}

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			// need unmap before copy to swapBuffers
			buffer_unmap(&vk_dynVertexBuffers[i].resource);
			vk_swapBuffers[vk_activeSwapBufferIdx][swapBufferOffset + i] = vk_dynVertexBuffers[i];

			QVk_CreateVertexBuffer(NULL, vk_config.vertex_buffer_size,
				&vk_dynVertexBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			vk_dynVertexBuffers[i].pMappedData = buffer_map(&vk_dynVertexBuffers[i].resource);

			QVk_DebugSetObjectName((uint64_t)vk_dynVertexBuffers[i].resource.buffer,
				VK_OBJECT_TYPE_BUFFER, va("Dynamic Vertex Buffer #%d", i));
			QVk_DebugSetObjectName((uint64_t)vk_dynVertexBuffers[i].resource.memory,
				VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Vertex Buffer #%d", i));
		}
	}

	*dstOffset = vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset;
	*dstBuffer = vk_dynVertexBuffers[vk_activeDynBufferIdx].resource.buffer;
	vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset += size;

	vk_config.vertex_buffer_usage = vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset;
	if (vk_config.vertex_buffer_max_usage < vk_config.vertex_buffer_usage)
		vk_config.vertex_buffer_max_usage = vk_config.vertex_buffer_usage;

	return (uint8_t *)vk_dynVertexBuffers[vk_activeDynBufferIdx].pMappedData + (*dstOffset);
}

static uint8_t *QVk_GetIndexBuffer(VkDeviceSize size, VkDeviceSize *dstOffset, int currentBufferIdx)
{
	// align to 4 bytes, so that we can reuse the buffer for both VK_INDEX_TYPE_UINT16 and VK_INDEX_TYPE_UINT32
	const uint32_t aligned_size = ROUNDUP(size, 4);

	if (vk_dynIndexBuffers[currentBufferIdx].currentOffset + aligned_size > vk_config.index_buffer_size)
	{
		vk_config.index_buffer_size = Q_max(
			vk_config.index_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		R_Printf(PRINT_ALL, "Resizing dynamic index buffer to %ukB\n", vk_config.index_buffer_size / 1024);
		int swapBufferOffset = vk_swapBuffersCnt[vk_activeSwapBufferIdx];
		vk_swapBuffersCnt[vk_activeSwapBufferIdx] += NUM_DYNBUFFERS;

		if (vk_swapBuffers[vk_activeSwapBufferIdx] == NULL)
			vk_swapBuffers[vk_activeSwapBufferIdx] = malloc(sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);
		else
			vk_swapBuffers[vk_activeSwapBufferIdx] = realloc(vk_swapBuffers[vk_activeSwapBufferIdx], sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			// need unmap before copy to swapBuffers
			buffer_unmap(&vk_dynIndexBuffers[i].resource);
			vk_swapBuffers[vk_activeSwapBufferIdx][swapBufferOffset + i] = vk_dynIndexBuffers[i];

			QVk_CreateIndexBuffer(NULL, vk_config.index_buffer_size,
				&vk_dynIndexBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			vk_dynIndexBuffers[i].pMappedData = buffer_map(&vk_dynIndexBuffers[i].resource);

			QVk_DebugSetObjectName((uint64_t)vk_dynIndexBuffers[i].resource.buffer,
				VK_OBJECT_TYPE_BUFFER, va("Dynamic Index Buffer #%d", i));
			QVk_DebugSetObjectName((uint64_t)vk_dynIndexBuffers[i].resource.memory,
				VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Index Buffer #%d", i));
		}
	}

	*dstOffset = vk_dynIndexBuffers[currentBufferIdx].currentOffset;
	vk_dynIndexBuffers[currentBufferIdx].currentOffset += aligned_size;

	vk_config.index_buffer_usage = vk_dynIndexBuffers[currentBufferIdx].currentOffset;
	if (vk_config.index_buffer_max_usage < vk_config.index_buffer_usage)
		vk_config.index_buffer_max_usage = vk_config.index_buffer_usage;

	return (uint8_t *)vk_dynIndexBuffers[currentBufferIdx].pMappedData + (*dstOffset);
}

uint8_t *QVk_GetUniformBuffer(VkDeviceSize size, uint32_t *dstOffset, VkDescriptorSet *dstUboDescriptorSet)
{
	// 0x100 alignment is required by Vulkan spec
	const uint32_t aligned_size = ROUNDUP(size, 0x100);

	if (vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset + UNIFORM_ALLOC_SIZE > vk_config.uniform_buffer_size)
	{
		vk_config.uniform_buffer_size = Q_max(
			vk_config.uniform_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		R_Printf(PRINT_ALL, "Resizing dynamic uniform buffer to %ukB\n", vk_config.uniform_buffer_size / 1024);
		int swapBufferOffset   = vk_swapBuffersCnt[vk_activeSwapBufferIdx];
		int swapDescSetsOffset = vk_swapDescSetsCnt[vk_activeSwapBufferIdx];
		vk_swapBuffersCnt[vk_activeSwapBufferIdx]  += NUM_DYNBUFFERS;
		vk_swapDescSetsCnt[vk_activeSwapBufferIdx] += NUM_DYNBUFFERS;

		if (vk_swapBuffers[vk_activeSwapBufferIdx] == NULL)
			vk_swapBuffers[vk_activeSwapBufferIdx] = malloc(sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);
		else
			vk_swapBuffers[vk_activeSwapBufferIdx] = realloc(vk_swapBuffers[vk_activeSwapBufferIdx], sizeof(qvkbuffer_t) * vk_swapBuffersCnt[vk_activeSwapBufferIdx]);

		if (vk_swapDescriptorSets[vk_activeSwapBufferIdx] == NULL)
			vk_swapDescriptorSets[vk_activeSwapBufferIdx] = malloc(sizeof(VkDescriptorSet) * vk_swapDescSetsCnt[vk_activeSwapBufferIdx]);
		else
			vk_swapDescriptorSets[vk_activeSwapBufferIdx] = realloc(vk_swapDescriptorSets[vk_activeSwapBufferIdx], sizeof(VkDescriptorSet) * vk_swapDescSetsCnt[vk_activeSwapBufferIdx]);

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			// need unmap before copy to swapBuffers
			buffer_unmap(&vk_dynUniformBuffers[i].resource);
			vk_swapBuffers[vk_activeSwapBufferIdx][swapBufferOffset + i] = vk_dynUniformBuffers[i];
			vk_swapDescriptorSets[vk_activeSwapBufferIdx][swapDescSetsOffset + i] = vk_uboDescriptorSets[i];

			VK_VERIFY(QVk_CreateUniformBuffer(vk_config.uniform_buffer_size,
				&vk_dynUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
			vk_dynUniformBuffers[i].pMappedData = buffer_map(&vk_dynUniformBuffers[i].resource);
			CreateUboDescriptorSet(&vk_uboDescriptorSets[i],
				vk_dynUniformBuffers[i].resource.buffer);

			QVk_DebugSetObjectName((uint64_t)vk_uboDescriptorSets[i],
				VK_OBJECT_TYPE_DESCRIPTOR_SET, va("Dynamic UBO Descriptor Set #%d", i));
			QVk_DebugSetObjectName((uint64_t)vk_dynUniformBuffers[i].resource.buffer,
				VK_OBJECT_TYPE_BUFFER, va("Dynamic Uniform Buffer #%d", i));
			QVk_DebugSetObjectName((uint64_t)vk_dynUniformBuffers[i].resource.memory,
				VK_OBJECT_TYPE_DEVICE_MEMORY, va("Memory: Dynamic Uniform Buffer #%d", i));
		}
	}

	*dstOffset = vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset;
	*dstUboDescriptorSet = vk_uboDescriptorSets[vk_activeDynBufferIdx];
	vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset += aligned_size;

	vk_config.uniform_buffer_usage = vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset;
	if (vk_config.uniform_buffer_max_usage < vk_config.uniform_buffer_usage)
		vk_config.uniform_buffer_max_usage = vk_config.uniform_buffer_usage;

	return (uint8_t *)vk_dynUniformBuffers[vk_activeDynBufferIdx].pMappedData + (*dstOffset);
}

uint8_t *QVk_GetStagingBuffer(VkDeviceSize size, int alignment, VkCommandBuffer *cmdBuffer, VkBuffer *buffer, uint32_t *dstOffset)
{
	qvkstagingbuffer_t * stagingBuffer = &vk_stagingBuffers[vk_activeStagingBuffer];
	stagingBuffer->currentOffset = ROUNDUP(stagingBuffer->currentOffset, alignment);

	if (((stagingBuffer->currentOffset + size) >= stagingBuffer->resource.size) && !stagingBuffer->submitted)
		SubmitStagingBuffer(vk_activeStagingBuffer);

	stagingBuffer = &vk_stagingBuffers[vk_activeStagingBuffer];
	if (size > stagingBuffer->resource.size)
	{
		R_Printf(PRINT_ALL, "%s: %d: Resize stanging buffer " YQ2_COM_PRIdS "-> " YQ2_COM_PRIdS "\n",
			__func__, vk_activeStagingBuffer, stagingBuffer->resource.size, size);

		DestroyStagingBuffer(stagingBuffer);
		CreateStagingBuffer(size, stagingBuffer, vk_activeStagingBuffer);
	}
	else
	{
		if (stagingBuffer->submitted)
		{
			VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &stagingBuffer->fence, VK_TRUE, UINT64_MAX));
			VK_VERIFY(vkResetFences(vk_device.logical, 1, &stagingBuffer->fence));

			stagingBuffer->currentOffset = 0;
			stagingBuffer->submitted = false;

			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = NULL,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = NULL
			};

			// Command buffers are implicitly reset by vkBeginCommandBuffer if VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT is set - this is expensive.
			// It's more efficient to reset entire pool instead and it also fixes the VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT performance warning.
			// see also: https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/performance/command_buffer_usage/command_buffer_usage_tutorial.md
			vkResetCommandPool(vk_device.logical, vk_stagingCommandPool[vk_activeStagingBuffer], 0);
			VK_VERIFY(vkBeginCommandBuffer(stagingBuffer->cmdBuffer, &beginInfo));
		}
	}

	if (cmdBuffer)
		*cmdBuffer = stagingBuffer->cmdBuffer;
	if (buffer)
		*buffer = stagingBuffer->resource.buffer;
	if (dstOffset)
		*dstOffset = stagingBuffer->currentOffset;

	unsigned char *data = (uint8_t *)stagingBuffer->pMappedData + stagingBuffer->currentOffset;
	stagingBuffer->currentOffset += size;

	return data;
}

static void
QVk_CheckTriangleIbo(VkDeviceSize indexCount)
{
	if (indexCount > vk_config.triangle_index_usage)
		vk_config.triangle_index_usage = indexCount;

	if (vk_config.triangle_index_usage > vk_config.triangle_index_max_usage)
		vk_config.triangle_index_max_usage = vk_config.triangle_index_usage;

	if (indexCount > vk_config.triangle_index_count)
	{
		vk_config.triangle_index_count *= BUFFER_RESIZE_FACTOR;
		R_Printf(PRINT_ALL, "Resizing triangle index buffer to %u indices.\n", vk_config.triangle_index_count);
		RebuildTriangleIndexBuffer();
	}
}

VkBuffer QVk_GetTriangleFanIbo(VkDeviceSize indexCount)
{
	QVk_CheckTriangleIbo(indexCount);

	return *vk_triangleFanIbo;
}

VkBuffer QVk_GetTriangleStripIbo(VkDeviceSize indexCount)
{
	QVk_CheckTriangleIbo(indexCount);

	return *vk_triangleStripIbo;
}

void QVk_SubmitStagingBuffers()
{
	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		if (!vk_stagingBuffers[i].submitted && vk_stagingBuffers[i].currentOffset > 0)
			SubmitStagingBuffer(i);
	}
}

VkSampler QVk_UpdateTextureSampler(qvktexture_t *texture, qvksampler_t samplerType, qboolean clampToEdge)
{
	const int samplerIndex = samplerType + (clampToEdge ? S_SAMPLER_CNT : 0);

	assert((vk_samplers[samplerIndex] != VK_NULL_HANDLE) && "Sampler is VK_NULL_HANDLE!");

	texture->clampToEdge = clampToEdge;

	VkDescriptorImageInfo dImgInfo = {
		.sampler = vk_samplers[samplerIndex],
		.imageView = texture->imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = NULL,
		.dstSet = texture->descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &dImgInfo,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL
	};

	vkUpdateDescriptorSets(vk_device.logical, 1, &writeSet, 0, NULL);

	return vk_samplers[samplerIndex];
}

void QVk_DrawColorRect(float *ubo, VkDeviceSize uboSize, qvkrenderpasstype_t rpType)
{
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData = QVk_GetUniformBuffer(uboSize,
		&uboOffset, &uboDescriptorSet);
	memcpy(vertData, ubo, uboSize);

	QVk_BindPipeline(&vk_drawColorQuadPipeline[rpType]);
	VkDeviceSize offsets = 0;
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_drawColorQuadPipeline[rpType].layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1,
		&vk_colorRectVbo.resource.buffer, &offsets);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, vk_rectIbo.resource.buffer,
		0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
}

void QVk_DrawTexRect(const float *ubo, VkDeviceSize uboSize, qvktexture_t *texture)
{
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(uboSize, &uboOffset, &uboDescriptorSet);
	memcpy(uboData, ubo, uboSize);

	QVk_BindPipeline(&vk_drawTexQuadPipeline[vk_state.current_renderpass]);
	VkDeviceSize offsets = 0;
	VkDescriptorSet descriptorSets[] = {
		texture->descriptorSet,
		uboDescriptorSet
	};

	float gamma = 2.1F - vid_gamma->value;

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(gamma), &gamma);

	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout, 0, 2, descriptorSets, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1,
		&vk_texRectVbo.resource.buffer, &offsets);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer,
		vk_rectIbo.resource.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
}

void QVk_BindPipeline(qvkpipeline_t *pipeline)
{
	if (vk_state.current_pipeline != pipeline->pl)
	{
		vkCmdBindPipeline(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pl);
		vk_state.current_pipeline = pipeline->pl;
	}
}

const char *QVk_GetError(VkResult errorCode)
{
#define ERRSTR(r) case VK_ ##r: return "VK_"#r
	switch (errorCode)
	{
		ERRSTR(SUCCESS);
		ERRSTR(NOT_READY);
		ERRSTR(TIMEOUT);
		ERRSTR(EVENT_SET);
		ERRSTR(EVENT_RESET);
		ERRSTR(INCOMPLETE);
		ERRSTR(ERROR_OUT_OF_HOST_MEMORY);
		ERRSTR(ERROR_OUT_OF_DEVICE_MEMORY);
		ERRSTR(ERROR_INITIALIZATION_FAILED);
		ERRSTR(ERROR_DEVICE_LOST);
		ERRSTR(ERROR_MEMORY_MAP_FAILED);
		ERRSTR(ERROR_LAYER_NOT_PRESENT);
		ERRSTR(ERROR_EXTENSION_NOT_PRESENT);
		ERRSTR(ERROR_FEATURE_NOT_PRESENT);
		ERRSTR(ERROR_INCOMPATIBLE_DRIVER);
		ERRSTR(ERROR_TOO_MANY_OBJECTS);
		ERRSTR(ERROR_FORMAT_NOT_SUPPORTED);
		ERRSTR(ERROR_SURFACE_LOST_KHR);
		ERRSTR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		ERRSTR(SUBOPTIMAL_KHR);
		ERRSTR(ERROR_OUT_OF_DATE_KHR);
		ERRSTR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		ERRSTR(ERROR_VALIDATION_FAILED_EXT);
		ERRSTR(ERROR_INVALID_SHADER_NV);
		default: return "<unknown>";
	}
#undef ERRSTR
	return "UNKNOWN ERROR";
}
