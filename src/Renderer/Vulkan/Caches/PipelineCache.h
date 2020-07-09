#pragma once
#include "../../Resources/Shader.h"
#include "../Wrappers/Pipeline.h"
#include "Cache.h"
#include <map>
#include <iostream>

namespace Renderer
{
	class GraphicsPipelineCache : public Cache<Pipeline, GraphicsPipelineKey>
	{
	public:
		// We want to move shader ownership into this class; It will contain; Pipeline(s), Shaders (compiled SPV, Descriptor Sets)
		void buildCache(VkDevice* device)
		{
			this->device = device;
		}

		GraphicsPipelineKey getGraphicsPipelineKey(VkRenderPass renderpass, DepthSettings depthSettings, const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology, std::vector<Shader*> shaders)
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

		void bindGraphicsPipeline(VkCommandBuffer buffer, GraphicsPipelineKey key)
		{
			auto pipeline = get(key)->getPipeline();

			return vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		}

		Pipeline* get(GraphicsPipelineKey key) override
		{
			try {
				return cache[key];
			}
			catch (...) {
				add(key);
				return cache[key];
			}
		}

		bool add(GraphicsPipelineKey key) override
		{
			if (cache.find(key) != cache.end())
				return false;

			cache.emplace(key, new Pipeline(device, key));
			registerInput(key);

			return true;
		}

		bool add(GraphicsPipelineKey key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end())
				return false;

			cache.emplace(key, new Pipeline(device, key));
			local = registerInput(key);

			return true;
		}

		void clearEntry(Pipeline* pipeline) override
		{
			vkDestroyPipelineLayout(*device, pipeline->getLayout(), nullptr);
			vkDestroyPipeline(*device, pipeline->getPipeline(), nullptr);
		}

	private:

		VkPipelineLayout createLayout(const std::vector<Shader*>& shaders)
		{
			VkDescriptorSetLayout layout;
			std::vector<VkPushConstantRange> pushConstants;

			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (const Shader* shader : shaders) {
				for (const auto& resource : shader->getResources()) {
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
			layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
			layoutInfo.pPushConstantRanges = pushConstants.data();

			VkPipelineLayout pLayout;
			vkCreatePipelineLayout(*device, &layoutInfo, nullptr, &pLayout);
			return pLayout;
		}

		VkDevice* device;
		std::map<GraphicsPipelineKey, Pipeline*> graphicsPipelines;
	};
}