#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "PriorityQueue.h"
#include "imgui.h"
#include <unordered_set>
#include "Buffer.h"
#include "Image.h"
#include <array>
#include <functional>

namespace Renderer
{
	class Device;
}

namespace Renderer::Memory
{
	class Block;
	class Allocation;
	
	class Allocator
	{
		friend class Block;
	private:
		Device* device;
		VkPhysicalDeviceMemoryProperties physMemoryProps;
		VkQueue transferQueue;
		uint32_t currentFrameOffset = 0;
		uint32_t framesInFlight;

		std::array< std::vector<Block*>, VK_MAX_MEMORY_TYPES> memoryBlocks;
		
		std::unordered_set<VkBuffer> allocatedBuffers;

		std::vector< std::vector<std::pair<Allocation, VkBuffer>>> buffersToClear;


	public:
		Allocator(Device* device, int framesInFlight = 3);
		~Allocator();

		Buffer* AllocateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& flags);
		Image AllocateImage(const VkExtent2D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags);

		void DeallocateBuffer(Buffer* buffer);
		void DeallocateImage(Image* image);
		
		void BeginFrame();
		void EndFrame();

		void DebugView();

	private:

		Allocation FindAllocation(const VkMemoryRequirements& memReqs, VkMemoryPropertyFlags requiredProperties);

		uint32_t findProperties(uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requiredProperties) const
		{
			for (uint32_t i = 0; i < physMemoryProps.memoryTypeCount; i++) if (memoryTypeBitsRequirement & (1 << i) && (physMemoryProps.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) return i;

			return ~0;
		}
	};
}
