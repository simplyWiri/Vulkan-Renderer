#include "Renderpass.h"
#include "Utils/Logging.h"


namespace Renderer
{
	bool AttachmentDesc::operator==(const AttachmentDesc& other) const
	{
		if (!(std::tie(format, loadOp) == std::tie(other.format, other.loadOp))) return false;

		return std::tie(clearValue.color.int32[0], clearValue.color.int32[1], clearValue.color.int32[2], clearValue.color.int32[3]) == std::tie(other.clearValue.color.int32[0], other.clearValue.color.int32[1],
			       other.clearValue.color.int32[2], other.clearValue.color.int32[3]);
	}

	bool RenderpassKey::operator==(const RenderpassKey& other) const { return std::tie(colourAttachments, depthAttachment) == std::tie(other.colourAttachments, other.depthAttachment); }

	Renderpass::Renderpass(VkDevice* device, RenderpassKey key) : device(device), colourAttachments(key.colourAttachments)
	{
		std::vector<VkAttachmentReference> attachmentRefs;
		std::vector<VkAttachmentDescription> attachmentDescriptions;

		uint32_t curIndex = 0;
		// parse our colour attachment(s)

		for (AttachmentDesc colourAttachment : key.colourAttachments)
		{
			VkAttachmentReference ref = {};
			ref.attachment = curIndex++;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription desc = {};
			desc.format = colourAttachment.format;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = colourAttachment.loadOp;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

			attachmentRefs.push_back(ref);
			attachmentDescriptions.push_back(desc);
		}

		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// parse our depth attachments, (only attach if applicable!)
		if (key.depthAttachment.format != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentReference depthRef;
			depthRef.attachment = curIndex++;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpassDesc.pDepthStencilAttachment = &depthRef;

			VkAttachmentDescription desc = {};
			desc.format = key.depthAttachment.format;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;

			desc.loadOp = key.depthAttachment.loadOp;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachmentDescriptions.push_back(desc);
		}
		subpassDesc.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size());
		subpassDesc.pColorAttachments = attachmentRefs.data();

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		createInfo.pAttachments = attachmentDescriptions.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDesc;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		const auto success = vkCreateRenderPass(*device, &createInfo, nullptr, &renderpass);
		Assert(success == VK_SUCCESS, "Failed to create renderpass");
	}


	void RenderpassCache::ClearEntry(Renderpass* renderpass) { vkDestroyRenderPass(*device, renderpass->GetHandle(), nullptr); }

	bool RenderpassCache::Add(const RenderpassKey& key)
	{
		if (cache.find(key) != cache.end()) return false;

		cache.emplace(key, new Renderpass(device, key));

		return true;
	}

	Renderpass* RenderpassCache::Get(const RenderpassKey& key)
	{
		auto& renderPass = cache[key];
		if (!renderPass) { renderPass = new Renderpass(device, key); }
		return renderPass;
	}
}
