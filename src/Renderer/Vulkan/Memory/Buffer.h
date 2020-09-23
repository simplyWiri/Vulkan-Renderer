#pragma once
#include "Allocation.h"

namespace Renderer::Memory
{
	class Buffer
	{
		friend class Allocator;
		VkBuffer buffer;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags flags;
		SubAllocation alloc;

	public:
		operator VkBuffer() { return buffer; }

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;

		VkBuffer& GetBuffer() { return buffer; }
		VkBufferUsageFlags GetUsage() { return usage; }
		VkMemoryPropertyFlags GetMemUsage() { return flags; }
		VkDeviceSize GetSize() { return alloc.range; }
		VkDeviceSize GetOffset() { return alloc.offset; }

		Buffer(VkBuffer buffer, SubAllocation& allocation, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
		{
			this->buffer = buffer;
			this->alloc = allocation;
			this->usage = usage;	
			this->flags = flags;
		}

		~Buffer()
		{
			alloc.parent->RemoveBuffer(buffer, alloc);
		}

		uint8_t* Map() { return alloc.Map(); }
		void UnMap() { alloc.UnMap(); }

		void Load(const uint8_t* data, const size_t size, const size_t offset = 0)
		{
			if (size > this->alloc.range) LogError("Trying to load more data into a GPU buffer than you have allocated");
			uint8_t* dst = Map();
			memcpy(dst + offset, data, size);
			UnMap();
		}

		void Load(void* data, const size_t size, const size_t offset = 0)
		{
			Load(reinterpret_cast<const uint8_t*>(data), size, offset);
		}
	};

}