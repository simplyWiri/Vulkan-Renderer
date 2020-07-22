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
		FramebufferKey(std::vector<VkImageView> imageViews, Renderpass renderpass, VkExtent2D extent)
			: imageViews(imageViews), renderpass(renderpass), extent(extent) { }

		std::vector<VkImageView> imageViews;
		Renderpass renderpass;
		VkExtent2D extent;

		bool operator <(const FramebufferKey& other) const
		{
			return std::tie(imageViews, extent.width, extent.height) < std::tie(
				other.imageViews, other.extent.width, other.extent.height);
		}
	};

	struct Framebuffer
	{
	public:
		Framebuffer(VkDevice* device, FramebufferKey key)
			: device(device)
		{
			framebuffers.resize(key.imageViews.size());

			for (int i = 0; i < key.imageViews.size(); i++)
			{
				VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
				createInfo.attachmentCount = 1;
				createInfo.pAttachments = &key.imageViews[i];
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
