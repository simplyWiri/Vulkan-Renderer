#pragma once
#include "vulkan.h"
#include "..\..\Resources\Shader.h" /* Accessing Shader Functionality */
#include <vector>
#include <memory>

namespace Renderer
{
	struct DepthSettings
	{
		VkCompareOp depthFunc;
		bool writeEnable;
		
		static DepthSettings DepthTest();
		static DepthSettings Disabled();
		
		bool operator < (const DepthSettings& other) const
		{
			return std::tie(depthFunc, writeEnable) < std::tie(other.depthFunc, other.writeEnable);
		}
	};

	struct BlendSettings
	{
		VkPipelineColorBlendAttachmentState blendState;
		
		static BlendSettings Opaque();
		static BlendSettings Add();
		static BlendSettings Mixed();
		static BlendSettings AlphaBlend();

		bool operator < (const BlendSettings& other) const
		{
			return
				std::tie(blendState.blendEnable, blendState.alphaBlendOp, blendState.colorBlendOp, blendState.srcColorBlendFactor, blendState.dstColorBlendFactor) <
				std::tie(other.blendState.blendEnable, other.blendState.alphaBlendOp, other.blendState.colorBlendOp, other.blendState.srcColorBlendFactor, other.blendState.dstColorBlendFactor);
		}

	};

	struct GraphicsPipelineKey
	{
		GraphicsPipelineKey()
		{
			renderpass = nullptr;
		}
		std::vector<std::shared_ptr<Shader>> shaders;
		VkRenderPass renderpass;
		VkPipelineLayout pLayout;
		VkDescriptorSetLayout dLayout;
		VkExtent2D extent;
		DepthSettings depthSetting;
		std::vector<BlendSettings> blendSettings;
		VkPrimitiveTopology topology;

		bool operator < (const GraphicsPipelineKey& other) const
		{
			return
				std::tie(shaders, pLayout, depthSetting, blendSettings, topology, renderpass) <
				std::tie(other.shaders, other.pLayout, other.depthSetting, other.blendSettings, other.topology, other.renderpass);
		}

	};

	struct Pipeline
	{
	private:
		VkDevice* device;
		VkPipeline pipeline;
		VkPipelineLayout layout;

		std::vector<VkShaderModule> shaderModules;
		std::vector<VkPushConstantRange> pushConstants;
		VkDescriptorSetLayout descriptorSetLayout;

	public:
		Pipeline(VkDevice* device, GraphicsPipelineKey key);

		VkPipeline& getPipeline() { return this->pipeline; }
		VkPipelineLayout& getLayout() { return this->layout; }

		inline VkDescriptorSetLayout& getDescriptorLayout() { return this->descriptorSetLayout; }

	private:

		VkShaderModule createShaderModule(std::shared_ptr<Shader> shader);
		VkPipelineShaderStageCreateInfo createShaderInfo(VkShaderModule module, ShaderType type);

	};
}
