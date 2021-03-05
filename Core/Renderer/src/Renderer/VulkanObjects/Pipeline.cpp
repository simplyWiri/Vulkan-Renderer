#include "tracy/Tracy.hpp"

#include "Renderer/Resources/ShaderProgram.h"
#include "Pipeline.h"

namespace Renderer
{
	DepthSettings DepthSettings::DepthTest()
	{
		DepthSettings settings;
		settings.depthFunc = VK_COMPARE_OP_LESS;
		settings.writeEnable = VK_TRUE;

		return settings;
	}

	DepthSettings DepthSettings::Disabled()
	{
		DepthSettings settings;
		settings.depthFunc = VK_COMPARE_OP_ALWAYS;
		settings.writeEnable = VK_FALSE;

		return settings;
	}

	bool DepthSettings::operator==(const DepthSettings& other) const { return std::tie(depthFunc, writeEnable) == std::tie(other.depthFunc, other.writeEnable); }

	VkPipelineColorBlendAttachmentState BlendSettings::createColorAttachmentState(VkColorComponentFlags f, VkBool32 b, VkBlendOp a, VkBlendOp c, VkBlendFactor sc, VkBlendFactor dc)
	{
		VkPipelineColorBlendAttachmentState info = {};
		info.blendEnable = b;
		info.alphaBlendOp = a;
		info.colorBlendOp = c;
		info.colorWriteMask = f;
		info.srcColorBlendFactor = sc;
		info.dstColorBlendFactor = dc;
		return info;
	}

	VkPipelineColorBlendAttachmentState BlendSettings::createColorAttachmentState(VkColorComponentFlags f, VkBool32 b)
	{
		VkPipelineColorBlendAttachmentState info = {};
		info.blendEnable = b;
		info.colorWriteMask = f;

		return info;
	}

	BlendSettings BlendSettings::Opaque()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), false);
		return settings;
	}

	BlendSettings BlendSettings::Add()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_ONE);
		return settings;
	}

	BlendSettings BlendSettings::Mixed()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		return settings;
	}

	BlendSettings BlendSettings::AlphaBlend()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		return settings;
	}

	bool BlendSettings::operator==(const BlendSettings& other) const
	{
		return std::tie(blendState.blendEnable, blendState.alphaBlendOp, blendState.colorBlendOp, blendState.srcColorBlendFactor, blendState.dstColorBlendFactor) == std::tie(other.blendState.blendEnable, other.blendState.alphaBlendOp,
			       other.blendState.colorBlendOp, other.blendState.srcColorBlendFactor, other.blendState.dstColorBlendFactor);
	}

	bool GraphicsPipelineKey::operator==(const GraphicsPipelineKey& other) const
	{
		if (program->getIds() != other.program->getIds()) return false;
		if (topology != other.topology) return false;

		return std::tie(vertexAttributes, depthSetting, blendSettings, renderpass, extent.width, extent.height) == std::tie(other.vertexAttributes, other.depthSetting, other.blendSettings, other.renderpass, other.extent.width,
			       other.extent.height);
	}

	Pipeline::Pipeline(VkDevice device, GraphicsPipelineKey key)
	{
		Assert(!(device == nullptr || key.renderpass == nullptr || key.extent.width == 0 || key.extent.height == 0), "Failed to obtain required information to create the graphics pipeline");

		ZoneScopedN("Creating Graphics Pipeline")

		
		this->device = device;
		key.program->InitialiseResources();
		this->program = key.program;
		this->descriptorSetLayout = key.program->getDescriptorLayout();
		this->pipelineLayout = key.program->getPipelineLayout();

		std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo = {};

		for (const auto& shader : key.program->getShaders())
		{
			switch (shader->getType())
			{
				case ShaderType::Vertex:
				case ShaderType::Fragment:
					// this also populates the field in Pipeline/shaderModules
					shaderCreateInfo.push_back(CreateShaderInfo(CreateShaderModule(shader), shader->getType()));
					break;
				default: LogError("Tried to register currently unsupported shader type");
			}
		}

		auto bindingDescription = key.vertexAttributes.getBindings();
		auto attributeDescription = key.vertexAttributes.getAttributes();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
		vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription.data();
		vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = key.topology;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(key.extent.width);
		viewport.height = static_cast<float>(key.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = key.extent;

		VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = static_cast<uint32_t>(key.blendSettings.size());
		
		auto colourAttachments = std::vector<VkPipelineColorBlendAttachmentState>();
		for (auto& blendSetting : key.blendSettings) { colourAttachments.push_back(blendSetting.blendState); }
		colorBlendCreateInfo.pAttachments = colourAttachments.data();
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = key.depthSetting.writeEnable;
		depthStencil.depthCompareOp = key.depthSetting.depthFunc;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		auto dynamicStates = std::vector<VkDynamicState>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.pStages = shaderCreateInfo.data();
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
		graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = key.renderpass;


		auto success = vkCreateGraphicsPipelines(device, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline) == VK_SUCCESS;
		Assert(success, "Failed to create graphics pipeline");

		for (auto shaderModule : shaderModules) { vkDestroyShaderModule(device, shaderModule, nullptr); }
	}

	Pipeline::Pipeline(VkDevice device, ComputePipelineKey key)
	{
		Assert(!(device == nullptr || key.program == nullptr), "Failed to obtain required information to create the graphics pipeline");

		this->device = device;
		key.program->InitialiseResources();

		VkPipelineShaderStageCreateInfo stage = {};
		for (const auto& shader : key.program->getShaders()) { stage = CreateShaderInfo(CreateShaderModule(shader), shader->getType()); }

		VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		createInfo.stage = stage;
		createInfo.layout = key.program->getPipelineLayout();

		vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline);
	}

	VkShaderModule Pipeline::CreateShaderModule(Shader* shader)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shader->getSize();
		createInfo.pCode = shader->getSPV().data();

		VkShaderModule shaderModule;
		auto success = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS;
		Assert(success, "Failed to create shader module");

		shaderModules.push_back(shaderModule);

		return shaderModule;
	}

	VkPipelineShaderStageCreateInfo Pipeline::CreateShaderInfo(VkShaderModule shaderModule, ShaderType type)
	{
		VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.stage = getFlagBits(type);
		shaderCreateInfo.module = shaderModule;
		shaderCreateInfo.pName = "main";

		return shaderCreateInfo;
	}

	void GraphicsPipelineCache::BindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass pass, const VkExtent2D& extent, const VertexAttributes& vertexAttributes, const DepthSettings& depthSettings,
	                                                 const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology, ShaderProgram* program)
	{
		GraphicsPipelineKey key;
		key.renderpass = pass;
		key.extent = extent;
		key.depthSetting = depthSettings;
		key.blendSettings = blendSettings;
		key.topology = topology;
		key.program = program;
		key.vertexAttributes = vertexAttributes;

		BindGraphicsPipeline(buffer, key);
	}

	void GraphicsPipelineCache::BindGraphicsPipeline(VkCommandBuffer buffer, GraphicsPipelineKey& key)
	{
		ZoneScopedN("Binding Graphics Pipeline")

		auto pipeline = Get(key)->GetPipeline();

		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	Pipeline* GraphicsPipelineCache::Get(const GraphicsPipelineKey& key)
	{
		auto& pipeline = cache[key];
		if (!pipeline) { pipeline = new Pipeline(device, key); }
		return pipeline;
	}

	bool GraphicsPipelineCache::Add(const GraphicsPipelineKey& key)
	{
		if (cache.find(key) != cache.end()) return false;

		cache.emplace(key, new Pipeline(device, key));

		return true;
	}

	void ComputePipelineCache::BindComputePipeline(VkCommandBuffer buffer, ComputePipelineKey key)
	{
		auto pipeline = Get(key)->GetPipeline();

		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	}

	Pipeline* ComputePipelineCache::Get(const ComputePipelineKey& key)
	{
		auto& pipeline = cache[key];
		if (!pipeline) { pipeline = new Pipeline(device, key); }
		return pipeline;
	}

	bool ComputePipelineCache::Add(const ComputePipelineKey& key)
	{
		if (cache.find(key) != cache.end()) return false;

		cache.emplace(key, new Pipeline(device, key));

		return true;
	}
}
