#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "Wrappers/Device.h"
#include "Wrappers/Swapchain.h"
#include "Wrappers/Pipeline.h"
#include "Wrappers/Renderpass.h"
#include "Caches/RenderpassCache.h"
#include "Caches/PipelineCache.h"
#include "Caches/FramebufferCache.h"
#include "../Resources/Buffer.h"

//#include "RenderGraph/Rendergraph.h"


namespace Renderer
{
	/*
		SwapchainSync (Default) - One copy of the buffer per image in the swapchain
		SingleBuffered - Only one copy of each type of buffer, this should only be used when there are large vertex buffers which are not updated often/quickly
		DoubleBuffered - Two copies of each type of buffer, the optimal scenario, while one buffer is submitted and drawn on, there is an alternate buffer being filled by CPU
		TripleBuffered - Three copies of each type of buffer, used when the CPU can outperform the GPU - Will potentially noticable input lag (rectifiable)
	*/

	//switch (settings.buffering) {
	//	case RendererBufferSettings::SwapchainSync: bufferCopies = maxFramesInFlight;
	//	case RendererBufferSettings::SingleBuffered: bufferCopies = 1;
	//	case RendererBufferSettings::DoubleBuffered: bufferCopies = 2;
	//	case RendererBufferSettings::TripleBuffered: bufferCopies = 3;
	//}

	//return true;

	enum class RendererBufferSettings { SwapchainSync, SingleBuffered, DoubleBuffered, TripleBuffered, };

	struct RendererSettings
	{
		RendererBufferSettings buffering = RendererBufferSettings::SwapchainSync;
	};

	class Core
	{
	public:
		Core(int x = 640, int y = 400, const char* name = "Vulkan Renderer");

		bool Initialise();

		~Core();
	private:
		Device device;
		Swapchain swapchain; // VkSwapchainKHR

		// caches
		RenderpassCache renderpassCache;
		GraphicsPipelineCache pipelineCache;
		FramebufferCache framebufferCache;

		VmaAllocator allocator;
		VkCommandPool commandPool;

		Buffer* vertexBuffer;
		Buffer* indexBuffer;

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		
		//std::unique_ptr<Rendergraph> rendergraph;

	private:

		void initialiseAllocator();
		void initialiseCommandPool();
		void initialiseDescriptorPool(GraphicsPipelineKey key);
		void initialiseDescriptorSets(GraphicsPipelineKey key);

	public:

		//Rendergraph* Rendergraph() { return rendergraph.get(); }

	private:
		RendererSettings settings;
		uint32_t maxFramesInFlight;
		uint32_t bufferCopies;
	};
}