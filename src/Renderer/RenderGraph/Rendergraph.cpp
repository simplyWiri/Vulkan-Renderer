#include "RenderGraph.h"

#include "../Core.h"
#include "imgui.h"
#include "../Memory/Allocator.h"
#include "../Memory/Image.h"
#include "../Memory/Block.h"
#include "../../Utils/DebugVisualisations.h"
#include <examples\imgui_impl_glfw.h>
#include <examples\imgui_impl_vulkan.h>
#include "GraphContext.h"
#include "Tether.h"
#include "PassDesc.h"


namespace Renderer
{
	RenderGraph::RenderGraph(Core* core)
	{
		this->core = core;

		buffers.resize(core->GetSwapchain()->GetFramesInFlight());

		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = core->GetDevice()->GetIndices()->graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto success = vkCreateCommandPool(*core->GetDevice(), &poolCreateInfo, nullptr, &pool);
		Assert(success == VK_SUCCESS, "Failed to create command pool");

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = pool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		success = vkAllocateCommandBuffers(*core->GetDevice(), &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");
	}

	RenderGraph::~RenderGraph()
	{
		vkDestroyCommandPool(*core->GetDevice(), pool, nullptr);

		for(auto& p : passes) p.reset();
		for(auto& r : resources) r.reset();
	}

	ImageResource& RenderGraph::GetImage(const std::string& name)
	{		
		auto val = resourceToIndex.find(name);

		if(val != resourceToIndex.end()) return static_cast<ImageResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new ImageResource(index));
		resources.back()->SetName(name);
		resourceToIndex[name] = index;

		return static_cast<ImageResource&>(*resources.back());
	}

	BufferResource& RenderGraph::GetBuffer(const std::string& name)
	{
		auto val = resourceToIndex.find(name);

		if(val != resourceToIndex.end()) return static_cast<BufferResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new BufferResource(index));
		resources.back()->SetName(name);
		resourceToIndex[name] = index;

		return static_cast<BufferResource&>(*resources.back());
	}

	void RenderGraph::Initialise()
	{
		ExtractGraphInformation();

#if DEBUG
		ValidateGraph();
#endif

		
		/* Todo at a later date*/
		BuildTransients();
		MergePasses();
		BuildBarriers();
	}

	void RenderGraph::Execute()
	{
		core->GetAllocator()->BeginFrame();

		FrameInfo frameInfo;
		VkCommandBuffer buffer = buffers[core->GetSwapchain()->GetIndex()];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		core->BeginFrame(buffer, frameInfo);

		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplVulkan_NewFrame();

		ImGui::NewFrame();
		
		DrawDebugVisualisations(core, frameInfo, passes);

		vkBeginCommandBuffer(buffer, &beginInfo);

		auto* renderpass = core->GetRenderpassCache()->Get(RenderpassKey({ { core->GetSwapchain()->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }, {  }));

		core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, { frameInfo.imageView }, renderpass, core->GetSwapchain()->GetExtent());

		GraphContext context{this, "", renderpass->GetHandle()};

		// foreach pass
		//	bind relevant renderpass - begin
		//     execute pass()
		//  end relevant renderpass;
		
		for (auto& pass : passes)
		{
			context.passId = pass->GetName();
			pass->Execute(buffer, frameInfo, context);
		}

		// if currentPass == finalPass - insert barrier from current_layout -> present_src_khr
		
		core->GetFramebufferCache()->EndPass(buffer);

		vkEndCommandBuffer(buffer);

		core->EndFrame(frameInfo);
		core->GetAllocator()->EndFrame();
	}

	void RenderGraph::Rebuild()
	{
		buffers.clear();
		buffers.resize(core->GetSwapchain()->GetFramesInFlight());

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = pool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		auto success = vkAllocateCommandBuffers(*core->GetDevice(), &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");
	}

	RenderGraphBuilder& RenderGraph::AddPass(const std::string& name, RenderGraphQueue type)
	{
		builders.emplace_back(name, type, this);
		return builders.back();
	}

	void RenderGraph::ExtractGraphInformation()
	{
		// Convert from builder -> pass, and call initialisation function
		for (auto& pass : builders)
		{
			auto passId = static_cast<uint32_t>(passes.size());

			Tether passResources = Tether{ this, passId };

			if (pass.initialisation != nullptr)
			{
				pass.initialisation(passResources);
			}

			passes.emplace_back(std::make_unique<PassDesc>(pass, passResources, passId));
			passToIndex[pass.taskName] = passId;
		}

		builders.clear();

		// Build adjacency list

		auto adjacencyLists = std::vector<std::vector<uint32_t>>(passes.size());

		for(int i = 0; i < passes.size(); i++)
		{
			auto& pass = passes[i];
			
			auto& adjList = adjacencyLists[i];
			
			for(int j = 0; j < passes.size(); j++)
			{
				if(i == j) continue; // Ignore self

				auto& otherPass = passes[j];

				for( auto& readResource : otherPass->GetReadResources())
				{
					bool isDependent = false;
					
					for (auto& writtenResource : pass->GetWrittenResources())
					{
						if(writtenResource.name == readResource.name && writtenResource.type == readResource.type) isDependent = true;

						if(isDependent) break;
					}

					if(isDependent)
					{
						adjList.emplace_back(j);
					}
				}
			}
		}

		
		
	}

	void RenderGraph::ValidateGraph()
	{
		
	}

	void RenderGraph::MergePasses()
	{
		
	}

	void RenderGraph::BuildTransients()
	{
		
	}

	void RenderGraph::BuildBarriers()
	{
		
	}

}
