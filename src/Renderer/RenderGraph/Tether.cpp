#include "Tether.h"

#include "PassDesc.h"
#include "RenderGraph.h"

namespace Renderer
{
	void Tether::AddReadDependencyImage(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, VkImageLayout layout)
	{
		AccessedResource img = {};
		img.type = ResourceType::Image;
		img.layout = layout;
		img.access = accessFlags;
		img.stages = stageFlags;
		img.name = name;
		
		graph->GetImage(name).ReadBy(passId);

		readResources.emplace_back(img);
	}
	
	void Tether::AddReadDependencyBuffer(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags)
	{
		AccessedResource buf = {};
		buf.type = ResourceType::Buffer;
		buf.access = accessFlags;
		buf.stages = stageFlags;
		buf.name = name;
		
		graph->GetBuffer(name).ReadBy(passId);

		readResources.emplace_back(buf);
	}

	void Tether::AddWriteDependencyImage(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, const ImageInfo& info)
	{
		AccessedResource img = {};
		img.type = ResourceType::Image;
		img.layout = info.layout;
		img.access = accessFlags;
		img.stages = stageFlags;
		img.name = name;

		if(name != graph->GetBackBuffer())
		{
			auto& image = graph->GetImage(name);
			image.WrittenTo(passId);
		
			image.CreatedBy(passId);
			image.AssignInformation(info);
		}
		
		writtenResources.emplace_back(img);
	}
	
	void Tether::AddWriteDependencyBuffer(const std::string& name, VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags, const BufferInfo& info)
	{
		AccessedResource buf = {};
		buf.type = ResourceType::Image;
		buf.access = accessFlags;
		buf.stages = stageFlags;
		buf.name = name;
		
		auto& buffer= graph->GetBuffer(name);
		buffer.WrittenTo(passId);
		
		buffer.CreatedBy(passId);
		buffer.AssignInformation(info);
		
		
		writtenResources.emplace_back(buf);
	}
}
