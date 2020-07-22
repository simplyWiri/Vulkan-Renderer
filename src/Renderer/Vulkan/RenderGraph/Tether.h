#pragma once
#include "../../Resources/Shader.h"
#include "vulkan.h"
#include <vector>
#include <tuple>

namespace Renderer
{

	struct Tether
	{
		/* Buffers */
		void RegisterBuffer(std::string id, VkBufferUsageFlags usageFlags)
		{
			buffers.emplace_back(id, usageFlags);
		}
		void RegisterImage(std::string id, VkImageUsageFlags usageFlags)
		{
			images.emplace_back(id, usageFlags);
		}
		void RegisterShader(std::string id, VkShaderStageFlagBits type, std::string path)
		{
			shaders.emplace_back(id, type, path);
		}

		void RegisterPipeline(VkRenderPass renderpass, VkExtent2D extent, DepthSettings depthSettings,
			const std::vector<BlendSettings>& blendSettings, VkPrimitiveTopology topology,
			std::vector<Shader*> shaders)
		{
			GraphicsPipelineKey key = {};
			key.shaders = shaders;
			key.extent = extent;
			key.depthSetting = depthSettings;
			key.blendSettings = blendSettings;
			key.topology = topology;
			key.renderpass = renderpass;

			pipelineKeys.emplace_back(key);
		}

		std::vector<std::tuple<std::string, VkBufferUsageFlags>> buffers;
		std::vector<std::tuple<std::string, VkImageUsageFlags>> images;
		std::vector<std::tuple<std::string, VkShaderStageFlagBits, std::string>> shaders;
		std::vector<GraphicsPipelineKey> pipelineKeys;

	};

}