#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "../../Utils/Logging.h"

namespace Renderer
{
	class Buffer
	{
	public:
		VkBuffer buffer;
		VmaAllocation bufferAllocation;
		VmaAllocator* allocator;
		VkDeviceSize size = 0;
		void* mappedData = nullptr;

	public:
		Buffer(VmaAllocator* allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
		       VmaMemoryUsage memUsage)
		{
			this->allocator = allocator;
			VkBufferCreateInfo buffCreateInfo = {};
			buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffCreateInfo.size = size;
			buffCreateInfo.usage = usage;
			buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.usage = memUsage;
			allocCreateInfo.requiredFlags = props;

			auto success = vmaCreateBuffer(*allocator, &buffCreateInfo, &allocCreateInfo, &buffer, &bufferAllocation,
			                               nullptr);
			Assert(success == VK_SUCCESS, "Failed to create Buffer");
		}

		~Buffer() { vmaDestroyBuffer(*allocator, buffer, bufferAllocation); }

		void* map()
		{
			vmaMapMemory(*allocator, bufferAllocation, &mappedData);

			return mappedData;
		}

		void unMap() const { vmaUnmapMemory(*allocator, bufferAllocation); }

		void cleanup() { vmaDestroyBuffer(*allocator, buffer, bufferAllocation); }
	};
}
