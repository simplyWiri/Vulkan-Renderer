#include "RenderGraph.h"
#include "../Core.h"
#include <unordered_set>

namespace Renderer
{

	RenderGraph::RenderGraph(Core* core)
		: framesInFlight(core->GetSwapchain()->GetFramesInFlight()), device(*core->GetDevice())
	{
		backBuffer.images = core->GetSwapchain()->GetImages();

		// Initialise our 3 queues
		
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
	}

	PassDesc& RenderGraph::AddPass(const std::string& name, QueueType type)
	{
		auto val = nameToPass.find(name);

		if(val != nameToPass.end()) return static_cast<PassDesc&>(*renderPasses[val->second]);
		
		const auto index = static_cast<uint32_t>(renderPasses.size());
		int queueFamily = 0;
		switch(type)
		{
			case QueueType::Graphics: case QueueType::Compute:
				queueFamily = queues.graphics.queueFamilyIndex; break;
			case QueueType::Transfer: 
				queueFamily = queues.transfer.queueFamilyIndex; break;
			case QueueType::AsyncCompute: 
				queueFamily = queues.compute.queueFamilyIndex; break;
		}
		
		renderPasses.emplace_back(new PassDesc(name, this, index, queueFamily));
		nameToPass[name] = index;

		return static_cast<PassDesc&>(*renderPasses.back());
	}
	
	PassDesc& RenderGraph::GetPass(const std::string& name)
	{
		auto val = nameToPass.find(name);

		if(val != nameToPass.end()) return static_cast<PassDesc&>(*renderPasses[val->second]);
		
		Assert(false, "Failed to find pass");
		
		return static_cast<PassDesc&>(*renderPasses.back());
	}
	
	ImageResource& RenderGraph::GetImage(const std::string& name)
	{
		if(name == backBuffer.name) return backBuffer;
		
		auto val = nameToResource.find(name);

		if(val != nameToResource.end()) return static_cast<ImageResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new ImageResource(name));
		nameToResource[name] = index;

		return static_cast<ImageResource&>(*resources.back());
	}
	
	BufferResource& RenderGraph::GetBuffer(const std::string& name)
	{
		auto val = nameToResource.find(name);

		if(val != nameToResource.end()) return static_cast<BufferResource&>(*resources[val->second]);
		
		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new BufferResource(name));
		nameToResource[name] = index;

		return static_cast<BufferResource&>(*resources.back());
	}
	
	void RenderGraph::Build()
	{
		CreateGraph();
		ValidateGraph();
		CreateResources();
	}

	void RenderGraph::Clear()
	{
		renderPasses.clear();
		resources.clear();

		nameToPass.clear();
		nameToResource.clear();

		vkDestroyCommandPool(device, queues.graphics.commandPool, nullptr);
		vkDestroyCommandPool(device, queues.transfer.commandPool, nullptr);
		vkDestroyCommandPool(device, queues.compute.commandPool, nullptr);
	}
	
	void RenderGraph::Execute()
	{
	
	}
	
	void RenderGraph::CreateGraph()
	{
		// Create Adjacency list
		auto adjacencyList = std::vector<std::vector<uint32_t>>(renderPasses.size());
		CreateAdjacencyList(adjacencyList);

		// Sort
		TopologicalSort(adjacencyList);
	}

	void RenderGraph::CreateAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		for(auto i = 0; i < renderPasses.size(); i++)
		{
			auto& pass = renderPasses[i];
			auto& adjacentIndices = adjacencyList[i];

			for(auto j = 0; j < renderPasses.size(); j++)
			{
				if(i == j) continue;

				auto& otherPass = renderPasses[j];

				for ( auto& res : otherPass->readResources )
				{					
					for ( auto& writes : res.writes )
					{
						if ( writes.passId == pass->passId )
						{
							adjacentIndices.emplace_back(otherPass->passId);
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

	    			renderPasses[i]->dependencyGraphIndex = currentDependencyLevel;
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

		auto copyPasses = std::vector<std::unique_ptr<PassDesc>>();
		
		for( auto& pass : sortedPassOrder)
		{
			copyPasses.emplace_back( std::move(renderPasses[pass]) );
			copyPasses.back()->dependencyGraphIndex = currentDependencyLevel - copyPasses.back()->dependencyGraphIndex; 
		}

		renderPasses = std::move(copyPasses);
	}

	bool RenderGraph::ValidateGraph()
	{
		// Ensure there is a pass which writes to the backbuffer

		bool backBufferWritten = false;
		for ( auto& pass : renderPasses)
		{
			if(pass->WritesTo(backBuffer.name))
			{
				backBufferWritten = true;
				break;
			}
		}

		Assert(backBufferWritten, "No pass writes to the backbuffer");
		if(!backBufferWritten) return false;

		return true;
	}
	
	void RenderGraph::CreateResources()
	{
		
	}

}