#pragma once
#include <string>
#include <vulkan.h>

struct GLFWwindow;

namespace Renderer
{
	class RenderpassCache;
	class GraphicsPipelineCache;
	class ComputePipelineCache;
	class DescriptorSetCache;
}

namespace Renderer::RenderGraph
{
	class RenderGraph;


	struct GraphContext
	{
		RenderGraph* graph;
		std::string passId;
		GLFWwindow* window;
		VkRenderPass renderPass;
		VkExtent2D extent;
		
		VkRenderPass GetRenderpass() const { return renderPass; }
		VkExtent2D GetExtent() const { return extent; }
		
		RenderpassCache* GetRenderpassCache();
		GraphicsPipelineCache* GetGraphicsPipelineCache();
		//ComputePipelineCache* GetComputePipelineCache();
		DescriptorSetCache* GetDescriptorSetCache();
	};
}
