#pragma once
#include "../../Resources/Shader.h"
#include "vulkan.h"
#include <vector>
#include <tuple>

#include "../../Resources/ShaderManager.h"
#include "../Wrappers/Pipeline.h"

namespace Renderer
{

	struct Tether
	{
		std::string passId;
		ShaderManager* shaderManager;

		static std::string getDefaultRenderpass() { return "main:renderpass"; }
		static std::string getDefaultExtent() { return "main:renderpass";}
		
		std::vector<std::tuple<std::string, VkBufferUsageFlags>> buffers;
		std::vector<std::tuple<std::string, VkImageUsageFlags>> images;
		std::vector<std::tuple<std::string, VkShaderStageFlagBits, std::string>> shaders;

		struct PipelineInfo
		{
			std::string id;
			std::string rpId;
			std::string extId;
			GraphicsPipelineKey key;
		};
		std::vector<PipelineInfo> pipelineKeys;
		
		/* Buffers */
		void RegisterBuffer(std::string id, VkBufferUsageFlags usageFlags)
		{
			buffers.emplace_back(passId + id, usageFlags);
		}
		void RegisterImage(std::string id, VkImageUsageFlags usageFlags)
		{
			images.emplace_back(passId + id, usageFlags);
		}



	};

}
