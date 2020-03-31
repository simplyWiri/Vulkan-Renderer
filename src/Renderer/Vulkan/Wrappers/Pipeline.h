#pragma once
#include "vulkan.h"
#include "..\..\Resources\Shader.h" /* Accessing Shader Functionality */
#include <vector>

struct Renderpass;

struct Pipeline
{
public:



private:
	std::vector<Shader> shaders;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkPipelineCache cache;
	
	Renderpass* renderpass; /* Renderpass to target*/

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorPool descriptorPool;
};

namespace Wrappers {

	bool buildPipeline(Pipeline pipeline);


}

