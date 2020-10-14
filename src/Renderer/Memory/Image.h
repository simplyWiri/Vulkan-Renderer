#pragma once
#include <functional>

#include "MemoryResource.h"
#include "vulkan.h"

namespace Renderer::Memory
{
	class Image : public MemoryResource<VkImage>
	{
	private:
		VkImageView view;

		VkExtent3D extent;
		VkFormat format;
		VkImageUsageFlags usage;
		VkImageSubresourceRange range;

		std::function<void(Image*)> cleanup;

	public:
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image& operator=(Image&&) = delete;

		Image(VkImage image, VkImageView view, Allocation alloc, VkImageSubresourceRange range, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, const std::function<void(Image*)>& cleanup) : MemoryResource(alloc, image),
			view(view), extent(extent), format(format), usage(usage), range(range), cleanup(cleanup) { }

		~Image() { cleanup(this); }

		VkImageView GetView() { return view; }
		VkExtent2D GetExtent() const { return { extent.width, extent.height }; }
		VkExtent3D GetExtent3D() const { return extent; }
		VkFormat& GetFormat() { return format; }
		VkImageSubresourceRange& GetSubresourceRange() { return range; }
	};
}
