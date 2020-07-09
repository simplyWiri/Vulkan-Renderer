#pragma once
#include "vulkan.h"
#include <vector>

namespace Renderer
{
	struct AttachmentDesc
	{
	public:
		VkFormat format;
		VkAttachmentLoadOp loadOp;
		VkClearValue clearValue;

		bool operator < (const AttachmentDesc& other) const
		{
			bool earlyExit = std::tie(format, loadOp) < std::tie(other.format, other.loadOp);
			if (!earlyExit) return false;

			bool clearVal = std::tie(clearValue.color.int32[0], clearValue.color.int32[1], clearValue.color.int32[2], clearValue.color.int32[3]) <
				std::tie(other.clearValue.color.int32[0], other.clearValue.color.int32[1], other.clearValue.color.int32[2], other.clearValue.color.int32[3]);

			return clearVal;
		}
	};

	struct RenderpassKey
	{
	public:
		RenderpassKey(std::vector<AttachmentDesc> colourAttachments, AttachmentDesc depthAttachment)
			: colourAttachments(colourAttachments), depthAttachment(depthAttachment) { }

		std::vector<AttachmentDesc> colourAttachments;
		AttachmentDesc depthAttachment;

		bool operator < (const RenderpassKey& other) const { return std::tie(colourAttachments, depthAttachment) < std::tie(other.colourAttachments, other.depthAttachment); }
	};

	struct Renderpass
	{
	public:
		Renderpass(VkDevice* device, RenderpassKey key);

		inline VkRenderPass getHandle() { return renderpass; }
		inline uint32_t getColourAttachmentCount() { return static_cast<uint32_t>(colourAttachments.size()); }
	private:
		VkDevice* device;
		VkRenderPass renderpass;
		std::vector<AttachmentDesc> colourAttachments;
	};
}