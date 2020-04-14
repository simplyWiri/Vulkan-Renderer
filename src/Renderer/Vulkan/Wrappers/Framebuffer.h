#pragma once
#include "vulkan.h"
#include <vector>
#include <map>
#include <iostream>
#include "Renderpass.h"

namespace Renderer {
	struct FramebufferKey
	{
	public:
		FramebufferKey(std::vector<VkImageView> imageViews, Renderpass renderpass, VkExtent2D extent)
			: imageViews(imageViews), renderpass(renderpass), extent(extent) { }

		std::vector<VkImageView> imageViews;
		Renderpass renderpass;
		VkExtent2D extent;

		// Allows for sorting in a map
		bool operator < (const FramebufferKey& other) { return std::tie(imageViews, renderpass, extent) < std::tie(imageViews, renderpass, extent); }
	};

	struct Framebuffer
	{
	public:
		Framebuffer(VkDevice* device, FramebufferKey key)
			: device(device)
		{
			VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			createInfo.attachmentCount = key.imageViews.size();
			createInfo.pAttachments = key.imageViews.data();
			createInfo.width = key.extent.width;
			createInfo.height = key.extent.height;
			createInfo.renderPass = key.renderpass.getHandle();
			createInfo.layers = 1;

			vkCreateFramebuffer(*device, &createInfo, nullptr, &framebuffer);
		}

		VkFramebuffer getHandle() { return framebuffer; }

		void cleanup() { vkDestroyFramebuffer(*device, framebuffer, nullptr); }

	private:
		VkDevice* device;
		VkFramebuffer framebuffer;
	};

	class FramebufferCache
	{
	public:

		FramebufferCache(VkDevice* device) : device(device) { }

		void beginPass(VkCommandBuffer commandBuffer, FramebufferKey key)
		{
			Framebuffer* framebuffer = getFramebuffer(key);

			VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO };
			beginInfo.renderPass = key.renderpass.getHandle();
			beginInfo.framebuffer = framebuffer->getHandle();
			beginInfo.renderArea.extent = key.extent;
			beginInfo.renderArea.offset = { 0 , 0 };

			vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		inline void endPass(VkCommandBuffer commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

	private:

		inline Framebuffer* getFramebuffer(FramebufferKey key)
		{
			try {
				return framebuffers.at(key);
			}
			catch (...) {
				framebuffers.emplace(new Framebuffer(device, key));
				return framebuffers.at(key);
			}
		}

		VkDevice* device;
		std::map<FramebufferKey, Framebuffer*> framebuffers;
	};
}