#include "RenderGraph.h"

#include "../Core.h"
#include "imgui.h"
#include "../Memory/Allocator.h"
#include "../Memory/Image.h"
#include "../Memory/Block.h"
#include "../../Utils/DebugVisualisations.h"

namespace Renderer::RenderGraph
{
	RenderGraph::RenderGraph(Core* core)
	{
		this->core = core;

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

		backBuffer.info.format = core->GetSwapchain()->GetFormat();
		backBuffer.info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	RenderGraph::~RenderGraph()
	{
		renderPasses.clear();
		resources.clear();

		nameToPass.clear();
		nameToResource.clear();

		vkDestroyCommandPool(*core->GetDevice(), queues.graphics.commandPool, nullptr);
		vkDestroyCommandPool(*core->GetDevice(), queues.transfer.commandPool, nullptr);
		vkDestroyCommandPool(*core->GetDevice(), queues.compute.commandPool, nullptr);
	}

	void RenderGraph::Build()
	{
		CreateGraph();
		ValidateGraph();
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
				auto rp = core->GetRenderpassCache()->Get(pass->key);
				core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, pass->GetViews(frameInfo.offset), rp, pass->GetExtent());

				GraphContext context{this, "", core->GetSwapchain()->GetWindow(), rp->GetHandle(), pass->GetExtent()};

				context.passId = pass->name;
				pass->Execute(buffer, frameInfo, context);

				core->GetFramebufferCache()->EndPass(buffer);
			}
		}


		vkEndCommandBuffer(buffer);

		core->EndFrame(frameInfo);
		core->GetAllocator()->EndFrame();
	}

	PassDesc& RenderGraph::AddPass(const std::string& name, QueueType type)
	{
		auto val = nameToPass.find(name);

		if(val != nameToPass.end()) return static_cast<PassDesc&>(passDescriptions[val->second]);
		
		const auto index = static_cast<uint32_t>(passDescriptions.size());
		int queueFamily = 0;
		switch(type)
		{
			case QueueType::Graphics: case QueueType::Compute: queueFamily = queues.graphics.queueFamilyIndex; break;
			case QueueType::Transfer: queueFamily = queues.transfer.queueFamilyIndex; break;
			case QueueType::AsyncCompute: queueFamily = queues.compute.queueFamilyIndex; break;
		}
		
		passDescriptions.emplace_back(PassDesc(name, this, index, queueFamily));
		nameToPass[name] = index;

		return static_cast<PassDesc&>(passDescriptions.back());
	}

	PassDesc& RenderGraph::GetPass(const std::string& name)
	{
		auto val = nameToPass.find(name);

		if(val != nameToPass.end()) return static_cast<PassDesc&>(passDescriptions[val->second]);
		
		Assert(false, "Failed to find pass");
		
		return static_cast<PassDesc&>(passDescriptions.back());
	}
	
	ImageResource& RenderGraph::GetImage(const std::string& name)
	{
		if(name == backBuffer.name) return backBuffer;
		
		auto val = nameToResource.find(name);

		if(val != nameToResource.end()) return reinterpret_cast<ImageResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new ImageResource(name));
		nameToResource[name] = index;

		return reinterpret_cast<ImageResource&>(*resources.back());
	}
	
	BufferResource& RenderGraph::GetBuffer(const std::string& name)
	{
		auto val = nameToResource.find(name);

		if(val != nameToResource.end()) return reinterpret_cast<BufferResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new BufferResource(name));
		nameToResource[name] = index;

		return reinterpret_cast<BufferResource&>(*resources.back());
	}

	Resource& RenderGraph::GetResource(const std::string& name)
	{
		if(name == backBuffer.name) return backBuffer;
		
		auto val = nameToResource.find(name);

		if(val != nameToResource.end()) return *resources[val->second];

		Assert(false, "Trying to retrieve resource which does not exist.")
	}

	void RenderGraph::CreateGraph()
	{
		// Create Adjacency list
		auto adjacencyList = std::vector<std::vector<uint32_t>>(passDescriptions.size());
		CreateAdjacencyList(adjacencyList);

		// Sort
		TopologicalSort(adjacencyList);

		// Create physical resources
		CreateResources();
	}

	void RenderGraph::CreateAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		for(auto i = 0; i < passDescriptions.size(); i++)
		{
			auto& pass = passDescriptions[i];
			auto& adjacentIndices = adjacencyList[i];

			for(auto j = 0; j < passDescriptions.size(); j++)
			{
				if(i == j) continue;

				auto& otherPass = passDescriptions[j];

				for ( auto& resKey : otherPass.readResources )
				{
					auto res = GetResource(resKey);
					
					for ( auto& writes : res.writes )
					{
						if ( writes.passId == pass.passId )
						{
							adjacentIndices.emplace_back(otherPass.passId);
							break;
						}
					}
				}
			}
		}
	}

	void RenderGraph::TopologicalSort(std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		auto sortedPassOrder = std::vector<uint32_t>();
		
		auto size = adjacencyList.size();
		auto currentDependencyLevel = 0;
		
		auto indexesToCheck = std::vector<uint32_t>();
		auto enqueuedPasses = std::unordered_set<uint32_t>();
		
		for(int i = 0; i < adjacencyList.size(); i++) indexesToCheck.push_back(i);
	    
	    while(sortedPassOrder.size() != size)
	    {
	    	for ( auto i : indexesToCheck )
	    	{
	    		bool canQueue = true;
	    		
	    		for (auto val : adjacencyList[i])
	    		{
	                if (enqueuedPasses.find(val) == enqueuedPasses.end()) // We have found an idx which hasn't been counted yet, continue loop
	                {
		                canQueue = false;
	                	break;
	                }
	    		}

	    		if(canQueue) 
	    		{
	    			sortedPassOrder.push_back(i);

	    			passDescriptions[i].dependencyGraphIndex = currentDependencyLevel;
				}
	    	}

	        enqueuedPasses.clear();
	        for ( auto index : sortedPassOrder)
	        {
				std::erase(indexesToCheck, index); // Don't re-check this node in subsequent passes
	            enqueuedPasses.emplace(index); // ignore passes which require this node in subsequent passes
			}

			++currentDependencyLevel;
	    }		

		
		std::reverse(sortedPassOrder.begin(), sortedPassOrder.end()); // Reverse our order to 

		auto copyPasses = std::vector<PassDesc>();
		
		for( auto& pass : sortedPassOrder)
		{
			copyPasses.emplace_back( std::move(passDescriptions[pass]) );
			copyPasses.back().dependencyGraphIndex = currentDependencyLevel - copyPasses.back().dependencyGraphIndex; 
		}

		passDescriptions = std::move(copyPasses);
	}

	bool RenderGraph::ValidateGraph()
	{
		return true;
	}
	
	void RenderGraph::CreateResources()
	{
		const auto framesInFlight = core->GetSwapchain()->GetFramesInFlight();
		const auto swapchainExtent = core->GetSwapchain()->GetExtent();
		auto* allocator = core->GetAllocator();

		for(auto& passDesc : passDescriptions)
		{
			auto& writtenResources = passDesc.writtenResources;

			std::vector<AttachmentDesc> colorAttachments;
			AttachmentDesc depthAttachment{VK_FORMAT_UNDEFINED};
			
			std::vector<ImageResource*> frameBufferImages;
			for(auto& resKey : writtenResources)
			{
				auto& res = GetResource(resKey);
				
				if(res.type == Resource::Type::Image)
				{
					auto& imgRes = reinterpret_cast<ImageResource&>(res);
					auto info = imgRes.info;
					// Create the actual image from the information given by the image resource.

					if(res.name != backBuffer.name)
						imgRes.Build(allocator, swapchainExtent, framesInFlight);
					
					auto usage = imgRes.info.usage;

					// Presumably compute, this is not used as a framebuffer image.
					if(usage == VK_IMAGE_USAGE_STORAGE_BIT) continue;
					
					auto attachment = AttachmentDesc{imgRes.info.format, VK_ATTACHMENT_LOAD_OP_CLEAR};
					
					// We assume the image is either a depth attachment or a color attachment.. works for now
					if(usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) depthAttachment = attachment;
					else colorAttachments.emplace_back(attachment);
				
					frameBufferImages.emplace_back(&imgRes);
				}
			}

			auto pass = std::make_unique<Pass>();

			pass->name = passDesc.name;
			pass->renderExtent = swapchainExtent;
			pass->execute = std::move(passDesc.execute);
			
			// Set our renderpass key
			auto rpKey = RenderpassKey{colorAttachments, depthAttachment};
			pass->key = std::move(rpKey);

			pass->views.resize(framesInFlight);

			for(auto& imgRes : frameBufferImages)
			{
				if(imgRes->name == backBuffer.name)
				{
					for(int i = 0 ; i < framesInFlight; i++)
						pass->views[i].emplace_back(core->GetSwapchain()->GetImageViews()[i]);
				}
				else
				{
					for(int i = 0 ; i < framesInFlight; i++)
						pass->views[i].emplace_back(imgRes->images[i]->GetView());
				}
				

			}

			renderPasses.emplace_back(std::move(pass));
			
		}
		
	}

}
