#include "GraphContext.h"
#include "RenderGraph.h"
#include "../Core.h"

namespace Renderer
{
	RenderpassCache* GraphContext::GetRenderpassCache() { return graph->GetCore()->GetRenderpassCache(); }
	VkExtent2D GraphContext::GetSwapchainExtent() { return graph->GetCore()->GetSwapchain()->GetExtent(); }
	DescriptorSetCache* GraphContext::GetDescriptorSetCache() { return graph->GetCore()->GetDescriptorSetCache(); }
	GraphicsPipelineCache* GraphContext::GetGraphicsPipelineCache() { return graph->GetCore()->GetGraphicsPipelineCache(); }

}