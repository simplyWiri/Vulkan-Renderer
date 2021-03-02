#pragma once
#include <unordered_set>
#include <array>
#include <functional>

#include "volk/volk.h"

namespace Renderer
{
	class Device;
}

namespace Renderer::Memory
{
	class Block;
	class Allocation;
	class Buffer;
	class Image;

	class Allocator
	{
		friend class Block;
	private:
		Device* device;
		VkPhysicalDeviceMemoryProperties physMemoryProps;
		VkQueue transferQueue;
		uint32_t currentFrameOffset = 0;
		uint32_t framesInFlight;

		std::array<std::vector<Block*>, VK_MAX_MEMORY_TYPES> memoryBlocks;

		std::unordered_set<VkBuffer> allocatedBuffers;
		std::unordered_set<VkImage> allocatedImages;

		std::vector<std::vector<std::function<void(VkDevice)>>> cleanups;

	public:
		Allocator(Device* device, int framesInFlight = 3);
		~Allocator();

		Buffer* AllocateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& flags);
		Image* AllocateImage(const VkExtent3D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags);
		Image* AllocateImage(const VkExtent2D& extent, const VkFormat& format, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& flags);


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
