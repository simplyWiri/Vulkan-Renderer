#pragma once
#include <functional>
#include <optional>
#include "GraphContext.h"
#include "Resource.h"

namespace Renderer
{
	struct FrameInfo;
}

namespace Renderer::RenderGraph
{
	class RenderGraph;
	struct GraphContext;
	
	class PassDesc
	{
	friend class RenderGraph;

		std::string name;
		RenderGraph* graph;
		uint32_t passId;
		uint32_t dependencyGraphIndex;
		uint32_t queueIndex;

		std::vector<std::string> readResources;
		std::vector<std::string> writtenResources;
		std::vector<std::string> feedbackResources;

		std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> execute;

	private:
		bool WritesTo(const std::string& name);
		bool ReadsFrom(const std::string& name);

	public:
		std::string GetName() const { return name; }
		uint32_t GetPassId() const { return passId; }
		uint32_t GetQueueIndex() const { return queueIndex; }
		
	public:
		PassDesc(const std::string& name, RenderGraph* graph, uint32_t passId, uint32_t queueFamily);
		
		// Resource Name, and when will it be read
		PassDesc& ReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access);
		
		// Resource Name, when will it be read, and in what layout is it expected to be in
		PassDesc& ReadImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, VkImageLayout expectedLayout = VK_IMAGE_LAYOUT_GENERAL);

		// Resource name, when is it written to, what is it
		PassDesc& WriteBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const BufferInfo& info);
		
		// Resource name, when is it written to, what is it
		PassDesc& WriteImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const ImageInfo& info);

		// Write to the backbuffer (swapchain)
		PassDesc& WriteToBackbuffer(VkPipelineStageFlags flags, VkAccessFlags access);
		
		// Read only
		PassDesc& ReadFeedbackImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access);

		// Set the record function for the pass
		PassDesc& SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> func);
	};

}
