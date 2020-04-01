#include "Pipeline.h"
#include "Context.h"

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
inline VkPipelineColorBlendAttachmentState& createColorAttachmentState(VkColorComponentFlags f, VkBool32 b) {
	VkPipelineColorBlendAttachmentState info;
	info.colorWriteMask = f;
	info.blendEnable = b;

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
inline VkPipelineDepthStencilStateCreateInfo& createDepthStencilState(VkBool32 t, VkBool32 w, VkCompareOp c) {
	VkPipelineDepthStencilStateCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.depthTestEnable = t;
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
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = resource.binding;
		binding.descriptorCount = resource.descriptorCount;
		binding.descriptorType = resource.type;
		binding.stageFlags = resource.flags;

		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo layout = {};
	layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout.bindingCount = static_cast<uint32_t>(bindings.size());
	layout.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(context->getDevice(), &layout, nullptr, &this->descriptorSetLayout) != VK_SUCCESS)
		return false;

	return true;
}

namespace Wrappers {
	bool buildGraphicsPipeline(Pipeline pipeline, PipelineSettings settings, std::initializer_list<Shader*> shaders) {
		std::vector<ShaderResources> resources;
		for (const auto& shader : shaders) {
			if (shader->getStatus() == ShaderStatus::Compiled) { // We don't need to recompile, just reflect
				shader->reflectSPIRV(resources);
			}
			else {
				shader->compileGLSL();
				shader->reflectSPIRV(resources);
			}
		}
		// We now have all of our shader resources required to make a pipeline layout (Descriptor Layouts + Push Constants)
		pipeline.setResources(resources);

		pipeline.createLayout(); // This creates our VkPipelineLayout, and populates our pipelines VkDescriptorLayout

		return true;
	}
	bool buildComputePipeline(Pipeline pipeline, PipelineSettings settings, std::initializer_list<Shader*> shaders) {
		return true;
	}
}