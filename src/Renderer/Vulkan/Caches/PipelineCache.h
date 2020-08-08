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

			void bindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass pass, VkExtent2D extent, VertexAttributes vertexAttributes, DepthSettings depthSettings, std::vector<BlendSettings> blendSettings, VkPrimitiveTopology topology, ShaderProgram* program)
			{
				GraphicsPipelineKey key;
				key.renderpass = pass;
				key.extent = extent;
				key.depthSetting = depthSettings;
				key.blendSettings = blendSettings;
				key.topology = topology;
				key.program = program;
				key.vertexAttributes = vertexAttributes;

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

	};
}
