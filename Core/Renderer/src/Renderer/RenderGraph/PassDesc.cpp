#include "PassDesc.h"

#include "GraphContext.h"
#include "RenderGraph.h"
#include "Resource.h"
#include "../../Utils/Logging.h"

namespace Renderer::RenderGraph
{

	PassDesc::PassDesc(const std::string& name, RenderGraph* graph, uint32_t passId, uint32_t queueFamily)
		: name(name), graph(graph), passId(passId), queueIndex(queueFamily)
	{
		
	}

	bool PassDesc::WritesTo(const std::string& name)
	{
		for ( auto res : writtenResources)
		{
			if (res == name) return true;
		}
		return false;
	}
	bool PassDesc::ReadsFrom(const std::string& name)
	{
		for ( auto res : readResources)
		{
			if (res == name) return true;
		}
		return false;
	}
	
	PassDesc& PassDesc::ReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.ReadBy(usage);

		readResources.emplace_back(name);
		
		return *this;
	}
	
	PassDesc& PassDesc::ReadImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, VkImageLayout expectedLayout)
	{		
		const Usage usage = { passId, flags, access, queueIndex, expectedLayout };

		auto& res = graph->GetImage(name);
		res.ReadBy(usage);

		readResources.emplace_back(name);

		return *this;
	}
	
	PassDesc& PassDesc::WriteBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const BufferInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		writtenResources.emplace_back(name);

		return *this;
	}
	
	PassDesc& PassDesc::WriteImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const ImageInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };
		
		auto& res = graph->GetImage(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		writtenResources.emplace_back(name);

		return *this;
	}

	PassDesc& PassDesc::WriteToBackbuffer(VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };
		
		auto& res = graph->GetImage(graph->GetBackBuffer());
		res.WrittenBy(usage);

		writtenResources.emplace_back(graph->GetBackBuffer());

		return *this;
	}

	PassDesc& PassDesc::ReadFeedbackImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetImage(name);

		feedbackResources.emplace_back(name);
		
		return *this;
	}

	PassDesc& PassDesc::SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> func)
	{
		this->execute = std::move(func);

		return *this;
	}



}