#include "Resource.h"
#include "../Memory/Allocator.h"
#include "RenderGraph.h"
#include "../Memory/Buffer.h"
#include "../Memory/Image.h"

namespace Renderer::RenderGraph
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

	ImageInfo& ImageInfo::SetSizeType(SizeType sizeType)
	{
		this->sizeType = sizeType;
		return *this;
	}

	BufferResource::~BufferResource()
	{
		for(auto buf : buffers) delete buf;
	}

	void BufferResource::Build(Memory::Allocator* allocator, uint32_t framesInFlight)
	{
		buffers.resize(framesInFlight);

		for(auto i = 0; i < framesInFlight; i++)
		{
			buffers[i] = allocator->AllocateBuffer(info.size, info.usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
	}

	ImageResource::~ImageResource()
	{
		for(auto img : images) delete img;
	}

	void ImageResource::Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight)
	{
		images.resize(framesInFlight);

		for(auto i = 0; i < framesInFlight; i++)
		{
			VkExtent2D extent;
			if(info.sizeType == ImageInfo::SizeType::Swapchain) extent = swapchainExtent;
			else if (info.sizeType == ImageInfo::SizeType::Fixed) extent = VkExtent2D{ static_cast<uint32_t>(info.size.x), static_cast<uint32_t>(info.size.y) };
			else extent = VkExtent2D {
				static_cast<uint32_t>(swapchainExtent.width * static_cast<float>(info.size.x)),
				static_cast<uint32_t>(swapchainExtent.height * static_cast<float>(info.size.y))
			};
			
			images[i] = allocator->AllocateImage(extent, info.format, info.usage,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}
}
