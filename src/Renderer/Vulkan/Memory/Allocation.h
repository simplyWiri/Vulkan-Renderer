#pragma once
#include <vulkan.h>
#include <vector>
#include "PriorityQueue.h"

namespace Renderer::Memory
{
	class Allocation;
	class Allocator;

	class SubAllocation
	{
	public:
		Allocation* parent;
		VkDeviceSize offset;
		VkDeviceSize range;

		bool operator <(const SubAllocation& other) const { return range < other.range; }

		uint8_t* Map();
		void UnMap();
	};

	class Allocation
	{
		friend class Allocator;
	private:
		Allocator* parent = nullptr;
		uint32_t memoryTypeIndex = 0;

		VkDeviceMemory memory = 0;
		VkDeviceSize size = 0;
		VkDeviceSize usedSize = 0;

		std::vector<SubAllocation> subAllocations;
		std::vector<std::pair<VkBuffer, SubAllocation>> pendingCleanup;
		PriorityQueue<SubAllocation> freeSubAllocations;

		std::vector<std::tuple<int, VkBuffer, SubAllocation>> buffersToClear;

		uint32_t mapCount = 0;
		uint8_t* beginMemory = nullptr;

	public:
		std::vector<SubAllocation> GetSubAllocations() { return subAllocations; }
		PriorityQueue<SubAllocation> GetFreeSubAllocations() { return freeSubAllocations; }

		bool operator <(const Allocation& other) const { return (size - usedSize) < (other.size - usedSize); }

		uint8_t* Map();
		void UnMap();

		void ClearSubAllocation(const SubAllocation& subAlloc);
		void RemoveBuffer(VkBuffer buffer, SubAllocation subAlloc);
	};
}