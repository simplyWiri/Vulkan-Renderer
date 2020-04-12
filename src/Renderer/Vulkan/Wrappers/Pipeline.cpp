#include "Pipeline.h"
#include "Context.h"

namespace Renderer {

	inline VkPipelineInputAssemblyStateCreateInfo& createInputAssemblyState(VkPrimitiveTopology t, VkBool32 p) {
		VkPipelineInputAssemblyStateCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.topology = t;
		info.primitiveRestartEnable = p;

		return info;
	}
	inline VkPipelineRasterizationStateCreateInfo& createRasterizationState(VkPolygonMode m, VkCullModeFlags c, VkFrontFace f) {
		VkPipelineRasterizationStateCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.depthClampEnable = VK_FALSE;
		info.rasterizerDiscardEnable = VK_FALSE;
		info.polygonMode = m;
		info.lineWidth = 1.0f;
		info.cullMode = c;
		info.frontFace = f;
		info.depthBiasEnable = VK_FALSE;

		return info;
	}
	// todo - Find default values for color attachment state.
	inline VkPipelineColorBlendAttachmentState& createColorAttachmentState(VkColorComponentFlags f, VkBool32 b, VkBlendOp a, VkBlendOp c, VkBlendFactor sc, VkBlendFactor dc) {
		VkPipelineColorBlendAttachmentState info;
		info.blendEnable = b;
		info.alphaBlendOp = a;
		info.colorBlendOp = c;
		info.colorWriteMask = f;
		info.srcColorBlendFactor = sc;
		info.dstColorBlendFactor = dc;
		return info;
	}
	inline VkPipelineColorBlendAttachmentState& createColorAttachmentState(VkColorComponentFlags f, VkBool32 b) {
		VkPipelineColorBlendAttachmentState info;
		info.blendEnable = b;		
		info.colorWriteMask = f;
		return info;
	}
	inline VkPipelineColorBlendStateCreateInfo& createColorBlendState(uint32_t c, VkPipelineColorBlendAttachmentState* s) {
		VkPipelineColorBlendStateCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		info.logicOpEnable = VK_FALSE;
		info.logicOp = VK_LOGIC_OP_COPY;
		info.attachmentCount = c;
		info.pAttachments = s;
		info.blendConstants[0] = 0.0f;
		info.blendConstants[1] = 0.0f;
		info.blendConstants[2] = 0.0f;
		info.blendConstants[3] = 0.0f;

		return info;
	}
	inline VkPipelineDepthStencilStateCreateInfo& createDepthStencilState(VkBool32 w, VkCompareOp c) {
		VkPipelineDepthStencilStateCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.depthTestEnable = VK_TRUE;
		info.depthWriteEnable = w;
		info.depthCompareOp = c;
		info.depthBoundsTestEnable = VK_FALSE;
		info.stencilTestEnable = VK_FALSE;

		return info;
	}



	bool Pipeline::createLayout()
	{
		// for each resource
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (const auto& resource : resources) {
			if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) { // push constant (ref. Shader.cpp ~ line 400)
				VkPushConstantRange range;
				range.offset = resource.offset;
				range.size = resource.size;
				range.stageFlags = resource.flags;
				pushConstants.push_back(range);
				continue;
			}

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = resource.binding;
			binding.descriptorCount = resource.descriptorCount;
			binding.descriptorType = resource.type;
			binding.stageFlags = resource.flags;

			bindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo desclayout = {};
		desclayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desclayout.bindingCount = static_cast<uint32_t>(bindings.size());
		desclayout.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(context->getDevice(), &desclayout, nullptr, &this->descriptorSetLayout) != VK_SUCCESS)
			return false;

		return true;
	}

	namespace Wrappers {
		
	}

	/* Helper functions for common use depth / blend settings*/
	VkPipelineDepthStencilStateCreateInfo DepthSettings::DepthTest()
	{
		return createDepthStencilState(VK_TRUE, VK_COMPARE_OP_LESS);
	}
	VkPipelineDepthStencilStateCreateInfo DepthSettings::Disabled()
	{
		return createDepthStencilState(VK_FALSE, VK_COMPARE_OP_ALWAYS);
	}

	VkPipelineColorBlendAttachmentState BlendSettings::Opaque()
	{
		return createColorAttachmentState( (VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_B_BIT), false);
	}
	VkPipelineColorBlendAttachmentState BlendSettings::Add()
	{
		return createColorAttachmentState( (VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
	}
	VkPipelineColorBlendAttachmentState BlendSettings::Mixed()
	{
		return createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	}
	VkPipelineColorBlendAttachmentState BlendSettings::AlphaBlend()
	{
		return createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	}

}