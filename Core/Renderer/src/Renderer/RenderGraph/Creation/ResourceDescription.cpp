#include "ResourceDescription.h"

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
}
