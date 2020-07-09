#pragma once
#include "vulkan.h"
#include "..\..\Resources\Shader.h" /* Accessing Shader Functionality */
#include <vector>

namespace Renderer
{
	struct DepthSettings
	{
		static VkPipelineDepthStencilStateCreateInfo DepthTest();
		static VkPipelineDepthStencilStateCreateInfo Disabled();

		VkCompareOp depthFunc;
		bool writeEnable;
		bool operator < (const DepthSettings& other) const
		{
			return std::tie(depthFunc, writeEnable) < std::tie(other.depthFunc, other.writeEnable);
		}
	};

	struct BlendSettings
	{
		static VkPipelineColorBlendAttachmentState Opaque();
		static VkPipelineColorBlendAttachmentState Add();
		static VkPipelineColorBlendAttachmentState Mixed();
		static VkPipelineColorBlendAttachmentState AlphaBlend();

		bool operator < (const BlendSettings& other) const
		{
			return
				std::tie(blendState.blendEnable, blendState.alphaBlendOp, blendState.colorBlendOp, blendState.srcColorBlendFactor, blendState.dstColorBlendFactor) <
				std::tie(other.blendState.blendEnable, other.blendState.alphaBlendOp, other.blendState.colorBlendOp, other.blendState.srcColorBlendFactor, other.blendState.dstColorBlendFactor);
		}

		VkPipelineColorBlendAttachmentState blendState;
	};

	struct GraphicsPipelineKey
	{
		GraphicsPipelineKey()
		{
			vertexShader = nullptr;
			fragmentShader = nullptr;
			renderpass = nullptr;
		}
		Shader* vertexShader;
		Shader* fragmentShader;
		VkRenderPass renderpass;
		VkPipelineLayout layout;
		VkExtent2D extent;
		DepthSettings depthSetting;
		std::vector<BlendSettings> blendSettings;
		VkPrimitiveTopology topology;

		bool operator < (const GraphicsPipelineKey& other) const
		{
			return
				std::tie(vertexShader, fragmentShader, layout, depthSetting, blendSettings, topology, renderpass) <
				std::tie(other.vertexShader, other.fragmentShader, other.layout, other.depthSetting, other.blendSettings, other.topology, other.renderpass);
		}
	};

	struct Pipeline
	{
	public:
		Pipeline(VkDevice* device, GraphicsPipelineKey key);

		VkPipeline& getPipeline() { return this->pipeline; }
		VkPipelineLayout& getLayout() { return this->layout; }

		inline VkDescriptorSetLayout& getDescriptorLayout() { return this->descriptorSetLayout; }

	private:
		VkDevice* device;
		VkPipeline pipeline;
		VkPipelineLayout layout;

		std::vector<VkPushConstantRange> pushConstants;
		VkDescriptorSetLayout descriptorSetLayout;
	};
}
