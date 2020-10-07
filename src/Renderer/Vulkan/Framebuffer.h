#pragma once
#include "vulkan.h"
#include <vector>
#include <map>
#include <iostream>
#include "Renderpass.h"
#include "../Cache.h"

namespace Renderer
{
	struct FramebufferKey
	{
	public:
		FramebufferKey(std::vector<VkImageView> imageViews, Renderpass renderpass, VkExtent2D extent) : imageViews(imageViews), renderpass(renderpass), extent(extent) { }

		std::vector<VkImageView> imageViews;
		Renderpass renderpass;
		VkExtent2D extent;

		bool operator ==(const FramebufferKey& other) const { return std::tie(imageViews, extent.width, extent.height) == std::tie(other.imageViews, other.extent.width, other.extent.height); }

	};

	struct FramebufferBundle
	{
		FramebufferBundle(VkDevice* device, FramebufferKey key, uint32_t count) : device(device)
		{
			framebuffers.resize(count);

			for (uint32_t i = 0; i < count; i++)
			{
				VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
				createInfo.attachmentCount = static_cast<uint32_t>(key.imageViews.size());
				createInfo.pAttachments = key.imageViews.data();
				createInfo.width = key.extent.width;
				createInfo.height = key.extent.height;
				createInfo.renderPass = key.renderpass.GetHandle();
				createInfo.layers = 1;

				vkCreateFramebuffer(*device, &createInfo, nullptr, &framebuffers[i]);
			}
		}

		std::vector<VkFramebuffer> GetHandle() { return framebuffers; }

	private:
		VkDevice* device;
		std::vector<VkFramebuffer> framebuffers;
	};
}

namespace std
{
	template<> struct hash<Renderer::FramebufferKey>
	{
		size_t operator()(const Renderer::FramebufferKey& s) const noexcept
		{
			size_t h1 = hash<uint32_t>{}(s.extent.width);
			h1 = h1 ^ (hash<uint32_t>{}(s.extent.height) << 1);

			h1 ^= (s.imageViews.size() << 1);
			for (auto& i : s.imageViews)
				h1 ^= reinterpret_cast<uint64_t>(i) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
			
			return h1;
		}
	};
}

namespace Renderer
{

	class FramebufferCache : public Cache<FramebufferBundle, FramebufferKey>
	{
	public:
		void BuildCache(VkDevice* device, uint32_t framesInFlight)
		{
			this->device = device;
			this->framesInFlight = framesInFlight;
		}

		void BeginPass(VkCommandBuffer buffer, uint32_t index, std::vector<VkImageView> views, Renderpass* renderpass, VkExtent2D extent)
		{
			FramebufferKey key = FramebufferKey(views, *renderpass, extent);

			auto framebuffer = Get(key);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderpass->GetHandle();
			renderPassInfo.framebuffer = framebuffer->GetHandle().at(index);
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = extent;

			std::vector<VkClearValue> clearColors = { {0.2f, 0.2f, 0.2f, 1.0f}, {1.0f, 0} };
			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearColors.data();

			VkViewport viewport = {};
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0;
			viewport.maxDepth = 1;

			VkRect2D rect = {};
			rect.offset = { 0, 0 };
			rect.extent = extent;

			vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(buffer, 0, 1, &viewport);
			vkCmdSetScissor(buffer, 0, 1, &rect);
		}

		void EndPass(VkCommandBuffer buffer) { vkCmdEndRenderPass(buffer); }

		FramebufferBundle* Get(const FramebufferKey& key) override
		{
			auto& framebuffer = cache[key];
			if (!framebuffer)
			{
				framebuffer = new FramebufferBundle(device, key, framesInFlight);
			}
			return framebuffer;
		}

		bool Add(const FramebufferKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new FramebufferBundle(device, key, framesInFlight));

			return true;
		}

	private:

		void ClearEntry(FramebufferBundle* framebuffers) override { for (auto framebuffer : framebuffers->GetHandle()) { vkDestroyFramebuffer(*device, framebuffer, nullptr); } }

		VkDevice* device;
		uint32_t framesInFlight;
	};
}
