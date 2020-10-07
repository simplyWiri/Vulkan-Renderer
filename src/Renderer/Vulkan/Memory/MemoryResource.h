#pragma once
#include <algorithm>

#include "Allocation.h"

namespace Renderer::Memory
{
	template <typename T>
	class MemoryResource
	{
		
	protected:
		T resourceHandle;
		Allocation allocation;

	public:
		MemoryResource(Allocation alloc, T handle) : resourceHandle(handle), allocation(alloc)
		{
			
		}
		
		Allocation GetAllocation() { return allocation; }
		T& GetResourceHandle() { return resourceHandle; }

		bool operator==(const MemoryResource<T> other) const
		{
			return std::tie(resourceHandle, allocation) == std::tie(other.resourceHandle, other.allocation);
		}
	};
}
