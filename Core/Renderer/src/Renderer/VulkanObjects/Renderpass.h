#pragma once
#include <vector>

#include "volk/volk.h"

#include "Cache.h"

namespace Renderer
{
	struct AttachmentDesc
	{
		VkFormat format;
		VkAttachmentLoadOp loadOp;
		VkClearValue clearValue{0.2f,0.2f,0.2f,0.2f};

		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		void SetDepthClear() { clearValue = {1.0f, 0}; }

		bool operator ==(const AttachmentDesc& other) const;
	};

	struct RenderpassKey
	{
		RenderpassKey()
		{
			depthAttachment = AttachmentDesc{VK_FORMAT_UNDEFINED};
		}
		RenderpassKey(std::vector<AttachmentDesc> colourAttachments, AttachmentDesc depthAttachment) : colourAttachments(std::move(colourAttachments)), depthAttachment(depthAttachment){ }

		std::vector<AttachmentDesc> colourAttachments;
		AttachmentDesc depthAttachment;

		bool operator ==(const RenderpassKey& other) const;
	};

	struct Renderpass
	{
		operator VkRenderPass() { return renderpass; }

		Renderpass(VkDevice device, RenderpassKey key);

		VkRenderPass GetHandle() const { return renderpass; }
		std::vector<VkClearValue>& GetClearValues() { return clearValues; }

		bool operator ==(const Renderpass& other) const { return renderpass == other.renderpass; }
	private:
		VkRenderPass renderpass;
		std::vector<VkClearValue> clearValues;
	};
}

namespace std
{
	template <>
	struct hash<Renderer::RenderpassKey>
	{
		size_t operator()(const Renderer::RenderpassKey& s) const noexcept
		{
			size_t h1 = hash<underlying_type<VkFormat>::type>{}(s.depthAttachment.format);

			for (auto& i : s.colourAttachments)
			{
				size_t h2 = hash<underlying_type<VkFormat>::type>{}(i.format);
				h2 ^= hash<underlying_type<VkAttachmentLoadOp>::type>{}(i.loadOp);

				h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
			}

			return h1;
		}
	};
}

namespace Renderer
{
	class RenderpassCache : public Cache<Renderpass, RenderpassKey>
	{
	public:
		void BuildCache(VkDevice device) { this->device = device; }

		Renderpass* Get(const RenderpassKey& key) override;
		bool Add(const RenderpassKey& key) override;
		void ClearEntry(Renderpass* renderpass) override;

	private:
		VkDevice device;
	};
}
