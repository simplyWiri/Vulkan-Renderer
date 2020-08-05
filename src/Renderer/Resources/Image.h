#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"

namespace Renderer
{
	class ImageView;

	class Image
	{
	private:
		VkImage image;
		std::vector<ImageView*> views;
		VmaAllocation allocation;

		VkExtent2D extent;
		VkFormat format;
		VkImageUsageFlags usage;

		uint8_t* data;
		VmaAllocator* allocator;

	public:
		Image(VmaAllocator* allocator, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memUsage)
			: allocator(allocator), extent(extent), format(format), usage(usage)
		{
			VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = format;
			imageInfo.extent.width = extent.width;
			imageInfo.extent.height = extent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = memUsage;

			if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) allocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

			auto result = vmaCreateImage(*allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
			Assert(result == VK_SUCCESS, "Failed to create image");
		}

		Image() {}

		~Image()
		{
			if (allocator != nullptr)
			{
				vmaDestroyImage(*allocator, image, allocation);
			}
		}

		VkImage& getImage() { return image; }
		std::vector<ImageView*> getViews() { return views; }
		VkExtent2D& getExtent() { return extent; }
		VkFormat& getFormat() { return format; }

		void registerImageView(ImageView* view) { views.push_back(view); }
		//void deregisterImageView(ImageView* view) { views.erase(view); }

	};
}
//
//Image::Image(Device &              device,
//             const VkExtent3D &    extent,
//             VkFormat              format,
//             VkImageUsageFlags     image_usage,
//             VmaMemoryUsage        memory_usage,
//             VkSampleCountFlagBits sample_count,
//             const uint32_t        mip_levels,
//             const uint32_t        array_layers,
//             VkImageTiling         tiling,
//             VkImageCreateFlags    flags) :
//    device{device},
//    type{find_image_type(extent)},
//    extent{extent},
//    format{format},
//    sample_count{sample_count},
//    usage{image_usage},
//    array_layer_count{array_layers},
//    tiling{tiling}
//{
//	assert(mip_levels > 0 && "Image should have at least one level");
//	assert(array_layers > 0 && "Image should have at least one layer");
//
//	subresource.mipLevel   = mip_levels;
//	subresource.arrayLayer = array_layers;
//
//	VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
//	image_info.flags       = flags;
//	image_info.imageType   = type;
//	image_info.format      = format;
//	image_info.extent      = extent;
//	image_info.mipLevels   = mip_levels;
//	image_info.arrayLayers = array_layers;
//	image_info.samples     = sample_count;
//	image_info.tiling      = tiling;
//	image_info.usage       = image_usage;
//
//	VmaAllocationCreateInfo memory_info{};
//	memory_info.usage = memory_usage;
//
//	if (image_usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
//	{
//		memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
//	}
//
//	auto result = vmaCreateImage(device.get_memory_allocator(),
//	                             &image_info, &memory_info,
//	                             &handle, &memory,
//	                             nullptr);
//
//	if (result != VK_SUCCESS)
//	{
//		throw VulkanException{result, "Cannot create Image"};
//	}
//}
//
//Image::Image(Device &device, VkImage handle, const VkExtent3D &extent, VkFormat format, VkImageUsageFlags image_usage, VkSampleCountFlagBits sample_count) :
//    device{device},
//    handle{handle},
//    type{find_image_type(extent)},
//    extent{extent},
//    format{format},
//    sample_count{sample_count},
//    usage{image_usage}
//{
//	subresource.mipLevel   = 1;
//	subresource.arrayLayer = 1;
//}
//
//Image::Image(Image &&other) :
//    device{other.device},
//    handle{other.handle},
//    memory{other.memory},
//    type{other.type},
//    extent{other.extent},
//    format{other.format},
//    sample_count{other.sample_count},
//    usage{other.usage},
//    tiling{other.tiling},
//    subresource{other.subresource},
//    mapped_data{other.mapped_data},
//    mapped{other.mapped}
//{
//	other.handle      = VK_NULL_HANDLE;
//	other.memory      = VK_NULL_HANDLE;
//	other.mapped_data = nullptr;
//	other.mapped      = false;
//
//	// Update image views references to this image to avoid dangling pointers
//	for (auto &view : views)
//	{
//		view->set_image(*this);
//	}
//}
//
//Image::~Image()
//{
//	if (handle != VK_NULL_HANDLE && memory != VK_NULL_HANDLE)
//	{
//		unmap();
//		vmaDestroyImage(device.get_memory_allocator(), handle, memory);
//	}
//}
