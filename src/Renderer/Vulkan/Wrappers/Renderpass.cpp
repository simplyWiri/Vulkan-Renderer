#include "Renderpass.h"
#include "../../../Utils/Logging.h"

namespace Renderer
{
	Renderpass::Renderpass(VkDevice* device, RenderpassKey key)
		: device(device), colourAttachments(key.colourAttachments)
	{
		std::vector<VkAttachmentReference> attachmentRefs;
		std::vector<VkAttachmentDescription> attachmentDescriptions;

		uint32_t curIndex = 0;
		//// parse our colour attachment(s)

		for (AttachmentDesc colourAttachment : key.colourAttachments) {
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
			desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			attachmentRefs.push_back(ref);
			attachmentDescriptions.push_back(desc);
		}

		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// parse our depth attachments, (only attach if applicable!)
		if (key.depthAttachment.format != VK_FORMAT_UNDEFINED) {
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

			desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			desc.finalLayout = desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

		auto success = vkCreateRenderPass(*device, &createInfo, nullptr, &renderpass);
		Assert(success == VK_SUCCESS, "Failed to create renderpass");
	}
}