#include "Allocation.h"
#include "Block.h"
#include "Allocator.h"
#include "../VulkanObjects/Device.h"
#include "../../Utils/Logging.h"

namespace Renderer::Memory
{
	Block::Block(Allocator* parent, uint32_t memoryTypeIndex, uint32_t heapIndex, uint32_t blockId, VkDeviceSize size) : blockId(blockId), parent(parent), device(*parent->device->GetDevice()), size(size), memoryTypeIndex(memoryTypeIndex),
	                                                                                                                     heapIndex(heapIndex)
	{
		VkMemoryAllocateInfo info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		info.allocationSize = size;
		info.memoryTypeIndex = memoryTypeIndex;

		memory = VkDeviceMemory{};

		const auto success = vkAllocateMemory(device, &info, nullptr, &memory);
		Assert(success == VK_SUCCESS, "Failed to allocate memory");

		allocations.emplace_back(Allocation{this, size, VkDeviceSize{0}, false});
		utilisedSize = 0;
	}

	void Block::FreeAllocation(Allocation alloc)
	{
		utilisedSize -= alloc.size;
		auto iterator = allocations.begin();

		while(iterator != allocations.end())
		{
			if(iterator->offset == alloc.offset) break;
			++iterator;
		}

		// Go to the left, merging all 'free' allocs which 
		auto leftIterator = iterator;
		
		if(leftIterator != allocations.begin())
		{			
			while(--leftIterator != allocations.begin())
			{
				if(leftIterator->inUse) break;
				
				iterator->offset -= leftIterator->size;
				iterator->size += leftIterator->size;
				leftIterator = allocations.erase(leftIterator);
			}
		}
		
		auto rightIterator = iterator;
		if(rightIterator != allocations.end())
		{
			++rightIterator;
			
			while(rightIterator != allocations.end())
			{
				if(rightIterator->inUse) break;
				
				iterator->size += rightIterator->size;
				rightIterator = allocations.erase(rightIterator);
			}
		}

		iterator->inUse = false;
	}

	void Block::Clear()
	{
		vkFreeMemory(device, memory, nullptr);
	}

	void* Block::Map()
	{
		if(mapCount++ == 0) vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mappedData);
		return mappedData;
	}

	void Block::Unmap()
	{
		if (--mapCount == 0) { vkUnmapMemory(device, memory); }
	}

	std::optional<Allocation> Block::TryFindMemory(const VkDeviceSize& size)
	{
		auto alloc = BestAllocation(size);

		if(alloc->size < size || alloc->inUse) return std::nullopt;

		utilisedSize += size;
		if(alloc->size == size)
		{
			alloc->inUse = true;
			return *alloc;
		}
		
		auto subAlloc = Allocation { this, size, alloc->offset, true };
		alloc->size -= size;
		alloc->offset += size;

		if(alloc == allocations.begin()) allocations.emplace_front(subAlloc);
		else allocations.emplace(alloc, subAlloc);
		
		return subAlloc;
	}

	std::list<Allocation>::iterator Block::BestAllocation(const VkDeviceSize& size)
	{
		auto iterator = allocations.begin();
		auto curBest = iterator;

		for(; iterator != allocations.end(); ++iterator)
		{
			if(iterator->inUse) 
				continue;
			if(iterator->size < size) continue;
			

			// Our current best isn't assigned properly
			if(curBest->size < size || curBest->inUse) curBest = iterator;

			// The current iterator size is < current one ( & meets the size criteria)
			if(iterator->size < curBest->size) curBest = iterator;			
		}
		
		return curBest;
	}

}
