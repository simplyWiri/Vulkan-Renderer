#include "../../Utils/Logging.h"
#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>

namespace Renderer
{
	bool Core::initialiseRenderer()
	{
		TempLogger::Init();
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		bool success = false;

		window.initialiseWindow(640, 400, "Vulkan Renderer");
		success = window.buildWindow();
		Wrappers::buildInstance(&context);
		success = window.buildSurface(&context);
		Wrappers::pickPhysicalDevice(&context, &window);
		Wrappers::buildDevice(&context);
		success = swapchain.buildSwapchain(&context, &window);
		renderpassCache.buildCache(&context.device.device);
		RenderpassKey key = RenderpassKey(
			{ { swapchain.getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }
		, {});
		success = renderpassCache.add(key);
		pipelineCache.buildCache(&context.device.device);

		//std::vector<std::shared_ptr<Shader>> shaders;
		//VkRenderPass renderpass;
		//VkPipelineLayout layout;
		//VkExtent2D extent;
		//DepthSettings depthSetting;
		//std::vector<BlendSettings> blendSettings;
		//VkPrimitiveTopology topology;

		auto gpKey = pipelineCache.bakeKey(
			renderpassCache[key]->getHandle(),
			swapchain.extent,
			DepthSettings::Disabled(),
			{ BlendSettings::Add() },
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			{
				std::make_shared<Shader>(ShaderType::Vertex, "resources/VertexShader.vert", 1),
				std::make_shared<Shader>(ShaderType::Fragment, "resources/FragmentShader.frag", 1)
			});
		
		auto dump = pipelineCache[gpKey];

		Assert(success, "Failed to build state for the renderer");

		maxFramesInFlight = swapchain.getSize();

		switch (settings.buffering) {
		case RendererBufferSettings::SwapchainSync: bufferCopies = maxFramesInFlight;
		case RendererBufferSettings::SingleBuffered: bufferCopies = 1;
		case RendererBufferSettings::DoubleBuffered: bufferCopies = 2;
		case RendererBufferSettings::TripleBuffered: bufferCopies = 3;
		}

		return true;
	}

	Core::~Core()
	{
		pipelineCache.clearCache();
		renderpassCache.clearCache();
		swapchain.cleanupSwapchain();
		context.cleanupDevice();
		window.cleanup();
		context.cleanupInstance();
	}
}