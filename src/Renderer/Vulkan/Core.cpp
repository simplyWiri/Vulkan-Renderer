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

		VerboseLog("Initialising Window");
		window.initialiseWindow(640, 400, "Vulkan Renderer");
		VerboseLog("Building Window");
		success = window.buildWindow();
		VerboseLog("Building Instance");
		Wrappers::buildInstance(&context);
		VerboseLog("Building Surface");
		success = window.buildSurface(&context);
		Wrappers::pickPhysicalDevice(&context, &window);
		VerboseLog("Building Device");
		Wrappers::buildDevice(&context);
		VerboseLog("Building Swapchain");
		success = swapchain.buildSwapchain(&context, &window);
		VerboseLog("Building renderpass Cache");
		renderpassCache.buildCache(&context.device.device);
		RenderpassKey key = RenderpassKey(
			{ { swapchain.getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }
		, {});
		VerboseLog("Adding to renderpass Cache");
		success = renderpassCache.add(key);
		VerboseLog("Building pipeline Cache");
		pipelineCache.buildCache(&context.device.device);

		VerboseLog("Baking pipeline Key");
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
		VerboseLog("Inserting pipeline Key");
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