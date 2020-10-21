#include "RenderGraphBuilder.h"
#include "PassDesc.h"

namespace Renderer
{
	RenderGraphBuilder& RenderGraphBuilder::SetExtent(VkExtent2D extent)
	{
		this->extent = extent;
		return *this;
	}
	RenderGraphBuilder& RenderGraphBuilder::SetInitialisationFunc(std::function<void(Tether&)> func)
	{
		this->initialisation = std::move(func);
		return *this;
	}
	RenderGraphBuilder& RenderGraphBuilder::SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext&)> func)
	{
		this->execute = std::move(func);
		return *this;	
	}

}
