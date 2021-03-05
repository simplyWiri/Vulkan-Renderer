#include "imgui/imgui.h"

#include "RenderGraph.h"
#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Image.h"
#include "Renderer/Memory/Block.h"
#include "Utils/DebugVisualisations.h"
#include "Creation/GraphBuilder.h"

namespace Renderer::RenderGraph
{
	RenderGraph::RenderGraph(Core* core, GraphBuilder& builder)
		: core(core)
	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto* dev = core->GetDevice();
		{
			queues.graphics = { dev->queues.graphics, dev->GetIndices()->graphicsFamily };
			info.queueFamilyIndex = queues.graphics.queueFamilyIndex;
			vkCreateCommandPool(*dev, &info, nullptr, &queues.graphics.commandPool);
		}
		{
			queues.compute = { dev->queues.compute, dev->GetIndices()->computeFamily };
			info.queueFamilyIndex = queues.compute.queueFamilyIndex;
			vkCreateCommandPool(*dev, &info, nullptr, &queues.compute.commandPool);
		}
		{
			queues.transfer = { dev->queues.transfer, dev->GetIndices()->transferFamily };
			info.queueFamilyIndex = queues.transfer.queueFamilyIndex;
			vkCreateCommandPool(*dev, &info, nullptr, &queues.transfer.commandPool);
		}
		
		builder.CreateGraph(this);
	}

	RenderGraph::~RenderGraph()
	{
		renderPasses.clear();
		nameToResource.clear();

		vkDestroyCommandPool(*core->GetDevice(), queues.graphics.commandPool, nullptr);
		vkDestroyCommandPool(*core->GetDevice(), queues.transfer.commandPool, nullptr);
		vkDestroyCommandPool(*core->GetDevice(), queues.compute.commandPool, nullptr);
	}

	std::vector<Memory::Image*>& RenderGraph::GetImage(const std::string& name)
	{
		return images[nameToResource[name]];
	}

	std::vector<Memory::Buffer*>& RenderGraph::GetBuffer(const std::string& name)
	{
		return buffers[nameToResource[name]];
	}

	void RenderGraph::Execute()
	{
		core->GetAllocator()->BeginFrame();

		FrameInfo frameInfo;
		VkCommandBuffer buffer = core->GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		core->BeginFrame(buffer, frameInfo);

		{
//#if DEBUG
//			ZoneScopedNC("Drawing Debug Visualisations", tracy::Color::Green)
//			DrawDebugVisualisations(core, frameInfo, renderPasses);
//#else
//			ImGui::NewFrame();
//#endif
		}

		vkBeginCommandBuffer(buffer, &beginInfo);

		{
			ZoneScopedNC("Gathering Draw Commands", tracy::Color::Green)
			
			for (auto& pass : renderPasses)
			{
				auto* rp = core->GetRenderpassCache()->Get(pass->key);
				core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, pass->GetViews(frameInfo.offset), rp, pass->GetExtent());

				GraphContext context{this, "", core->GetSwapchain()->GetWindow(), rp->GetHandle(), pass->GetExtent()};

				context.passId = pass->name;
				pass->Execute(buffer, frameInfo, context);

				core->GetFramebufferCache()->EndPass(buffer);

				if(!pass->imageBarriers.empty())
				{

					vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 
						0, nullptr, 
						0, nullptr, 
						pass->imageBarriers[frameInfo.offset].size(), pass->imageBarriers[frameInfo.offset].data());
				}
			}
		}


		vkEndCommandBuffer(buffer);

		core->EndFrame(frameInfo);
		core->GetAllocator()->EndFrame();
	}

}
