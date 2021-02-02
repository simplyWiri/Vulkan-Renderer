#include "Framebuffer.h"

#include <Tracy.hpp>

#include "Renderpass.h"

namespace Renderer
{
	FramebufferKey::FramebufferKey(std::vector<VkImageView> imageViews, Renderpass* renderpass, VkExtent2D extent) : imageViews(std::move(imageViews)), renderpass(renderpass), extent(extent) { }


	bool FramebufferKey::operator==(const FramebufferKey& other) const { return std::tie(imageViews, extent.width, extent.height) == std::tie(other.imageViews, other.extent.width, other.extent.height); }

	FramebufferBundle::FramebufferBundle(VkDevice* device, FramebufferKey key, uint32_t count) : device(device)
	{
		framebuffers.resize(count);

		for (uint32_t i = 0; i < count; i++)
		{
			VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			createInfo.attachmentCount = static_cast<uint32_t>(key.imageViews.size());
			createInfo.pAttachments = key.imageViews.data();
			createInfo.width = key.extent.width;
			createInfo.height = key.extent.height;
			createInfo.renderPass = key.renderpass->GetHandle();
			createInfo.layers = 1;

			vkCreateFramebuffer(*device, &createInfo, nullptr, &framebuffers[i]);
		}
	}

	void FramebufferCache::BuildCache(VkDevice* device, uint32_t framesInFlight)
	{
		this->device = device;
		this->framesInFlight = framesInFlight;
	}

	void FramebufferCache::BeginPass(VkCommandBuffer buffer, uint32_t index, const std::vector<VkImageView>& views, Renderpass* renderpass, VkExtent2D extent)
	{
		ZoneScopedN("Beginning renderpass")

		FramebufferKey key = FramebufferKey(views, renderpass, extent);

		auto framebuffer = Get(key);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderpass->GetHandle();
		renderPassInfo.framebuffer = framebuffer->GetHandle()[index];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;

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

	void FramebufferCache::EndPass(VkCommandBuffer buffer) { vkCmdEndRenderPass(buffer); }

	FramebufferBundle* FramebufferCache::Get(const FramebufferKey& key)
	{
		auto& framebuffer = cache[key];
		if (!framebuffer) { framebuffer = new FramebufferBundle(device, key, framesInFlight); }
		return framebuffer;
	}

	bool FramebufferCache::Add(const FramebufferKey& key)
	{
		if (cache.find(key) != cache.end()) return false;

		cache.emplace(key, new FramebufferBundle(device, key, framesInFlight));

		return true;
	}

	void FramebufferCache::ClearEntry(FramebufferBundle* framebuffers) { for (auto framebuffer : framebuffers->GetHandle()) { vkDestroyFramebuffer(*device, framebuffer, nullptr); } }
}
