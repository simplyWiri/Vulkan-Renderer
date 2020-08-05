#pragma once
#include "vulkan.h"
#include "Image.h"

namespace Renderer
{
	class ImageView
	{
	private:
		Image* image;
		VkImageView view;
		VkImageSubresourceRange range;
		VkDevice* device;
	public:
		ImageView(VkDevice* device, Image* image)
			: image(image), device(device)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image->getImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			const auto format = image->getFormat();
			viewInfo.format = format;

			if (format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM_S8_UINT ||
				format == VK_FORMAT_D24_UNORM_S8_UINT ||
				format == VK_FORMAT_D32_SFLOAT_S8_UINT)
			{
				range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			viewInfo.subresourceRange = range;

			auto success = vkCreateImageView(*device, &viewInfo, nullptr, &view);

			image->registerImageView(this);

			Assert(success == VK_SUCCESS, "Failed to create image view");
		}
		ImageView() {}
		~ImageView()
		{
			if(device != nullptr)
			{
				vkDestroyImageView(*device, view, nullptr);
			}
			//if(image != nullptr)
			//{
			//	image->deregisterImageView(this);
			//}
		}

		VkImageView getView() { return view; }

	};
}