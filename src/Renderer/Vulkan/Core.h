#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"
#include "Device.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "Renderpass.h"
#include "Framebuffer.h"
#include "../Resources/Buffer.h"

#include "RenderGraph/Rendergraph.h"
#include "../Resources/ShaderManager.h"
//#include "Memory/Allocator.h"

namespace Renderer
{
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
			Memory::Allocator* allocator;

			// caches
			RenderpassCache renderpassCache;
			GraphicsPipelineCache graphicsPipelineCache;
			FramebufferCache framebufferCache;
			DescriptorSetCache descriptorCache;
			ShaderManager* shaderManager;

			VkCommandPool commandPool;
			//Memory::Allocator allocator;
			std::unique_ptr<Rendergraph> rendergraph;

		private:

			void initialiseCommandPool();
			void initialiseDescriptorPool(GraphicsPipelineKey key);
			void initialiseDescriptorSets(GraphicsPipelineKey key);
			void windowResize();

		public:

			Rendergraph* GetRendergraph() { return rendergraph.get(); }

			Device* GetDevice() { return &device; }
			Memory::Allocator* GetAllocator() { return allocator; }

			Swapchain* GetSwapchain() { return &swapchain; }

			RenderpassCache* GetRenderpassCache() { return &renderpassCache; }
			GraphicsPipelineCache* GetGraphicsPipelineCache() { return &graphicsPipelineCache; }
			FramebufferCache* GetFramebufferCache() { return &framebufferCache; }
			DescriptorSetCache* GetDescriptorSetCache() { return &descriptorCache; }

			ShaderManager* GetShaderManager() { return shaderManager; }


			void SetImageLayout(VkCommandBuffer buffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

			VkCommandBuffer GetCommandBuffer(VkCommandBufferLevel level, bool begin);
			void FlushCommandBuffer(VkCommandBuffer buffer);

			void BeginFrame(VkCommandBuffer& buffer, FrameInfo& info);
			void EndFrame(FrameInfo info);

		private:
			RendererSettings settings;
			uint32_t maxFramesInFlight;
			int bufferCopies = 2;
	};
}
