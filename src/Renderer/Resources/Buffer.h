#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "../../Utils/Logging.h"

namespace Renderer
{
	class Buffer
	{
		
		VkBuffer buffer;
		VkBufferUsageFlags usage;
		VmaMemoryUsage memUsage;
		VmaAllocation bufferAllocation;
		VmaAllocator* allocator;
		VkDeviceSize size = 0;
		uint8_t* mappedData = nullptr;

		public:
			operator VkBuffer() { return buffer; }
		
			Buffer(const Buffer&) = delete;
			Buffer& operator=(const Buffer&) = delete;
			Buffer& operator=(Buffer&&) = delete;

			Buffer(VmaAllocator* allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
			{
				this->allocator = allocator;
				this->size = size;
				this->usage = usage;
				this->memUsage = memUsage;

				VkBufferCreateInfo buffCreateInfo = {};
				buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffCreateInfo.size = size;
				buffCreateInfo.usage = usage;
				buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = memUsage;

				auto success = vmaCreateBuffer(*allocator, &buffCreateInfo, &allocCreateInfo, &buffer, &bufferAllocation, nullptr);
				Assert(success == VK_SUCCESS, "Failed to create Buffer");
			}

			~Buffer() { vmaDestroyBuffer(*allocator, buffer, bufferAllocation); }

			VkBuffer& getBuffer() { return buffer; }
			VkBufferUsageFlags getUsage() { return usage; }
			VmaMemoryUsage getMemUsage() { return memUsage; }
			VkDeviceSize getSize() { return size; }

			uint8_t* map()
			{
				vmaMapMemory(*allocator, bufferAllocation, reinterpret_cast<void**>(&mappedData));

				return mappedData;
			}

			void reSize(const size_t size)
			{
				vmaDestroyBuffer(*allocator, buffer, bufferAllocation);
				this->size = size;

				VkBufferCreateInfo buffCreateInfo = {};
				buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffCreateInfo.size = size;
				buffCreateInfo.usage = usage;
				buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = memUsage;

				auto success = vmaCreateBuffer(*allocator, &buffCreateInfo, &allocCreateInfo, &buffer, &bufferAllocation, nullptr);

				Assert(success == VK_SUCCESS, "Failed to re-create Buffer");
			}

			void load(const uint8_t* data, const size_t size, const size_t offset = 0)
			{
				if (size > this->size) reSize(size);

				uint8_t* dst = map();
				memcpy(dst + offset, data, size);
				unMap();
			}

			void load(void* data, const size_t size, const size_t offset = 0)
			{
				load(reinterpret_cast<const uint8_t*>(data), size, offset);
			}

			void unMap() const
			{
				vmaUnmapMemory(*allocator, bufferAllocation);
			}
	};
}
