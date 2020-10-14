#pragma once
#include <string>
#include <vulkan.h>

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
		VkRenderPass defaultPass;

		VkRenderPass GetDefaultRenderpass() { return defaultPass; }
		
		VkExtent2D GetSwapchainExtent();
		RenderpassCache* GetRenderpassCache();
		GraphicsPipelineCache* GetGraphicsPipelineCache();
		//ComputePipelineCache* GetComputePipelineCache();
		DescriptorSetCache* GetDescriptorSetCache();
	};
}
