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

	std::vector<ShaderResources> resources;
	VkDescriptorSetLayout descriptorSetLayout;
};

namespace Wrappers {

	bool buildPipeline(Pipeline pipeline, std::initializer_list<Shader> shaders);


}

