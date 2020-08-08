#pragma once
#include "vulkan.h"
#include "../Wrappers/Framebuffer.h"
#include "Cache.h"

namespace Renderer
{
	class FramebufferCache : public Cache<FramebufferBundle, FramebufferKey>
	{
		public:
			void buildCache(VkDevice* device, uint32_t framesInFlight)
			{
				this->device = device;
				this->framesInFlight = framesInFlight;
			}

			void BeginPass(VkCommandBuffer buffer, uint32_t index, std::vector<VkImageView> views, Renderpass* renderpass, VkExtent2D extent)
			{
				FramebufferKey key = FramebufferKey(views, *renderpass, extent);

				auto framebuffer = get(key);

				VkRenderPassBeginInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = renderpass->getHandle();
				renderPassInfo.framebuffer = framebuffer->getHandle()[index];
				renderPassInfo.renderArea.offset = {0, 0};
				renderPassInfo.renderArea.extent = extent;

				std::vector<VkClearValue> clearColors = {{0.2f, 0.2f, 0.2f, 1.0f}, {1.0f, 0}};
				renderPassInfo.clearValueCount = 2;
				renderPassInfo.pClearValues = clearColors.data();

				VkViewport viewport = {};
				viewport.width = static_cast<float>(extent.width);
				viewport.height = static_cast<float>(extent.height);
				viewport.minDepth = 0;
				viewport.maxDepth = 1;

				VkRect2D rect = {};
				rect.offset = {0, 0};
				rect.extent = extent;

				vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdSetViewport(buffer, 0, 1, &viewport);
				vkCmdSetScissor(buffer, 0, 1, &rect);
			}

			void EndPass(VkCommandBuffer buffer) { vkCmdEndRenderPass(buffer); }

			FramebufferBundle* get(const FramebufferKey& key) override
			{
				auto& framebuffer = cache[key];
				if (!framebuffer)
				{
					framebuffer = new FramebufferBundle(device, key, framesInFlight);
					registerInput(key);
				}
				return framebuffer;
			}

			bool add(const FramebufferKey& key) override
			{
				if (cache.find(key) != cache.end()) return false;

				cache.emplace(key, new FramebufferBundle(device, key, framesInFlight));
				registerInput(key);

				return true;
			}

			bool add(const FramebufferKey& key, uint16_t& local) override
			{
				if (cache.find(key) != cache.end()) return false;

				cache.emplace(key, new FramebufferBundle(device, key, framesInFlight));
				local = registerInput(key);

				return true;
			}

		private:

			void clearEntry(FramebufferBundle* framebuffers) override { for (auto framebuffer : framebuffers->getHandle()) { vkDestroyFramebuffer(*device, framebuffer, nullptr); } }

			VkDevice* device;
			uint32_t framesInFlight;
	};
}
