#pragma once
#include "volk/volk.h"

#include "Cache.h"

namespace Renderer
{
	struct Renderpass;

	struct FramebufferKey
	{
		FramebufferKey(std::vector<VkImageView> imageViews, Renderpass* renderpass, VkExtent2D extent);

		std::vector<VkImageView> imageViews;
		Renderpass* renderpass;
		VkExtent2D extent;

		bool operator ==(const FramebufferKey& other) const;
	};

	struct FramebufferBundle
	{
		FramebufferBundle(VkDevice device, FramebufferKey key, uint32_t count);

		std::vector<VkFramebuffer> GetHandle() { return framebuffers; }

	private:
		std::vector<VkFramebuffer> framebuffers;
	};
}

namespace std
{
	template <>
	struct hash<Renderer::FramebufferKey>
	{
		size_t operator()(const Renderer::FramebufferKey& s) const noexcept
		{
			size_t h1 = hash<uint32_t>{}(s.extent.width);
			h1 = h1 ^ (hash<uint32_t>{}(s.extent.height) << 1);

			h1 ^= (s.imageViews.size() << 1);
			for (auto& i : s.imageViews) h1 ^= reinterpret_cast<uint64_t>(i) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);

			return h1;
		}
	};
}

namespace Renderer
{
	class FramebufferCache : public Cache<FramebufferBundle, FramebufferKey>
	{
	public:
		void BuildCache(VkDevice device, uint32_t framesInFlight);

		void BeginPass(VkCommandBuffer buffer, uint32_t index, const std::vector<VkImageView>& views, Renderpass* renderpass, VkExtent2D extent);
		void EndPass(VkCommandBuffer buffer);

		FramebufferBundle* Get(const FramebufferKey& key) override;
		bool Add(const FramebufferKey& key) override;

	private:

		void ClearEntry(FramebufferBundle* framebuffers) override;

		VkDevice device;
		uint32_t framesInFlight;

		const VkClearValue clearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
	};
}
