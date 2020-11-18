#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "PassDesc.h"
#include <vector>
#include <vulkan.h>

#include "Resource.h"


namespace Renderer
{
	class Core;
	enum class QueueType : char { CPU = 1 << 0, Graphics = 1 << 1, Compute = 1 << 2, Transfer = 1 << 3, AsyncCompute = 1 << 4 };
	enum class ImageSize : char { Swapchain = 1 << 0, Fixed = 1 << 1 };

	struct Queue
	{
		VkQueue queue;
		int queueFamilyIndex;
		VkCommandPool commandPool;
	};
	
	class RenderGraph
	{
	friend class PassDesc;
		ImageResource backBuffer{ "_backBuffer" };
		VkDevice device;
		uint32_t framesInFlight;

		std::unordered_map<std::string, uint32_t> nameToPass;
		std::vector<std::unique_ptr<PassDesc>> renderPasses;
		
		std::unordered_map<std::string, uint32_t> nameToResource;
		std::vector<std::unique_ptr<Resource>> resources;
				
		struct Queues
		{
			Queue graphics, compute, transfer;
		} queues;

	public:
		std::string GetBackBuffer() const { return backBuffer.name; }
		
	public:
		RenderGraph(Core* core);
		
		PassDesc& AddPass(const std::string& name, QueueType type);
		PassDesc& GetPass(const std::string& name);
		
		ImageResource& GetImage(const std::string& name);
		BufferResource& GetBuffer(const std::string& name);

		void Build();
		void Clear();

		void Execute();

	private:

		void CreateGraph(); // 1.  Create the DAG from a list of passes ( assert if cyclic )

		void CreateAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList);
		void TopologicalSort(std::vector<std::vector<uint32_t>>& adjacencyList);
		
		bool ValidateGraph(); // 2.  make sure backbuffer is written to, ensure read resources exist etc
		void CreateResources(); // 3. create the resources, create sync objects.

	};

}

