#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "Wrappers/Device.h"
#include "Wrappers/Swapchain.h"
#include "Wrappers/Pipeline.h"
#include "Caches/RenderpassCache.h"
#include "Caches/PipelineCache.h"
#include "Caches/FramebufferCache.h"
#include "../Resources/Buffer.h"

#include "RenderGraph/Rendergraph.h"
#include "../Resources/ShaderManager.h"
#include "Caches/DescriptorSetCache.h"

namespace Renderer
{
	/*
		SwapchainSync (Default) - One copy of the buffer per image in the swapchain
		SingleBuffered - Only one copy of each type of buffer, this should only be used when there are large vertex buffers which are not updated often/quickly
		DoubleBuffered - Two copies of each type of buffer, the optimal scenario, while one buffer is submitted and drawn on, there is an alternate buffer being filled by CPU
		TripleBuffered - Three copies of each type of buffer, used when the CPU can outperform the GPU - Will potentially noticable input lag (rectifiable)
	*/

	enum class RendererBufferSettings { SwapchainSync, SingleBuffered, DoubleBuffered, TripleBuffered, };

	struct RendererSettings
	{
		RendererBufferSettings buffering = RendererBufferSettings::DoubleBuffered;
	};

	class Core
	{
	public:
		Core(int x = 640, int y = 400, const char* name = "Vulkan Renderer");

		bool Initialise();
		bool Run();

		~Core();
	private:

		Device device;
		Swapchain swapchain; // VkSwapchainKHR

		// caches
		RenderpassCache renderpassCache;
		GraphicsPipelineCache graphicsPipelineCache;
		FramebufferCache framebufferCache;
		DescriptorSetCache descriptorCache;
		ShaderManager shaderManager;

		std::unique_ptr<Rendergraph> rendergraph;

	private:

		void initialiseCommandPool();
		void initialiseDescriptorPool(GraphicsPipelineKey key);
		void initialiseDescriptorSets(GraphicsPipelineKey key);
		void windowResize();

	public:

		Rendergraph* GetRendergraph() { return rendergraph.get(); }

		Device* GetDevice() { return &device; }
		VmaAllocator* GetAllocator() { return device.getAllocator(); }

		Swapchain* GetSwapchain() { return &swapchain; }

		RenderpassCache* GetRenderpassCache() { return &renderpassCache; }
		GraphicsPipelineCache* GetGraphicsPipelineCache() { return &graphicsPipelineCache; }
		FramebufferCache* GetFramebufferCache() { return &framebufferCache; }
		DescriptorSetCache* GetDescriptorSetCache() { return &descriptorCache; }

		ShaderManager* GetShaderManager() { return &shaderManager; }

		void BeginFrame(VkCommandBuffer& buffer, FrameInfo& info);
		void EndFrame(FrameInfo info);

	private:
		RendererSettings settings;
		uint32_t maxFramesInFlight;
		int bufferCopies = 2;
	};
}
