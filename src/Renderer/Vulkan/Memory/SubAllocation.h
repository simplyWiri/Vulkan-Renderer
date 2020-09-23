#pragma once
#include "Allocation.h"

namespace Renderer::Memory
{
	struct SubAllocation
	{
		Allocation* parent;
		VkDeviceSize offset;
		VkDeviceSize range;

		bool operator <(const SubAllocation& other) const { return range < other.range; }

		uint8_t* Map()
		{
			return parent->Map() + offset;
		}

		void UnMap()
		{
			parent->UnMap();
		}
	};
}