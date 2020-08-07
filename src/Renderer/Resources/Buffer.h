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
		VkMemoryPropertyFlags props;
		VmaMemoryUsage memUsage;
		VmaAllocation bufferAllocation;
		VmaAllocator* allocator;
		VkDeviceSize size = 0;
		uint8_t* mappedData = nullptr;

		operator VkBuffer() { return buffer; }

		public:
			Buffer(VmaAllocator* allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
			{
				this->allocator = allocator;
				this->size = size;
				this->usage = usage;
				this->props = props;
				this->memUsage = memUsage;

				VkBufferCreateInfo buffCreateInfo = {};
				buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffCreateInfo.size = size;
				buffCreateInfo.usage = usage;
				buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = memUsage;
				//allocCreateInfo.requiredFlags = props;

				auto success = vmaCreateBuffer(*allocator, &buffCreateInfo, &allocCreateInfo, &buffer, &bufferAllocation, nullptr);
				Assert(success == VK_SUCCESS, "Failed to create Buffer");
			}

			~Buffer()
			{
				vmaDestroyBuffer(*allocator, buffer, bufferAllocation);
			}

			VkBuffer& getBuffer() { return buffer; }
			VkBufferUsageFlags getUsage() { return usage; }
			VkMemoryPropertyFlags getMemoryProps() { return props; };
			VmaMemoryUsage getMemUsage() { return memUsage; }
			VkDeviceSize getSize() { return size; }

			uint8_t* map()
			{
				vmaMapMemory(*allocator, bufferAllocation, reinterpret_cast<void**>(&mappedData));

				return mappedData;
			}

			void load(const uint8_t* data, const size_t size, const size_t offset = 0)
			{
				if (size > this->size)
				{
					// when this happens we need to rebind any commands buffers connected to this.. todo
					vmaDestroyBuffer(*allocator, buffer, bufferAllocation);

					VkBufferCreateInfo buffCreateInfo = {};
					buffCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffCreateInfo.size = size;
					buffCreateInfo.usage = usage;
					buffCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

					VmaAllocationCreateInfo allocCreateInfo = {};
					allocCreateInfo.usage = memUsage;
					allocCreateInfo.requiredFlags = props;

					auto success = vmaCreateBuffer(*allocator, &buffCreateInfo, &allocCreateInfo, &buffer, &bufferAllocation, nullptr);

					Assert(success == VK_SUCCESS, "Failed to re-create Buffer");
				}

				uint8_t* dst = map();
				std::copy(data, data + size, dst + offset);
				unMap();
			}

			void load(void* data, const size_t size, const size_t offset = 0) { load(reinterpret_cast<const uint8_t*>(data), size, offset); }

			void unMap() const { vmaUnmapMemory(*allocator, bufferAllocation); }
	};
}
