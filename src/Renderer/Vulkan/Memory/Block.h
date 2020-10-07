#pragma once
#include "PriorityQueue.h"
#include "vulkan.h"
#include <optional>
#include <unordered_set>

namespace Renderer
{
	namespace Memory {
		class Allocation;
		class Allocator;
	}

	class Device;
}

namespace std
{
	template<>
	struct hash<Renderer::Memory::Allocation>;
}


namespace Renderer::Memory
{
	class Block
	{
		friend class Allocator;
	private:
		uint32_t blockId;
		Allocator* parent;
		VkDevice device;

		VkDeviceMemory memory;
		VkDeviceSize size;
		VkDeviceSize utilisedSize;

		PriorityQueue<Allocation> freeAllocs;
		std::unordered_set<Allocation> usedAllocs;

		uint32_t memoryTypeIndex;
		uint32_t heapIndex;

		uint32_t mapCount = 0;
		void* mappedData = 0;

	public:

		VkDeviceSize GetSize() const { return size; }
		VkDeviceSize GetUsedSize() const { return utilisedSize; }
		VkDeviceMemory GetMemory() const { return memory; }
		uint32_t GetId() const { return blockId; }
		uint32_t GetHeapIndex() const { return heapIndex; }
		uint32_t GetMemoryTypeIndex() const { return memoryTypeIndex; }
		std::unordered_set<Allocation> GetUsedAllocs() { return usedAllocs;}

		bool operator==(const Block& other)
		{
			return std::tie(blockId, heapIndex, memoryTypeIndex) == std::tie(other.blockId, other.heapIndex, other.memoryTypeIndex);
		}
		bool operator<(const Block& other) 
		{
			return std::tie(blockId, heapIndex, memoryTypeIndex) == std::tie(other.blockId, other.heapIndex, other.memoryTypeIndex);
		}

	public:
		Block(Allocator* parent, uint32_t memoryTypeIndex, uint32_t heapIndex, uint32_t blockId, VkDeviceSize size);
		Block(const Block& o) = delete;
		Block& operator=(const Block&) = delete;
		~Block() = default;

		void DefragmentMemory();
		void FreeAllocation(Allocation alloc);
		void Clear();

		void* Map();
		void Unmap();

		// if a value is returned, it has been moved into the 'used' queue
		std::optional<Allocation> TryFindMemory(const VkDeviceSize& size);
	};
}
