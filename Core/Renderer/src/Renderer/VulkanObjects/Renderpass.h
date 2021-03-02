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
		VkClearValue clearValue;

		bool operator ==(const AttachmentDesc& other) const;
	};

	struct RenderpassKey
	{
		RenderpassKey()
		{
			depthAttachment = AttachmentDesc{VK_FORMAT_UNDEFINED};
		}
		RenderpassKey(std::vector<AttachmentDesc> colourAttachments, AttachmentDesc depthAttachment) : colourAttachments(std::move(colourAttachments)), depthAttachment(depthAttachment) { }

		std::vector<AttachmentDesc> colourAttachments;
		AttachmentDesc depthAttachment;

		bool operator ==(const RenderpassKey& other) const;
	};

	struct Renderpass
	{
		operator VkRenderPass() { return renderpass; }

		Renderpass(VkDevice* device, RenderpassKey key);

		VkRenderPass GetHandle() { return renderpass; }
		uint32_t GetColourAttachmentCount() { return static_cast<uint32_t>(colourAttachments.size()); }
	private:
		VkDevice* device;
		VkRenderPass renderpass;
		std::vector<AttachmentDesc> colourAttachments;
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
		void BuildCache(VkDevice* device) { this->device = device; }

		Renderpass* Get(const RenderpassKey& key) override;
		bool Add(const RenderpassKey& key) override;
		void ClearEntry(Renderpass* renderpass) override;

	private:
		VkDevice* device;
	};
}
