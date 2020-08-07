#pragma once
#include "vulkan.h"
#include <vector>
#include <map>
#include <iostream>
#include "Renderpass.h"

namespace Renderer
{
	struct FramebufferKey
	{
		public:
			FramebufferKey(std::vector<VkImageView> imageViews, Renderpass renderpass, VkExtent2D extent) : imageViews(imageViews), renderpass(renderpass), extent(extent) { }

			std::vector<VkImageView> imageViews;
			Renderpass renderpass;
			VkExtent2D extent;

			bool operator <(const FramebufferKey& other) const { return std::tie(imageViews, extent.width, extent.height) < std::tie(other.imageViews, other.extent.width, other.extent.height); }
	};

	struct FramebufferBundle
	{
		FramebufferBundle(VkDevice* device, FramebufferKey key, uint32_t count) : device(device)
		{
			framebuffers.resize(count);

			for (uint32_t i = 0; i < count; i++)
			{
				VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
				createInfo.attachmentCount = static_cast<uint32_t>(key.imageViews.size());
				createInfo.pAttachments = key.imageViews.data();
				createInfo.width = key.extent.width;
				createInfo.height = key.extent.height;
				createInfo.renderPass = key.renderpass.getHandle();
				createInfo.layers = 1;

				vkCreateFramebuffer(*device, &createInfo, nullptr, &framebuffers[i]);
			}
		}

		std::vector<VkFramebuffer> getHandle() { return framebuffers; }

		private:
			VkDevice* device;
			std::vector<VkFramebuffer> framebuffers;
	};
}
