#include"../../../Utils/Logging.h"
#include "Pipeline.h"
#include "Context.h"
#include "../../Resources/Vertex.h"
#include "../Caches/PipelineCache.h"

namespace Renderer {
	inline VkPipelineInputAssemblyStateCreateInfo createInputAssemblyState(VkPrimitiveTopology t, VkBool32 p)
	{
		VkPipelineInputAssemblyStateCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.topology = t;
		info.primitiveRestartEnable = p;

		return info;
	}
	inline VkPipelineRasterizationStateCreateInfo createRasterizationState(VkPolygonMode m, VkCullModeFlags c, VkFrontFace f)
	{
		VkPipelineRasterizationStateCreateInfo info = {};
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
	inline VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b, VkBlendOp a, VkBlendOp c, VkBlendFactor sc, VkBlendFactor dc)
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
	inline VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b)
	{
		VkPipelineColorBlendAttachmentState info = {};
		info.blendEnable = b;
		info.colorWriteMask = f;

		return info;
	}
	inline VkPipelineColorBlendStateCreateInfo createColorBlendState(uint32_t c, VkPipelineColorBlendAttachmentState* s)
	{
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
	inline VkPipelineDepthStencilStateCreateInfo createDepthStencilState(VkBool32 w, VkCompareOp c)
	{
		VkPipelineDepthStencilStateCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.depthTestEnable = VK_TRUE;
		info.depthWriteEnable = w;
		info.depthCompareOp = c;
		info.depthBoundsTestEnable = VK_FALSE;
		info.stencilTestEnable = VK_FALSE;

		return info;
	}

	Pipeline::Pipeline(VkDevice* device, GraphicsPipelineKey key)
	{
		if (device == nullptr || key.renderpass == nullptr || key.extent.width == 0 || key.extent.height == 0)
			Assert(false, "Failed to obtain required information to create the graphics pipeline");
		this->device = device;
		this->layout = key.pLayout;
		this->descriptorSetLayout = key.dLayout;

		std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo = {};

		for (const auto& shader : key.shaders)
		{
			if (shader->getStatus() == ShaderStatus::Uninitialised)
			{
				shader->compileGLSL();
				shader->reflectSPIRV();
			}

			switch (shader->getType())
			{
			case ShaderType::Vertex:
			case ShaderType::Fragment:
				// this also populates the field in Pipeline/shaderModules
				shaderCreateInfo.push_back(createShaderInfo(createShaderModule(shader), shader->getType()));
				break;
			default:
				LogError("Tried to register currently unsupported shader type");
			}
		}

		auto bindingDescription = Vertex::getVertexBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();

		// Input assembly create info stage
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = createInputAssemblyState(key.topology, false);

		// Viewport create info stage
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

		//// Rasterizer create info stage
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = createRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		//// Multisampling create info stage
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		//// Colour blending create info stage
		VkPipelineColorBlendAttachmentState colorBlendAttachment = createColorAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = createColorBlendState(1, &colorBlendAttachment);

		//// Creating the pipeline, and tethering it to the struct
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

		//// TODO
		////graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		////graphicsPipelineCreateInfo.pDynamicState = nullptr;

		// we pre-bake the layout in the cache
		graphicsPipelineCreateInfo.layout = key.pLayout;
		graphicsPipelineCreateInfo.renderPass = key.renderpass;
		graphicsPipelineCreateInfo.subpass = 0;
		//graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

		auto success = vkCreateGraphicsPipelines(*device, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline) == VK_SUCCESS;
		Assert(success, "Failed to create graphics pipeline");

		for (auto shaderModule : shaderModules)
		{
			vkDestroyShaderModule(*device, shaderModule, nullptr);
		}
	}

	VkShaderModule Pipeline::createShaderModule(std::shared_ptr<Shader> shader)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shader->getSize();
		createInfo.pCode = shader->getSPV().data();

		VkShaderModule shaderModule;
		auto success = vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS;
		Assert(success, "Failed to create shader module");

		shaderModules.push_back(shaderModule);

		return shaderModule;
	}

	VkPipelineShaderStageCreateInfo Pipeline::createShaderInfo(VkShaderModule module, ShaderType type)
	{
		VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.stage = getFlagBits(type);
		shaderCreateInfo.module = module;
		shaderCreateInfo.pName = "main";

		return shaderCreateInfo;
	}

	/* Helper functions for common use depth / blend settings*/
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
	BlendSettings BlendSettings::Opaque()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), false);
		return settings;
	}
	BlendSettings BlendSettings::Add()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
		return settings;
	}
	BlendSettings BlendSettings::Mixed()
	{
		BlendSettings settings;

		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		return settings;
	}
	BlendSettings BlendSettings::AlphaBlend()
	{
		BlendSettings settings;
		settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		return settings;
	}
}