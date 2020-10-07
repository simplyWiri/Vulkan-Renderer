#include "Allocation.h"
#include "Block.h"
#include "Allocator.h"
#include "../Device.h"

namespace Renderer::Memory
{
	Block::Block(Allocator* parent, uint32_t memoryTypeIndex, uint32_t heapIndex, uint32_t blockId, VkDeviceSize size) : blockId(blockId), parent(parent), device(*parent->device->GetDevice()), size(size),
	                                                                                                                     memoryTypeIndex(memoryTypeIndex), heapIndex(heapIndex)
	{
		VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		info.allocationSize = size;
		info.memoryTypeIndex = memoryTypeIndex;

		memory = VkDeviceMemory{};
		
		const auto success = vkAllocateMemory(device, &info, nullptr, &memory);
		Assert(success == VK_SUCCESS, "Failed to allocate memory");

		freeAllocs.Push(Allocation{this, size, VkDeviceSize{0}, 0});
		utilisedSize = 0;
	}

	void Block::DefragmentMemory()
	{
		// todo
	}

	void Block::FreeAllocation(Allocation alloc)
	{
		usedAllocs.erase(alloc);
		freeAllocs.Push(alloc);
		utilisedSize -= alloc.size;
	}
	void Block::Clear()
	{
		vkFreeMemory(device, memory, nullptr);
	}

	void* Block::Map()
	{
		vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mappedData);
		mapCount++;
		return mappedData;
	}

	void Block::Unmap()
	{
		mapCount--;
		if (mapCount == 0) { vkUnmapMemory(device, memory); }
	}

	std::optional<Allocation> Block::TryFindMemory(const VkDeviceSize& size)
	{
		auto pos = freeAllocs.BinarySearchClosestIndex(Allocation{nullptr, size, 0, 0});

		if(pos == freeAllocs.Size()) return std::nullopt;

		// Our free allocation
		auto alloc = freeAllocs.Take(pos);

		// the new allocation we return
		auto subAlloc = Allocation{this, size, alloc.offset, (uint32_t)usedAllocs.size() };
		usedAllocs.emplace(subAlloc);

		// Update the free range accordingly
		alloc.offset += size;
		alloc.size -= size;
		alloc.parent = this;

		freeAllocs.Push(alloc);

		// Update the total utilised size
		utilisedSize += size;
		
		return subAlloc;
	}
}
