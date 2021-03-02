#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "Pass.h"


namespace Renderer
{
	namespace Memory {
		class Buffer;
		class Image;
	}

	class Core;
}

namespace Renderer::RenderGraph
{
	enum class QueueType : char { CPU = 1 << 0, Graphics = 1 << 1, Compute = 1 << 2, Transfer = 1 << 3, AsyncCompute = 1 << 4 };
	
	class RenderGraph
	{
		friend class GraphBuilder;
	private:
		Core* core;
		std::vector<std::unique_ptr<Pass>> renderPasses;
		
		std::unordered_map<std::string, uint32_t> nameToResource;

		// n images/buffers for the amount of frames in flight
		std::vector<std::vector<Memory::Image*>> images;
		std::vector<std::vector<Memory::Buffer*>> buffers;

		std::vector<VkCommandBuffer> commandBuffers;

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
		RenderGraph(Core* core, GraphBuilder& builder);
		~RenderGraph();

		std::vector<Memory::Image*>& GetImage(const std::string& name);
		std::vector<Memory::Buffer*>& GetBuffer(const std::string& name);

		void Execute();
		
	private:

		void CreatePhysicalResources();
	};
}
