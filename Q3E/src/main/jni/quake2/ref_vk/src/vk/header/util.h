/*
 * Copyright (C) 2018 Christoph Schied
 * Copyright (C) 2020 Denis Pauk
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
 */

#ifndef  __VK_UTIL_H__
#define  __VK_UTIL_H__

#ifdef __APPLE__
#define VOLK_VULKAN_H_PATH <MoltenVK/vk_mvk_moltenvk.h>
#endif
#include "../volk/volk.h"

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

typedef struct BufferResource_s {
	VkBuffer buffer;
	// shared memory used for buffer
	VkDeviceMemory memory;
	// image size
	VkDeviceSize size;
	// posision in shared memory
	VkDeviceSize offset;
	// is mapped?
	VkBool32 is_mapped;
} BufferResource_t;

typedef struct ImageResource_s {
	VkImage image;
	// shared memory used for image
	VkDeviceMemory memory;
	// image size
	VkDeviceSize size;
	// posision in shared memory
	VkDeviceSize offset;
} ImageResource_t;

VkResult buffer_create(BufferResource_t *buf,
		VkBufferCreateInfo buf_create_info,
		VkMemoryPropertyFlags mem_properties,
		VkMemoryPropertyFlags mem_preferences,
		VkMemoryPropertyFlags mem_skip);

VkResult buffer_destroy(BufferResource_t *buf);
void buffer_unmap(BufferResource_t *buf);
void *buffer_map(BufferResource_t *buf);
VkResult buffer_flush(const BufferResource_t *buf);
VkResult buffer_invalidate(const BufferResource_t *buf);

VkResult image_create(ImageResource_t *img,
		VkImageCreateInfo img_create_info,
		VkMemoryPropertyFlags mem_properties,
		VkMemoryPropertyFlags mem_preferences,
		VkMemoryPropertyFlags mem_skip);
VkResult image_destroy(ImageResource_t *img);

void vulkan_memory_init(void);
void vulkan_memory_types_show(void);
void vulkan_memory_free_unused(void);
void vulkan_memory_delete(void);

#endif  /*__VK_UTIL_H__*/
