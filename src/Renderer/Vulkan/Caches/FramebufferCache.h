#pragma once
#include "vulkan.h"
#include "../Wrappers/Framebuffer.h"

namespace Renderer {
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
				framebuffers.emplace(key, new Framebuffer(device, key));
				return framebuffers.at(key);
			}
		}

		VkDevice* device;
		std::map<FramebufferKey, Framebuffer*> framebuffers;
	};
}
