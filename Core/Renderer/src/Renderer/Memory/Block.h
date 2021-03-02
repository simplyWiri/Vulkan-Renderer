#pragma once
#include <optional>

#include "PriorityQueue.h"
#include "volk/volk.h"

namespace Renderer
{
	namespace Memory
	{
		class Allocation;
		class Allocator;
	}

	class Device;
}

namespace std
{
	template <>
	struct hash<Renderer::Memory::Allocation>;
}


namespace Renderer::Memory
{
	// Todo, implement as a free list
	// doubly linked list, ascending order of offset
	
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

		std::list<Allocation> allocations;

		uint32_t memoryTypeIndex;
		uint32_t heapIndex;

		uint32_t mapCount = 0;
		void* mappedData = nullptr;

	public:

		VkDeviceSize GetSize() const { return size; }
		VkDeviceSize GetUsedSize() const { return utilisedSize; }
		VkDeviceMemory GetMemory() const { return memory; }
		uint32_t GetId() const { return blockId; }
		uint32_t GetHeapIndex() const { return heapIndex; }
		uint32_t GetMemoryTypeIndex() const { return memoryTypeIndex; }
		std::list<Allocation> GetAllocations() { return allocations; }

		bool operator==(const Block& other) { return std::tie(blockId, heapIndex, memoryTypeIndex) == std::tie(other.blockId, other.heapIndex, other.memoryTypeIndex); }
		bool operator<(const Block& other) { return std::tie(blockId, heapIndex, memoryTypeIndex) == std::tie(other.blockId, other.heapIndex, other.memoryTypeIndex); }

	public:
		Block(Allocator* parent, uint32_t memoryTypeIndex, uint32_t heapIndex, uint32_t blockId, VkDeviceSize size);
		Block(const Block& o) = delete;
		Block& operator=(const Block&) = delete;
		~Block() = default;

		void FreeAllocation(Allocation alloc);
		void Clear();

		void* Map();
		void Unmap();

		// if a value is returned, it has been moved into the 'used' queue
		std::optional<Allocation> TryFindMemory(const VkDeviceSize& size);

	private:

		std::list<Allocation>::iterator BestAllocation(const VkDeviceSize& size);
		//std::list<Allocation>::iterator FirstAllocation(const VkDeviceSize& size);
		
	};
}
