#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include <vector>
#include <unordered_map>
#include <string>

#include "Pass.h"
#include "PassDesc.h"
#include "../VulkanObjects/Renderpass.h"


namespace Renderer
{
	namespace Memory {
		class Image;
	}

	class Core;
}

namespace Renderer::RenderGraph
{
	enum class QueueType : char { CPU = 1 << 0, Graphics = 1 << 1, Compute = 1 << 2, Transfer = 1 << 3, AsyncCompute = 1 << 4 };
	
	class RenderGraph
	{
	private:
		ImageResource backBuffer{ "_backBuffer"};

		std::unordered_map<std::string, uint32_t> nameToPass;
		std::vector<PassDesc> passDescriptions;
		
		std::vector<std::unique_ptr<Pass>> renderPasses;
		
		std::unordered_map<std::string, uint32_t> nameToResource;
		std::vector<std::unique_ptr<Resource>> resources;

		Core* core;

		Memory::Image* depthImage;
		std::vector<VkCommandBuffer> buffers;

		struct Queue
		{
			VkQueue queue;
			int queueFamilyIndex;
			VkCommandPool commandPool;
		};
		struct Queues
		{
			Queue graphics, compute, transfer;
		} queues;

	public:
		RenderGraph(Core* core);
		~RenderGraph();
				
		PassDesc& AddPass(const std::string& name, QueueType type);
		PassDesc& GetPass(const std::string& name);
		
		ImageResource& GetImage(const std::string& name);
		BufferResource& GetBuffer(const std::string& name);
		Resource& GetResource(const std::string& name);

		Core* GetCore() const { return core; }
		std::string GetBackBuffer() const { return backBuffer.name; }
		
		void Build();
		void Clear();

		void Execute();

	private:

		void CreateGraph(); // 1.  Create the DAG from a list of passes ( assert if cyclic )

		void CreateAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList);
		void TopologicalSort(std::vector<std::vector<uint32_t>>& adjacencyList);
		
		bool ValidateGraph(); // 
		void CreateResources(); //
	};
}
