#include "Allocation.h"
#include "Allocator.h"

namespace Renderer::Memory
{

	uint8_t* Allocation::Map()
	{
		if (mapCount++ == 0) vkMapMemory(*parent->device, memory, 0, size, 0, reinterpret_cast<void**>(beginMemory));

		return beginMemory;
	}
	void Allocation::UnMap()
	{
		if (mapCount-- == 1) vkUnmapMemory(*parent->device, memory);
	}

	void Allocation::ClearSubAllocation(const SubAllocation& subAlloc)
	{
		auto& itr = subAllocations.begin();

		while (itr != subAllocations.end())
			if (itr->offset == subAlloc.offset) break;

		freeSubAllocations.Push(SubAllocation{ this, itr->offset, itr->range });
		subAllocations.erase(itr);
	}

	void Allocation::RemoveBuffer(VkBuffer buffer, SubAllocation subAlloc)
	{
		// -1 says that we do not have a frame for this to be deleted on
		buffersToClear.push_back(std::make_tuple( -1, buffer, subAlloc ));
	}

	uint8_t* SubAllocation::Map()
	{
		return parent->Map() + offset;
	}

	void SubAllocation::UnMap()
	{
		parent->UnMap();
	}
}