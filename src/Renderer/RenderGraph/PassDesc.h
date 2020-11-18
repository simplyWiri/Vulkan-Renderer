#pragma once
#include <functional>
#include <string>
#include <vector>
#include <vulkan.h>

#include "Resource.h"

namespace Renderer
{
	struct FrameInfo;
	class RenderGraph;
	struct GraphContext;
	enum class QueueType : char;


	class PassDesc
	{
	friend class RenderGraph;
		std::string name;
		RenderGraph* graph;
		uint32_t passId;
		uint32_t dependencyGraphIndex;
		uint32_t queueIndex;

		std::vector<Resource> readResources;
		std::vector<Resource> writtenResources;
		std::vector<Resource> feedbackResources;

		std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> execute;

	private:
		bool WritesTo(const std::string& name);
		bool ReadsFrom(const std::string& name);
		

	public:
		PassDesc(const std::string& name, RenderGraph* graph, uint32_t passId, uint32_t queueFamily);
		
		// Resource Name, and when will it be read
		PassDesc& AddReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access);
		
		// Resource Name, when will it be read, and in what layout is it expected to be in
		PassDesc& AddReadImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, VkImageLayout expectedLayout = VK_IMAGE_LAYOUT_GENERAL);

		// Resource name, when is it written to, what is it
		PassDesc& AddWrittenBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const BufferInfo& info);
		
		// Resource name, when is it written to, what is it
		PassDesc& AddWrittenImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const ImageInfo& info);

		// Read only
		PassDesc& AddFeedbackImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access);

		// Writes directly to the backbuffer, if the backbuffer is written to, this layer will use it as a basis
		PassDesc& AddGuiOutput();

		// Set the record function for the pass
		PassDesc& SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> func);
	};

}
