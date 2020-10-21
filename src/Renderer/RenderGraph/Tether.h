#pragma once
#include "vulkan.h"
#include <string>
#include <vector>
#include "Resource.h"

namespace Renderer
{
	class RenderGraph;
	struct AccessedResource;

	struct Tether
	{
	public:
		RenderGraph* graph;
		uint32_t passId;

		std::vector<AccessedResource> readResources;
		std::vector<AccessedResource> writtenResources;
		
	public:
		Tether(RenderGraph* graph, uint32_t passId) : graph(graph), passId(passId) { }

		void AddReadDependencyImage(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);
		void AddReadDependencyBuffer(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags);

		void AddWriteDependencyImage(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, const ImageInfo& info = ImageInfo{});
		void AddWriteDependencyBuffer(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, const BufferInfo& info);

		friend class RenderGraph;
	};
}
