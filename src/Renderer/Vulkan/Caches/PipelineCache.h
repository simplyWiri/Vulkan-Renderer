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
	private:
		VkDevice* device;

	public:
		void buildCache(VkDevice* device) { this->device = device; }

		void bindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass pass, VkExtent2D extent, DepthSettings depthSettings, std::vector<BlendSettings> blendSettings, VkPrimitiveTopology topology, ShaderProgram program)
		{
			GraphicsPipelineKey key;
			key.renderpass = pass;
			key.extent = extent;
			key.depthSetting = depthSettings;
			key.blendSettings = blendSettings;
			key.topology = topology;
			key.program = program;

			bindGraphicsPipeline(buffer, key);
		}

		void bindGraphicsPipeline(VkCommandBuffer buffer, GraphicsPipelineKey key)
		{
			auto pipeline = get(key)->getPipeline();

			vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		}

		Pipeline* get(const GraphicsPipelineKey& key) override
		{
			auto& pipeline = cache[key];
			if (!pipeline)
			{
				pipeline = new Pipeline(device, key);
				registerInput(key);
			}
			return pipeline;
		}

		bool add(const GraphicsPipelineKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Pipeline(device, key));
			registerInput(key);

			return true;
		}

		bool add(const GraphicsPipelineKey& key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Pipeline(device, key));
			local = registerInput(key);

			return true;
		}

		void clearEntry(Pipeline* pipeline) override
		{
			vkDestroyDescriptorSetLayout(*device, pipeline->getDescriptorLayout(), nullptr);
			vkDestroyPipelineLayout(*device, pipeline->getLayout(), nullptr);
			vkDestroyPipeline(*device, pipeline->getPipeline(), nullptr);
		}

	private:
		void createLayout(const std::vector<Shader*>& shaders, VkDescriptorSetLayout& dLayout,
			VkPipelineLayout& pLayout)
		{
			std::vector<VkPushConstantRange> pushConstants;
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			for (const auto& shader : shaders)
			{
				if (shader->getStatus() == ShaderStatus::Uninitialised)
				{
					shader->compileGLSL();
					shader->reflectSPIRV();
				}

				for (const auto& resource : shader->getResources())
				{
					if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
					{
						// push constant (ref. Shader.cpp ~ line 400)
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
					binding.pImmutableSamplers = nullptr;

					bindings.push_back(binding);
				}
			}

			VkDescriptorSetLayoutCreateInfo desclayout = {};
			desclayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desclayout.bindingCount = static_cast<uint32_t>(bindings.size());
			desclayout.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(*device, &desclayout, nullptr, &dLayout);

			VkPipelineLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutInfo.setLayoutCount = 1;
			layoutInfo.pSetLayouts = &dLayout;
			layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
			layoutInfo.pPushConstantRanges = pushConstants.data();

			vkCreatePipelineLayout(*device, &layoutInfo, nullptr, &pLayout);
		}
	};
}
