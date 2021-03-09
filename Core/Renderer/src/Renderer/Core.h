#pragma once
#include "volk/volk.h"

#include "InputHandler.h"
#include "VulkanObjects/Device.h"
#include "VulkanObjects/Swapchain.h"
#include "VulkanObjects/Pipeline.h"
#include "VulkanObjects/Renderpass.h"
#include "VulkanObjects/Framebuffer.h"
#include "RenderGraph/Creation/GraphBuilder.h"
#include "Resources/ShaderManager.h"
#include "VulkanObjects/DescriptorSet.h"


namespace tracy {
	class VkCtx;
}

namespace Renderer
{
	enum class RendererBufferSettings { SwapchainSync, SingleBuffered, DoubleBuffered, TripleBuffered };

	struct Settings
	{
		RendererBufferSettings buffering = RendererBufferSettings::DoubleBuffered;
		int width = 640;
		int height = 400;
		const char* name = "Renderer";
		bool vsync = false;
		bool validationLayers = false;
		std::vector<std::string> enabledExtensions;
	};

	class Core
	{
	public:
		Core(Settings settings);

		bool Initialise();
		bool Run();

		~Core();
	private:

		Settings settings;

		Device device;
		Swapchain swapchain; 
		Memory::Allocator* allocator;
		FrameInfo frameInfo;
		InputHandler inputHandler;

		// caches
		RenderpassCache renderpassCache;
		GraphicsPipelineCache graphicsPipelineCache;
		FramebufferCache framebufferCache;
		DescriptorSetCache descriptorCache;
		ShaderManager* shaderManager;

		VkCommandPool commandPool;
		std::unique_ptr<RenderGraph::RenderGraph> rendergraph;
		tracy::VkCtx* tracyContext;

	private:

		void initialiseCommandPool();
		void initialiseDescriptorPool(GraphicsPipelineKey key);
		void initialiseDescriptorSets(GraphicsPipelineKey key);
		void windowResize();

	public:

		
		RenderGraph::RenderGraph* CreateRenderGraph(RenderGraph::GraphBuilder& builder)
		{
			rendergraph = std::make_unique<RenderGraph::RenderGraph>(this, builder);
			return rendergraph.get();
		}
		Memory::Allocator* GetAllocator() { return allocator; }

		Device* GetDevice() { return &device; }

		Swapchain* GetSwapchain() { return &swapchain; }

		RenderpassCache* GetRenderpassCache() { return &renderpassCache; }
		GraphicsPipelineCache* GetGraphicsPipelineCache() { return &graphicsPipelineCache; }
		FramebufferCache* GetFramebufferCache() { return &framebufferCache; }
		DescriptorSetCache* GetDescriptorSetCache() { return &descriptorCache; }

		ShaderManager* GetShaderManager() { return shaderManager; }
		Settings* GetSettings() { return &settings; }
		InputHandler* GetInputHandler() { return &inputHandler; }


		void SetImageLayout(VkCommandBuffer buffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

		VkCommandBuffer GetCommandBuffer(VkCommandBufferLevel level, bool begin);
		void FlushCommandBuffer(VkCommandBuffer buffer);

		void BeginFrame(VkCommandBuffer& buffer, FrameInfo& info);
		void EndFrame(FrameInfo info);

	private:
		uint32_t maxFramesInFlight;
		int bufferCopies = 2;
	};
}
