#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "../Vulkan/Device.h"

namespace Renderer
{
	class Image
	{
		private:
			VkImage image;
			VkImageView view;
			VmaAllocation allocation;

			VkExtent3D extent;
			VkFormat format;
			VkImageUsageFlags usage;
			VkImageSubresourceRange range;

			uint8_t* data;

			Device* device;

		public:
			Image(const Image&) = delete;
			Image& operator=(const Image&) = delete; 
			Image& operator=(Image&&) = delete;

			Image(Device* device, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memUsage) : format(format), usage(usage), device(device)
			{
				VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
				imageInfo.imageType = VK_IMAGE_TYPE_2D;
				imageInfo.format = format;
				this->extent = { extent.width, extent.height, 1 };
				imageInfo.extent = this->extent;
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

				auto result = vmaCreateImage(*device->GetAllocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr);
				Assert(result == VK_SUCCESS, "Failed to create image");

				VkImageViewCreateInfo viewInfo = {};
				viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewInfo.image = image;
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewInfo.format = format;

				if (format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
				{
					range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				else { range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; }
				range.baseMipLevel = 0;
				range.levelCount = 1;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				viewInfo.subresourceRange = range;

				result = vkCreateImageView(*device, &viewInfo, nullptr, &view);
				Assert(result == VK_SUCCESS, "Failed to create imageview");
			}

			Image() {}

			~Image()
			{
				if (device != nullptr)
				{
					vmaDestroyImage(*device->GetAllocator(), image, allocation);
					vkDestroyImageView(*device, view, nullptr); 
				}
			}

			VkImage& GetImage() { return image; }
			VkImageView GetView() { return view; }
			VkExtent2D GetExtent() const { return { extent.width, extent.height }; }
			VkExtent3D GetExtent3D() const { return extent; }
			VkFormat& GetFormat() { return format; }
			VkImageSubresourceRange GetSubresourceRange() { return range; }
	};
}
