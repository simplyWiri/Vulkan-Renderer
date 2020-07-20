#pragma once
#include "vulkan.h"
#include "../Wrappers/Framebuffer.h"
#include "Cache.h"

namespace Renderer
{
	class FramebufferCache : public Cache<Framebuffer, FramebufferKey>
	{
	public:
		void buildCache(VkDevice* device) { this->device = device; }

		void beginPass(VkCommandBuffer commandBuffer, FramebufferKey key)
		{
			Framebuffer* framebuffer = get(key);

			VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO};
			beginInfo.renderPass = key.renderpass.getHandle();
			beginInfo.framebuffer = framebuffer->getHandle();
			beginInfo.renderArea.extent = key.extent;
			beginInfo.renderArea.offset = {0, 0};

			vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		inline void endPass(VkCommandBuffer commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

		Framebuffer* get(FramebufferKey key) override
		{
			auto& renderPass = cache[key];
			if (!renderPass)
			{
				renderPass = new Framebuffer(device, key);
				registerInput(key);
			}
			return renderPass;
		}

		bool add(FramebufferKey key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Framebuffer(device, key));
			registerInput(key);

			return true;
		}

		bool add(FramebufferKey key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Framebuffer(device, key));
			local = registerInput(key);

			return true;
		}

	private:

		void clearEntry(Framebuffer* framebuffer) override
		{
			vkDestroyFramebuffer(*device, framebuffer->getHandle(), nullptr);
		}

		VkDevice* device;
	};
}
