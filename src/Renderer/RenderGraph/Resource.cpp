#include "Resource.h"
#include "../Memory/Allocator.h"
#include "RenderGraph.h"

namespace Renderer
{
	ImageInfo& ImageInfo::SetSize(glm::vec3 size)
	{
		this->size = size;
		return *this;
	}

	ImageInfo& ImageInfo::SetFormat(VkFormat format)
	{
		this->format = format;
		return *this;
	}

	ImageInfo& ImageInfo::SetLayout(VkImageLayout layout)
	{
		this->layout = layout;
		return *this;
	}

	ImageInfo& ImageInfo::SetUsage(VkImageUsageFlags usage)
	{
		this->usage = usage;
		return *this;
	}

	ImageInfo& ImageInfo::SetSizeType(ImageSize sizeType)
	{
		this->sizeType = sizeType;
		return *this;
	}

	void BufferResource::Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight)
	{
		buffers.resize(framesInFlight);

		for(auto i = 0; i < framesInFlight; i++)
		{
			buffers[i] = allocator->AllocateBuffer(info.size, info.usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
	}

	void ImageResource::Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight)
	{
		images.resize(framesInFlight);

		for(auto i = 0; i < framesInFlight; i++)
		{
			images[i] = allocator->AllocateImage( info.sizeType == ImageSize::Swapchain ? swapchainExtent : 
				VkExtent2D{ (uint32_t)info.size.x, (uint32_t)info.size.y }, info.format, info.usage,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
	}
}
