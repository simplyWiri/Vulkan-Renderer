#include "GraphContext.h"
#include "RenderGraph.h"
#include "../Core.h"

namespace Renderer::RenderGraph
{
	RenderpassCache* GraphContext::GetRenderpassCache() { return graph->GetCore()->GetRenderpassCache(); }
	DescriptorSetCache* GraphContext::GetDescriptorSetCache() { return graph->GetCore()->GetDescriptorSetCache(); }
	GraphicsPipelineCache* GraphContext::GetGraphicsPipelineCache() { return graph->GetCore()->GetGraphicsPipelineCache(); }
}