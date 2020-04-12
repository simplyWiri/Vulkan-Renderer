#pragma once
#include "../../Resources/Shader.h"
#include "../../Resources/ShaderProgram.h"
#include "Pipeline.h"
#include <map>
#include <iostream>

// Taking inspiration	 from Susilk/Raikiri's engine 'LegitVulkan'
// Cache and Key relation

namespace Renderer {

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
			return std::tie(vertexShader, fragmentShader, renderpass, layout, extent, depthSetting, blendSettings, topology) <
				std::tie(other.vertexShader, other.fragmentShader, other.renderpass, other.layout, other.extent, other.depthSetting, other.blendSettings, other.topology);
		}
	};

	struct ComputePipelineKey
	{

	};

	class PipelineCache
	{
	public:
		// We want to move shader ownership into this class; It will contain; Pipeline(s), Shaders (compiled SPV, Descriptor Sets)
		PipelineCache(VkDevice* device)
		{

		}

		bool bindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass renderpass, DepthSettings depthSettings, const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology, const std::vector<Shader*>& shaders)
		{
			GraphicsPipelineKey key = {};
			key.vertexShader = std::move(shaders[0]);
			key.fragmentShader = std::move(shaders[1]);
			key.depthSetting = depthSettings;
			key.blendSettings = blendSettings;
			key.topology = topology;
			key.layout = createLayout(new ShaderProgram(shaders)); 
			key.renderpass = renderpass;

			vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline(key));
		}

		VkPipeline getPipeline(GraphicsPipelineKey key)
		{
			try {
				return graphicsPipelines.at(key).getPipeline();
			}
			catch (const std::exception& e) {
				std::cout << e.what();
				throw std::runtime_error("Failed to find a suitable pipeline from pipeline key");
			}
		}

	private:
		
		VkPipelineLayout createLayout(ShaderProgram* program)
		{
			VkDescriptorSetLayout layout;
			std::vector<VkPushConstantRange> pushConstants;
			program->getResources(device, layout, pushConstants);

			VkPipelineLayoutCreateInfo layoutInfo;
			layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutInfo.setLayoutCount = 1;
			layoutInfo.pSetLayouts = &layout;
			layoutInfo.pushConstantRangeCount = pushConstants.size();
			layoutInfo.pPushConstantRanges = pushConstants.data();

			VkPipelineLayout pLayout;
			vkCreatePipelineLayout(*device, &layoutInfo, nullptr, &pLayout);
			return pLayout;
		}

		VkDevice* device;
		std::map<GraphicsPipelineKey, Pipeline> graphicsPipelines;
		std::map<ComputePipelineKey, Pipeline> computePipelines;

		
	};

}