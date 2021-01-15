#pragma once
#include <string>
#include <vulkan.h>

struct GLFWwindow;

namespace Renderer
{
	class Rendergraph;
	class RenderpassCache;
	class GraphicsPipelineCache;
	class ComputePipelineCache;
	class DescriptorSetCache;

	struct GraphContext
	{
		Rendergraph* graph;
		std::string passId;
		GLFWwindow* window;
		VkRenderPass defaultPass;

		VkRenderPass GetDefaultRenderpass() { return defaultPass; }
		
		VkExtent2D GetSwapchainExtent();
		RenderpassCache* GetRenderpassCache();
		GraphicsPipelineCache* GetGraphicsPipelineCache();
		//ComputePipelineCache* GetComputePipelineCache();
		DescriptorSetCache* GetDescriptorSetCache();
	};
}
