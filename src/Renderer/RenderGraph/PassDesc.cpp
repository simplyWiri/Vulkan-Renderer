#include "PassDesc.h"

#include "GraphContext.h"
#include "RenderGraph.h"
#include "Resource.h"
#include "../../Utils/Logging.h"

namespace Renderer
{

	PassDesc::PassDesc(const std::string& name, RenderGraph* graph, uint32_t passId, uint32_t queueFamily)
		: name(name), graph(graph), passId(passId), queueIndex(queueFamily)
	{
		
	}

	bool PassDesc::WritesTo(const std::string& name)
	{
		for ( auto res : writtenResources)
		{
			if (res.name == name) return true;
		}
		return false;
	}
	bool PassDesc::ReadsFrom(const std::string& name)
	{
		for ( auto res : readResources)
		{
			if (res.name == name) return true;
		}
		return false;
	}
	
	PassDesc& PassDesc::AddReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.ReadBy(usage);

		readResources.emplace_back(res);
		
		return *this;
	}
	
	PassDesc& PassDesc::AddReadImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, VkImageLayout expectedLayout)
	{
		Assert(name != graph->GetBackBuffer(), "Reading from swapchain image");
		
		const Usage usage = { passId, flags, access, queueIndex, expectedLayout };

		auto& res = graph->GetImage(name);
		res.ReadBy(usage);

		readResources.emplace_back(res);

		return *this;
	}
	
	PassDesc& PassDesc::AddWrittenBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const BufferInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		writtenResources.emplace_back(res);

		return *this;
	}
	
	PassDesc& PassDesc::AddWrittenImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const ImageInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };
		
		auto& res = graph->GetImage(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		writtenResources.emplace_back(res);

		return *this;
	}

	PassDesc& PassDesc::AddFeedbackImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetImage(name);

		feedbackResources.emplace_back(res);
		
		return *this;
	}

	PassDesc& PassDesc::AddGuiOutput()
	{
		const Usage usage = { passId, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, queueIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		auto& res = graph->GetImage(graph->GetBackBuffer());
		res.ReadBy(usage);

		readResources.emplace_back(res);
		
		return *this;
	}

	PassDesc& PassDesc::SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> func)
	{
		this->execute = std::move(func);

		return *this;
	}



}
