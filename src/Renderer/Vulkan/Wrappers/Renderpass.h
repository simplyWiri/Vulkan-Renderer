#pragma once
#include "vulkan.h"

namespace Renderer {

	struct AttachmentDesc {
		VkFormat format;
		VkAttachmentLoadOp loadOp;
		VkClearValue clearValue;

		bool operator < (const AttachmentDesc& other) {
			return std::tie(format, loadOp, clearValue) < std::tie(other.format, other.loadOp, other.clearValue);
		}
	};

	struct RenderpassKey {
		RenderpassKey(std::vector<AttachmentDesc> colourAttachments, AttachmentDesc depthAttachment) 
			: colourAttachments(colourAttachments), depthAttachment(depthAttachment) { }

		std::vector<AttachmentDesc> colourAttachments;
		AttachmentDesc depthAttachment;

		bool operator < (const RenderpassKey& other) {
			return std::tie(colourAttachments, depthAttachment) < std::tie(other.colourAttachments, other.depthAttachment);
		}
	};

	struct Renderpass {
	public:
		Renderpass(VkDevice* device, RenderpassKey key) : device(device), colourAttachments(key.colourAttachments)
		{
			std::vector<VkAttachmentReference> attachmentRefs;
			std::vector<VkAttachmentDescription> attachmentDescriptions;

			uint32_t curIndex = 0;
			// parse our colour attachment(s)
			for (AttachmentDesc colourAttachment : key.colourAttachments) {
				VkAttachmentReference ref = {};
				ref.attachment = curIndex++;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				VkAttachmentDescription desc = {};
				desc.format = colourAttachment.format;
				desc.samples = VK_SAMPLE_COUNT_1_BIT;
				// We define the load op, and don't care for the store op
				desc.loadOp = colourAttachment.loadOp;
				desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				// dont care about stencil
				desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				// colour attachment optimal suffices
				desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				attachmentRefs.push_back(ref);
				attachmentDescriptions.push_back(desc);
			}

			VkAttachmentReference depthRef;
			// parse our depth attachments, if it exists
			if (key.depthAttachment.format != VK_FORMAT_UNDEFINED) {
				depthRef.attachment = curIndex++;
				depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				VkAttachmentDescription desc;
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

			VkSubpassDescription subpassDesc;
			subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDesc.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size());
			subpassDesc.pColorAttachments = attachmentRefs.data();
			subpassDesc.pDepthStencilAttachment = &depthRef;

			VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
			createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			createInfo.pAttachments = attachmentDescriptions.data();
			createInfo.subpassCount = 1;
			createInfo.pSubpasses = &subpassDesc;
			createInfo.dependencyCount = 0;
			createInfo.pDependencies = nullptr;	

			vkCreateRenderPass(*device, &createInfo, nullptr, &renderpass);
		}

		inline VkRenderPass getHandle() { return renderpass; }
		inline uint32_t getColourAttachmentCount() { return static_cast<uint32_t>(colourAttachments.size()); }
	private:
		VkDevice* device;
		VkRenderPass renderpass;
		std::vector<AttachmentDesc> colourAttachments;

	};

	class RenderpassCache {
	public:
		RenderpassCache(VkDevice* device) : device(device) { }
		
		inline Renderpass getRenderpass(RenderpassKey key) 
		{ 
			try {
				return renderpasses.at(key);
			}
			catch (...) {
				renderpasses.emplace(new Renderpass(device, key));
				return renderpasses.at(key);
			}
		}

	private:
		VkDevice* device;
		std::map<RenderpassKey, Renderpass> renderpasses;
	};
}