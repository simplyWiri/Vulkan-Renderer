#pragma once
#include "../../Resources/Shader.h"
#include "Pipeline.h"
#include <map>
#include <iostream>


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
			return std::tie(vertexShader, fragmentShader, layout, extent, depthSetting, blendSettings, topology) <
				std::tie(other.vertexShader, other.fragmentShader, other.layout, other.extent, other.depthSetting, other.blendSettings, other.topology);
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

		GraphicsPipelineKey getGraphicsPipelineKey(VkRenderPass renderpass, DepthSettings depthSettings, const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology, const std::vector<Shader*>& shaders)
		{
			GraphicsPipelineKey key = {};
			key.vertexShader = std::move(shaders[0]);
			key.fragmentShader = std::move(shaders[1]);
			key.depthSetting = depthSettings;
			key.blendSettings = blendSettings;
			key.topology = topology;
			key.layout = createLayout(shaders);
			key.renderpass = renderpass;

			return key;
		}

		bool bindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass renderpass, DepthSettings depthSettings, const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology, const std::vector<Shader*>& shaders)
		{
			GraphicsPipelineKey key = getGraphicsPipelineKey(renderpass, depthSettings, blendSettings, topology, shaders);

			bindGraphicsPipeline(buffer, key);
		}

		bool bindGraphicsPipeline(VkCommandBuffer buffer, GraphicsPipelineKey key)
		{
			try {
				vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline(key)->getPipeline());
			}
			catch (...) {
				return false;
			}
			return true;
		}

		inline Pipeline* getPipeline(GraphicsPipelineKey key)
		{
			try {
				return graphicsPipelines.at(key);
			}
			catch (const std::exception& e) {
				// todo
				//graphicsPipelines.emplace(key, new Pipeline(device, key));
				return graphicsPipelines.at(key);
				std::cout << e.what();
				throw std::runtime_error("Failed to find a suitable pipeline from pipeline key");
			}
		}

	private:

		VkPipelineLayout createLayout(const std::vector<Shader*>& shaders)
		{
			VkDescriptorSetLayout layout;
			std::vector<VkPushConstantRange> pushConstants;

			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (const Shader* shader : shaders) {
				for (const auto& resource : shader->getResources) {
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
			}

			VkDescriptorSetLayoutCreateInfo desclayout = {};
			desclayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desclayout.bindingCount = static_cast<uint32_t>(bindings.size());
			desclayout.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(*device, &desclayout, nullptr, &layout);

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
		std::map<GraphicsPipelineKey, Pipeline*> graphicsPipelines;
		std::map<ComputePipelineKey, Pipeline*> computePipelines;
	};
}