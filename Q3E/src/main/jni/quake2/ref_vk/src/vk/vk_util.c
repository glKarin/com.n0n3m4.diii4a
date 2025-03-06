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

#include "header/util.h"
#include "header/local.h"

#include <assert.h>

/*
Returns number of bits set to 1 in (v).

On specific platforms and compilers you can use instrinsics like:

Visual Studio:
    return __popcnt(v);
GCC, Clang:
    return static_cast<uint32_t>(__builtin_popcount(v));
*/
static inline uint32_t count_bits_set(uint32_t v)
{
#ifdef __builtin_popcount
	return __builtin_popcount(v);
#else
	uint32_t c = v - ((v >> 1) & 0x55555555);
	c = ((c >> 2) & 0x33333333) + (c & 0x33333333);
	c = ((c >> 4) + c) & 0x0F0F0F0F;
	c = ((c >> 8) + c) & 0x00FF00FF;
	c = ((c >> 16) + c) & 0x0000FFFF;
	return c;
#endif
}

/*
 * Check:
 *   https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
 * for more information.
 */
static uint32_t
get_memory_type(uint32_t mem_req_type_bits,
								  VkMemoryPropertyFlags mem_prop,
								  VkMemoryPropertyFlags mem_pref,
								  VkMemoryPropertyFlags mem_skip)
{
	uint32_t mem_type_index = VK_MAX_MEMORY_TYPES;
	int max_cost = -1;

	// update prefered with required
	mem_pref |= mem_prop;
	// skip for host visible memory
	if (mem_pref & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		mem_skip |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	for(uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if(mem_req_type_bits & (1 << i)) {
			VkMemoryPropertyFlags propertyFlags;

			// cache flags
			propertyFlags = vk_device.mem_properties.memoryTypes[i].propertyFlags;
			// This memory type contains mem_prop and no mem_skip
			if(
				(propertyFlags & mem_prop) == mem_prop &&
				!(propertyFlags & mem_skip)
			)
			{
				int curr_cost;

				// Calculate cost as number of bits from preferredFlags
				// not present in this memory type.
				curr_cost = count_bits_set(propertyFlags & mem_pref);
				// Remember memory type with upper cost as has more prefered bits.
				if(curr_cost > max_cost)
				{
					mem_type_index = i;
					max_cost = curr_cost;
				}
			}
		}
	}
	return mem_type_index;
}

typedef struct MemoryResource_s {
	// type of memory
	uint32_t memory_type;
	// offset step
	VkDeviceSize alignment;
	// id memory used
	VkBool32 used;
	// suballocate
	VkBool32 suballocate;
	// shared memory used for image
	VkDeviceMemory memory;
	// image size
	VkDeviceSize size;
	// posision in shared memory
	VkDeviceSize offset;
} MemoryResource_t;

// 1MB buffers / 512 x 512 * RGBA
#define MEMORY_THRESHOLD (512 * 512 * 4)

static VkDeviceSize memory_block_threshold;
static MemoryResource_t *used_memory;
static VkDeviceSize used_memory_size;

void
vulkan_memory_init(void)
{
	memory_block_threshold = MEMORY_THRESHOLD;
	used_memory_size = 1024; // Size of buffers history
	used_memory = calloc(used_memory_size, sizeof(MemoryResource_t));
}

static void
memory_type_print(VkMemoryPropertyFlags mem_prop)
{
	if (!mem_prop)
	{
		R_Printf(PRINT_ALL, " VK_MEMORY_PROPERTY_NONE");
		return;
	}

#define MPSTR(r, prop) \
	if((prop & VK_MEMORY_PROPERTY_ ##r) != 0) \
		{ R_Printf(PRINT_ALL, " %s", "VK_MEMORY_PROPERTY_"#r); }; \

	MPSTR(DEVICE_LOCAL_BIT, mem_prop);
	MPSTR(HOST_VISIBLE_BIT, mem_prop);
	MPSTR(HOST_COHERENT_BIT, mem_prop);
	MPSTR(HOST_CACHED_BIT, mem_prop);
	MPSTR(LAZILY_ALLOCATED_BIT, mem_prop);
	MPSTR(PROTECTED_BIT, mem_prop);
	MPSTR(DEVICE_COHERENT_BIT_AMD, mem_prop);
	MPSTR(DEVICE_UNCACHED_BIT_AMD, mem_prop);
#ifdef VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV
	MPSTR(RDMA_CAPABLE_BIT_NV, mem_prop);
#endif

#undef PMSTR
}

void
vulkan_memory_types_show(void)
{
	R_Printf(PRINT_ALL, "\nMemory blocks:");

	for(uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if (vk_device.mem_properties.memoryTypes[i].propertyFlags)
		{
			R_Printf(PRINT_ALL, "\n   #%d:", i);
			memory_type_print(vk_device.mem_properties.memoryTypes[i].propertyFlags);
		}
	}
	R_Printf(PRINT_ALL, "\n");
}

static VkBool32
vulkan_memory_is_used(int start_pos, int end_pos, VkDeviceMemory memory)
{
	int pos;

	for (pos = start_pos; pos < end_pos; pos++)
	{
		if (used_memory[pos].memory == memory && used_memory[pos].used)
			return VK_TRUE;
	}

	return VK_FALSE;
}

void
vulkan_memory_free_unused(void)
{
	int pos_global;

	for (pos_global = 0; pos_global < used_memory_size; pos_global ++)
	{
		VkDeviceMemory memory = used_memory[pos_global].memory;
		if (memory != VK_NULL_HANDLE && !used_memory[pos_global].used)
		{
			int pos_local;

			// is used somewhere else after
			if (vulkan_memory_is_used(pos_global, used_memory_size, memory))
				continue;

			// is used somewhere else before
			if (vulkan_memory_is_used(0, pos_global, memory))
				continue;

			// free current memory block
			vkFreeMemory(vk_device.logical, memory, NULL);
			memset(&used_memory[pos_global], 0, sizeof(MemoryResource_t));

			// cleanup same block
			for (pos_local = pos_global + 1; pos_local < used_memory_size; pos_local++)
			{
				if (used_memory[pos_local].memory == memory)
				{
					memset(&used_memory[pos_local], 0, sizeof(MemoryResource_t));
				}
			}

		}
	}
}

void
vulkan_memory_delete(void)
{
	int pos_global;
	for (pos_global = 0; pos_global < used_memory_size; pos_global ++)
	{
		VkDeviceMemory memory = used_memory[pos_global].memory;
		if (memory != VK_NULL_HANDLE)
		{
			int pos_local;

			// free current memory block
			vkFreeMemory(vk_device.logical, memory, NULL);
			memset(&used_memory[pos_global], 0, sizeof(MemoryResource_t));

			// cleanup same block
			for (pos_local = pos_global + 1; pos_local < used_memory_size; pos_local++)
			{
				if (used_memory[pos_local].memory == memory)
				{
					memset(&used_memory[pos_local], 0, sizeof(MemoryResource_t));
				}
			}
		}
	}
	free(used_memory);
}

static VkResult
memory_block_min(VkDeviceSize size,
		uint32_t memory_type,
		VkDeviceSize alignment,
		VkBool32 suballocate,
		int* block_pos)
{
	int pos;
	VkDeviceSize min_size = memory_block_threshold;
	VkResult result = VK_ERROR_OUT_OF_DEVICE_MEMORY;

	// update max_size
	if (min_size < size)
	{
		*block_pos = -1;
		return result;
	}

	// search minimal posible size
	for (pos = 0; pos < used_memory_size; pos ++)
	{
		if (used_memory[pos].memory_type == memory_type &&
			used_memory[pos].suballocate == suballocate &&
			used_memory[pos].alignment == alignment &&
			used_memory[pos].memory != VK_NULL_HANDLE &&
			used_memory[pos].used == VK_FALSE &&
			used_memory[pos].size < min_size &&
			used_memory[pos].size >= size)
		{
			// save minimal size
			min_size = used_memory[pos].size;
			*block_pos = pos;
			result = VK_SUCCESS;
		}
	}

	return result;
}

static VkResult
memory_block_empty(int *block_pos)
{
	int pos;
	MemoryResource_t *memory;

	// search empty memory
	for (pos = *block_pos; pos < used_memory_size; pos ++)
	{
		if (used_memory[pos].memory == VK_NULL_HANDLE)
		{
			*block_pos = pos;
			return VK_SUCCESS;
		}
	}

	memory = realloc(used_memory, (used_memory_size * 2) * sizeof(MemoryResource_t));
	if (!memory)
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;

	// use previous end
	*block_pos = used_memory_size;

	// update old struct
	memset(memory + used_memory_size, 0, used_memory_size * sizeof(MemoryResource_t));
	used_memory_size *= 2;
	used_memory = memory;

	return VK_SUCCESS;
}

static VkResult
memory_block_allocate(VkDeviceSize size,
		uint32_t memory_type,
		VkDeviceSize alignment,
		VkBool32 suballocate,
		int *block_pos)
{
	int pos = 0;
	if (memory_block_empty(&pos) == VK_SUCCESS)
	{
		VkResult result;
		VkDeviceMemory memory;

		if (size < MEMORY_THRESHOLD)
			size = MEMORY_THRESHOLD;

		// allocate only aligned
		size = ROUNDUP(size, alignment);

		// Need to split only buffers with suballocate support
		if (suballocate)
		{
			// requested bigger then usual
			if (size > memory_block_threshold)
			{
				size *= 2;
				// up threshold for next allocations
				memory_block_threshold = size;
			}
			// allcate bigger memory for reuse
			else if (size < memory_block_threshold)
			{
				size = memory_block_threshold;
			}
		}

		VkMemoryAllocateInfo mem_alloc_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = size,
			.memoryTypeIndex = memory_type
		};

		result = vkAllocateMemory(vk_device.logical, &mem_alloc_info, NULL, &memory);
		if (result == VK_SUCCESS)
		{
			used_memory[pos].memory = memory;
			used_memory[pos].memory_type = memory_type;
			used_memory[pos].alignment = alignment;
			used_memory[pos].offset = 0;
			used_memory[pos].size = size;
			used_memory[pos].suballocate = suballocate;
			used_memory[pos].used = VK_FALSE;
			*block_pos = pos;
		}
		else
		{
			R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
				__func__, __LINE__, QVk_GetError(result));
		}
		return result;
	}

	return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

static VkResult
memory_create(VkDeviceSize size,
		uint32_t memory_type,
		VkBool32 suballocate,
		VkDeviceSize alignment,
		VkDeviceMemory *memory,
		VkDeviceSize *offset)
{
	int pos = -1;
	VkResult result;
	result = memory_block_min(size, memory_type, alignment, suballocate, &pos);

	if (result != VK_SUCCESS)
	{
		result = memory_block_allocate(size, memory_type, alignment, suballocate, &pos);
	}
	if (result == VK_SUCCESS)
	{
		// check size of block,
		// new block should be at least same size as current
		// and bigger than double minimal offset
		// and marked as not for mmap
		if (used_memory[pos].size > (size * 2) &&
			(used_memory[pos].size > (used_memory[pos].alignment * 2)) &&
			used_memory[pos].suballocate)
		{
			// search from next slot
			int new_pos = pos + 1;
			result = memory_block_empty(&new_pos);
			if (result == VK_SUCCESS)
			{
				VkDeviceSize new_size = ROUNDUP(size, used_memory[pos].alignment);

				// split to several blocks
				memmove(&used_memory[new_pos], &used_memory[pos], sizeof(MemoryResource_t));
				used_memory[new_pos].offset = used_memory[pos].offset + new_size;
				used_memory[new_pos].size = used_memory[pos].size - new_size;

				// save new size to block, it can be bigger than required
				used_memory[pos].size = used_memory[new_pos].offset - used_memory[pos].offset;
				assert(used_memory[pos].size > 0);
			}
		}

		used_memory[pos].used = VK_TRUE;
		*offset = used_memory[pos].offset;
		*memory = used_memory[pos].memory;
		return result;
	}

	return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

static void
memory_destroy(VkDeviceMemory memory, VkDeviceSize offset)
{
	int pos;
	for (pos = 0; pos < used_memory_size; pos ++)
	{
		if (used_memory[pos].memory == memory && used_memory[pos].offset == offset)
		{
			used_memory[pos].used = VK_FALSE;
			return;
		}
	}
	// looks as no such memory registered
	vkFreeMemory(vk_device.logical, memory, NULL);
}

static VkResult
memory_create_by_property(VkMemoryRequirements* mem_reqs,
		VkMemoryPropertyFlags mem_properties,
		VkMemoryPropertyFlags mem_preferences,
		VkMemoryPropertyFlags mem_skip,
		VkDeviceMemory *memory,
		VkDeviceSize *offset)
{
	VkMemoryPropertyFlags host_visible;
	uint32_t memory_index;

	if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "Asked about memory properties with:\n");
		memory_type_print(mem_properties);
		R_Printf(PRINT_ALL, "\nAsked about memory preferences with:\n");
		memory_type_print(mem_preferences);
		R_Printf(PRINT_ALL, "\nAsked about memory skip with:\n");
		memory_type_print(mem_skip);
		R_Printf(PRINT_ALL, "\n");
	}

	memory_index = get_memory_type(mem_reqs->memoryTypeBits,
		mem_properties, mem_preferences, mem_skip);

	if (memory_index == VK_MAX_MEMORY_TYPES)
	{
		R_Printf(PRINT_ALL, "%s:%d: Have not found required memory type.\n",
			__func__, __LINE__);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}
	else if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "%s:%d: Selected %d memory properties with:\n",
			__func__, __LINE__, memory_index);
		memory_type_print(
			vk_device.mem_properties.memoryTypes[memory_index].propertyFlags);
		R_Printf(PRINT_ALL, "\n");
	}

	/* get selected memory properties */
	mem_properties = vk_device.mem_properties.memoryTypes[memory_index].propertyFlags &
		(mem_properties | mem_preferences);

	host_visible = mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	return memory_create(mem_reqs->size, memory_index,
		// suballocate allowed
		host_visible != VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		mem_reqs->alignment, memory, offset);
}

VkResult
buffer_create(BufferResource_t *buf,
		VkBufferCreateInfo buf_create_info,
		VkMemoryPropertyFlags mem_properties,
		VkMemoryPropertyFlags mem_preferences,
		VkMemoryPropertyFlags mem_skip)
{
	assert(buf_create_info.size > 0);
	assert(buf);
	VkResult result = VK_SUCCESS;

	buf->size = buf_create_info.size;
	buf->is_mapped = VK_FALSE;

	result = vkCreateBuffer(vk_device.logical, &buf_create_info, NULL, &buf->buffer);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_buffer;
	}
	assert(buf->buffer != VK_NULL_HANDLE);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(vk_device.logical, buf->buffer, &mem_reqs);

	result = memory_create_by_property(&mem_reqs, mem_properties, mem_preferences,
		mem_skip, &buf->memory, &buf->offset);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_mem_alloc;
	}

	assert(buf->memory != VK_NULL_HANDLE);

	result = vkBindBufferMemory(vk_device.logical, buf->buffer, buf->memory, buf->offset);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_bind_buf_memory;
	}

	return VK_SUCCESS;

fail_bind_buf_memory:
	memory_destroy(buf->memory, buf->offset);
fail_mem_alloc:
	vkDestroyBuffer(vk_device.logical, buf->buffer, NULL);
fail_buffer:
	buf->buffer = VK_NULL_HANDLE;
	buf->memory = VK_NULL_HANDLE;
	buf->size   = 0;
	return result;
}

VkResult
image_create(ImageResource_t *img,
		VkImageCreateInfo img_create_info,
		VkMemoryPropertyFlags mem_properties,
		VkMemoryPropertyFlags mem_preferences,
		VkMemoryPropertyFlags mem_skip)
{
	assert(img);
	VkResult result = VK_SUCCESS;

	result = vkCreateImage(vk_device.logical, &img_create_info, NULL, &img->image);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_buffer;
	}
	assert(img->image != VK_NULL_HANDLE);

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(vk_device.logical, img->image, &mem_reqs);
	img->size = mem_reqs.size;

	result = memory_create_by_property(&mem_reqs, mem_properties, mem_preferences,
		mem_skip, &img->memory, &img->offset);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_mem_alloc;
	}

	assert(img->memory != VK_NULL_HANDLE);

	result = vkBindImageMemory(vk_device.logical, img->image, img->memory, img->offset);
	if(result != VK_SUCCESS) {
		R_Printf(PRINT_ALL, "%s:%d: VkResult verification: %s\n",
			__func__, __LINE__, QVk_GetError(result));
		goto fail_bind_buf_memory;
	}

	return VK_SUCCESS;

fail_bind_buf_memory:
	memory_destroy(img->memory, img->offset);
fail_mem_alloc:
	vkDestroyImage(vk_device.logical, img->image, NULL);
fail_buffer:
	img->image = VK_NULL_HANDLE;
	img->memory = VK_NULL_HANDLE;
	img->size   = 0;
	return result;
}

VkResult
buffer_destroy(BufferResource_t *buf)
{
	assert(!buf->is_mapped);

	// buffer should be destroed before bound memory
	if(buf->buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vk_device.logical, buf->buffer, NULL);
		buf->buffer = VK_NULL_HANDLE;
	}

	// buffer desroed, we can free up memory
	if(buf->memory != VK_NULL_HANDLE)
	{
		memory_destroy(buf->memory, buf->offset);
		buf->memory = VK_NULL_HANDLE;
	}

	memset(buf, 0, sizeof(BufferResource_t));

	return VK_SUCCESS;
}

VkResult
image_destroy(ImageResource_t *img)
{
	// image should be destroed before bound memory
	if(img->image != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk_device.logical, img->image, NULL);
		img->image = VK_NULL_HANDLE;
	}

	// image destroed, we can free up memory
	if(img->memory != VK_NULL_HANDLE)
	{
		memory_destroy(img->memory, img->offset);
		img->memory = VK_NULL_HANDLE;
	}

	memset(img, 0, sizeof(ImageResource_t));

	return VK_SUCCESS;
}

VkResult
buffer_flush(BufferResource_t *buf)
{
	VkResult result = VK_SUCCESS;

	VkMappedMemoryRange ranges[1] = {{
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.memory = buf->memory,
		.offset = buf->offset,
		.size = buf->size
	}};
	result = vkFlushMappedMemoryRanges(vk_device.logical, 1, ranges);
	return result;
}

VkResult
buffer_invalidate(BufferResource_t *buf)
{
	VkResult result = VK_SUCCESS;

	VkMappedMemoryRange ranges[1] = {{
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.memory = buf->memory,
		.offset = buf->offset,
		.size = buf->size
	}};
	result = vkInvalidateMappedMemoryRanges(vk_device.logical, 1, ranges);
	return result;

}

void *
buffer_map(BufferResource_t *buf)
{
	assert(buf->memory);
	assert(!buf->is_mapped);
	buf->is_mapped = VK_TRUE;
	void *ret = NULL;
	assert(buf->memory != VK_NULL_HANDLE);
	assert(buf->size > 0);
	VK_VERIFY(vkMapMemory(vk_device.logical, buf->memory,
		buf->offset/*offset*/, buf->size, 0 /*flags*/, &ret));
	return ret;
}

void
buffer_unmap(BufferResource_t *buf)
{
	assert(buf->memory);
	assert(buf->is_mapped);
	buf->is_mapped = VK_FALSE;
	vkUnmapMemory(vk_device.logical, buf->memory);
}
