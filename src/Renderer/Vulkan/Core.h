#pragma once
#include "Wrappers/Window.h"
#include "Wrappers/Context.h"
#include "Wrappers/Swapchain.h"
#include "Wrappers/Pipeline.h"
#include "Wrappers/Renderpass.h"
#include "Wrappers/Framebuffer.h"
#include "Caches/RenderpassCache.h"
#include "Caches/PipelineCache.h"
#include "Caches/FramebufferCache.h"
#include "RenderGraph/Rendergraph.h"

#include "vk_mem_alloc.h"

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
		RendererBufferSettings buffering = RendererBufferSettings::SwapchainSync;
	};

	class Core
	{
	public:
		Core(int x = 640, int y = 400, const char* name = "Vulkan Renderer");

		bool Initialise();

		~Core();
	private:

		Window		window; // GLFWwindow, Surface
		Context		context; // VkDevice, VkInstance, VkPhysicalDevice
		Swapchain	swapchain; // VkSwapchainKHR

		// caches
		RenderpassCache renderpassCache;
		GraphicsPipelineCache pipelineCache;
		FramebufferCache framebufferCache;

		VmaAllocator allocator;
		
		std::unique_ptr<Rendergraph> rendergraph;

	private:

		void initialiseAllocator();

	public:

		Rendergraph* Rendergraph() { return rendergraph.get(); }


		VkDevice LogicalDevice() { return context.getDevice(); }
		VkPhysicalDevice PhysicalDevice() { return context.getPhysicalDevice(); }
		VkSwapchainKHR Swapchain() { return *swapchain.getSwapchain(); }
		VkExtent2D Extent() { return swapchain.getExtent(); }
		VkFormat Format() { return swapchain.getFormat(); }


	private:
		RendererSettings settings;
		uint32_t maxFramesInFlight;
		uint32_t bufferCopies;
	};
}