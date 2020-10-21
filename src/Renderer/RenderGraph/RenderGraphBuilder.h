#pragma once
#include <functional>
#include <optional>
#include "PassDesc.h"

namespace Renderer
{
	struct Tether;
	struct FrameInfo;
	struct GraphContext;
	class RenderGraph;
	enum class RenderGraphQueue;
	
	struct RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(const std::string& name, RenderGraphQueue queueType, RenderGraph* graph) : taskName(name), queueType(queueType), graph(graph) { }

		RenderGraphBuilder& SetExtent(VkExtent2D extent);
		RenderGraphBuilder& SetInitialisationFunc(std::function<void(Tether&)> func);
		RenderGraphBuilder& SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo &, GraphContext&)> func);


		RenderGraph* graph;
		RenderGraphQueue queueType;
		
		std::string taskName;
		std::optional<VkExtent2D> extent;

		std::function<void(Tether&)> initialisation;
		std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext&)> execute;

		std::vector<AccessedResource> readResources;
		std::vector<AccessedResource> writtenResources;
	};
}
